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

#include "include/chd.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUFSIZE 1024
#define REU_SIZE 4096
typedef struct {
  int status_code;        // HTTP/1.1 '200' OK
  char content_type[128]; // Content-Type: application/gzip
  long content_length;    // Content-Length: 11683079
  char file_name[256];
} resp_header_def;

resp_header_def resp;

#include <stdio.h>
#include <unistd.h>

// ANSI 控制码
#define CLEAR_LINE "\033[2K\r" // 清除整行并回到行首
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define COLOR_CYAN "\033[36m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"

void update_progress_bar(long downloaded, long total_size) {
  static int last_percent = -1;
  const int bar_width = 40;

  // 参数校验
  if (total_size <= 0 || downloaded < 0)
    return;

  // 计算百分比
  int percent = (int)((downloaded * 100.0) / total_size);
  if (percent > 100)
    percent = 100;

  // 仅当百分比变化时更新
  if (percent == last_percent)
    return;
  last_percent = percent;

  // 构建进度条
  int filled = (percent * bar_width) / 100;

  // 使用 \r 确保始终在同一行
  //  printf("%s%s", CLEAR_LINE, HIDE_CURSOR);  // 清除行并隐藏光标
  printf("%s", CLEAR_LINE);
  // 左侧百分比
  printf("%s[%3d%%]%s ", COLOR_CYAN, percent, COLOR_RESET);

  // 进度条主体
  printf("%s|", COLOR_GREEN);
  for (int i = 0; i < bar_width; ++i) {
    putchar(i < filled ? '=' : i == filled ? '>' : ' ');
  }
  printf("|%s", COLOR_RESET);

  // 右侧大小显示
  printf(" (%ld/%ld MB)", downloaded / (1024 * 1024),
         total_size / (1024 * 1024));

  fflush(stdout);

  // 下载完成时换行并恢复光标
  if (percent == 100) {
    printf("\n%s", SHOW_CURSOR);
  }
}

static int get_resp_header(const char *response, resp_header_def *resp) {
  char *pos = strstr(response, "HTTP/");
  if (pos) {
    sscanf(pos, "%*s %d", &resp->status_code);
  }
  pos = strstr(response, "Content-Type:");
  if (pos) {
    sscanf(pos, "%*s %s", resp->content_type);
  }
  pos = strstr(response, "Content-Length:");
  if (pos) {
    sscanf(pos, "%*s %ld", &resp->content_length);
  }
  return 0;
}

static int getHttpHead(int fd, char *buf, int bufLen) {
  char tmp[1] = {0};
  int i = 0;
  int offset = 0;
  int nbytes = 0;

  while ((nbytes = recv(fd, tmp, 1, 0)) == 1) {
    if (offset > bufLen - 1) {
      return bufLen;
    }

    if (i < 4) {
      if (tmp[0] == '\r' || tmp[0] == '\n') {
        i++;
      } else {
        i = 0;

        strncpy(buf + offset, tmp, 1);
        offset++;
      }
    }
    if (4 == i) {
      return offset;
    }
  }

  return -1;
}

static int geturl(char *url) {
  int cfd;
  struct sockaddr_in cadd;
  struct hostent *pURL = NULL;
  char host[BUFSIZE], GET[BUFSIZE];
  char request[REU_SIZE];
  char text[BUFSIZE] = {0};

  sscanf(url, "%*[^//]//%[^/]%s", host, GET);
  //  printf("Host = %s\n", host);
  //  printf("GET = %s\n", GET);

  // const char *red_color = "\033[31m";
  const char *green_color = "\033[32m";
  const char *reset_color = "\033[0m";

  printf("%sUrl%s :  %s\n", green_color, reset_color, url);

  if (-1 == (cfd = socket(AF_INET, SOCK_STREAM, 0))) {
    printf("create socket failed of client!\n");
    return (-1);
  }

  if ((pURL = gethostbyname(host)) == 0) {
    cperror(RED, "ERROR");
    return -1;
  }

  memset(&cadd, 0x00, sizeof(struct sockaddr_in));
  cadd.sin_family = AF_INET;
  cadd.sin_addr.s_addr = *((unsigned long *)pURL->h_addr_list[0]);
  cadd.sin_port = htons(80);

  char UA[BUFSIZE] = "curl/8.6.0";
  snprintf(request, REU_SIZE,
           "GET %s HTTP/1.1\r\n"
           "HOST: %s\r\n"
           "User-Agent: %s\r\n"
           "Cache-Control: no-cache\r\n"
           "Connection: close\r\n\r\n",
           GET, host, UA);

  if (-1 == connect(cfd, (struct sockaddr *)&cadd, (socklen_t)sizeof(cadd))) {
    printf("connect failed of client!\n");
    return -1;
  }

  if (-1 == send(cfd, request, strlen(request), 0)) {
    printf("Send request failed!\n");
    return -1;
  }

  getHttpHead(cfd, text, sizeof(text));
  printf("%sHead%s :  %s\n", green_color, reset_color, text);
  get_resp_header(text, &resp);
  printf("%sResult%s :\n Content_length:%ld\n Status_code:%d\n "
         "Content-Type:%s\n",
         green_color, reset_color, resp.content_length, resp.status_code,
         resp.content_type);

  if (resp.status_code == 404) {
    fprintf(stderr, "Error: 404 No such file\n");
    return -1;
  }

  return cfd;
}

int downloader(char addr[], char *sa_nam) {
  int sockfd = -1;
  char buf[BUFSIZE] = {0};
  char fileName[BUFSIZE] = {0};
  int fd = -1;

  snprintf(fileName, BUFSIZE, "%s", sa_nam);
  if ((sockfd = geturl(addr)) < 0) {
    return -1;
  }

  printf("file = %s\n", fileName);
  remove(fileName);

  fd = open(fileName, O_WRONLY | O_CREAT, 0777);
  if (fd == -1) {
    perror("Open error");
    goto __END;
  }
  int len = 0;
  while (1) {
    memset(buf, 0, BUFSIZE);
    int cr;
    cr = recv(sockfd, buf, BUFSIZE, 0);
    len += cr;
    if (resp.content_length != 0) {
      // printf("\r[%d/%ld]", len, resp.content_length);
      update_progress_bar(len, resp.content_length);
      fflush(stdout);
    }
    if (cr <= 0) {
      printf("\n");
      break;
    }
    if (write(fd, buf, cr) < 0) {
      perror("Err");
      goto __END;
    }
  }
__END:
  close(fd);
  close(sockfd);
  return 0;
}
