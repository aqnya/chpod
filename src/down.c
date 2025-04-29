/*
 * Copyright (C) 2023-2024  nyaaaww
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "chd.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 1024
#define REU_SIZE 4096

typedef struct {
  int status_code;        // HTTP/1.1 '200' OK
  char content_type[128]; // Content-Type: application/gzip
  long content_length;    // Content-Length: 11683079
  char file_name[256];
} resp_header_def;

resp_header_def resp;

// ANSI 控制码
#define CLEAR_LINE "\033[2K\r"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define COLOR_CYAN "\033[36m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"

static void update_progress_bar(long downloaded, long total_size) {
  static int last_percent = -1;
  const int bar_width = 40;

  if (total_size <= 0 || downloaded < 0)
    return;

  int percent = (int)((downloaded * 100.0) / total_size);
  if (percent > 100)
    percent = 100;

  if (percent == last_percent)
    return;
  last_percent = percent;

  int filled = (percent * bar_width) / 100;

  printf("%s%s[%3d%%]%s %s|", CLEAR_LINE, COLOR_CYAN, percent, COLOR_RESET,
         COLOR_GREEN);
  for (int i = 0; i < bar_width; ++i) {
    putchar(i < filled ? '=' : (i == filled ? '>' : ' '));
  }
  printf("|%s (%ld/%ld MB)", COLOR_RESET, downloaded / (1024 * 1024),
         total_size / (1024 * 1024));
  fflush(stdout);

  if (percent == 100) {
    printf("\n%s", SHOW_CURSOR);
  }
}

static int parse_response_header(const char *response) {
  char *pos = strstr(response, "HTTP/");
  if (pos)
    sscanf(pos, "%*s %d", &resp.status_code);

  if ((pos = strstr(response, "Content-Type:"))) {
    sscanf(pos, "%*s %s", resp.content_type);
  }

  if ((pos = strstr(response, "Content-Length:"))) {
    sscanf(pos, "%*s %ld", &resp.content_length);
  }
  return 0;
}

static int fetch_http_header(int fd, char *buf, int buf_len) {
  char tmp[1];
  int i = 0, offset = 0, nbytes;

  while ((nbytes = recv(fd, tmp, 1, 0)) == 1) {
    if (offset >= buf_len - 1)
      return buf_len;

    if (i < 4) {
      if (tmp[0] == '\r' || tmp[0] == '\n') {
        i++;
      } else {
        i = 0;
        buf[offset++] = tmp[0];
      }
    }
    if (i == 4)
      return offset;
  }
  return -1;
}

static int connect_to_url(const char *url) {
  int cfd;
  struct sockaddr_in cadd;
  struct hostent *host_entry;
  char host[BUFSIZE], path[BUFSIZE], request[REU_SIZE];

  if (sscanf(url, "%*[^//]//%[^/]%s", host, path) != 2) {
    fprintf(stderr, "Invalid URL format.\n");
    return -1;
  }

  printf("%sUrl%s :  %s\n", COLOR_GREEN, COLOR_RESET, url);

  if ((cfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket creation failed");
    return -1;
  }

  if (!(host_entry = gethostbyname(host))) {
    fprintf(stderr, "Failed to resolve host: %s\n", host);
    close(cfd);
    return -1;
  }

  memset(&cadd, 0, sizeof(cadd));
  cadd.sin_family = AF_INET;
  cadd.sin_addr.s_addr = *(unsigned long *)host_entry->h_addr_list[0];
  cadd.sin_port = htons(80);

  snprintf(request, REU_SIZE,
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: curl/8.6.0\r\n"
           "Cache-Control: no-cache\r\n"
           "Connection: close\r\n\r\n",
           path, host);

  if (connect(cfd, (struct sockaddr *)&cadd, sizeof(cadd)) == -1) {
    perror("Connection failed");
    close(cfd);
    return -1;
  }

  if (send(cfd, request, strlen(request), 0) == -1) {
    perror("Failed to send request");
    close(cfd);
    return -1;
  }

  char response[BUFSIZE] = {0};
  fetch_http_header(cfd, response, sizeof(response));
  printf("%sHead%s :  %s\n", COLOR_GREEN, COLOR_RESET, response);
  parse_response_header(response);

  if (resp.status_code == 404) {
    fprintf(stderr, "Error: 404 Not Found\n");
    close(cfd);
    return -1;
  }

  return cfd;
}

// 下载器
int downloader(const char *url, const char *filename) {
  int sockfd = -1, fd = -1;
  char buf[BUFSIZE] = {0};
  int total_downloaded = 0;

  if ((sockfd = connect_to_url(url)) < 0)
    return -1;

  printf("Downloading to file: %s\n", filename);
  remove(filename);

  if ((fd = open(filename, O_WRONLY | O_CREAT, 0777)) == -1) {
    perror("Failed to open file");
    close(sockfd);
    return -1;
  }

  while (1) {
    int bytes_received = recv(sockfd, buf, BUFSIZE, 0);
    if (bytes_received <= 0)
      break;

    total_downloaded += bytes_received;
    update_progress_bar(total_downloaded, resp.content_length);

    if (write(fd, buf, bytes_received) < 0) {
      perror("Write error");
      close(fd);
      close(sockfd);
      return -1;
    }
  }

  printf("\nDownload complete.\n");
  close(fd);
  close(sockfd);
  return 0;
}