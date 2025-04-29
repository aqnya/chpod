#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <stdatomic.h>

#define BUFSIZE 1024
#define REU_SIZE 4096
#define MAX_THREADS 4

typedef struct {
    int status_code;
    char content_type[128];
    long content_length;
    char file_name[256];
} resp_header_def;

typedef struct {
    int thread_id;
    const char *url;
    const char *filename;
    long start_byte;
    long end_byte;
    atomic_long *total_downloaded;
} ThreadData;

resp_header_def resp;

#define CLEAR_LINE "\033[2K\r"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define COLOR_CYAN "\033[36m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"

static atomic_long global_downloaded = 0;

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
        sscanf(pos, "%*s %127s", resp.content_type); // 防止缓冲区溢出
    }

    if ((pos = strstr(response, "Content-Range: bytes"))) {
        long total;
        sscanf(pos, "%*[^/]/%ld", &total); // 修正解析格式
        resp.content_length = total;
    } else if ((pos = strstr(response, "Content-Length:"))) {
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

static int connect_to_url_part(const char *url, long start, long end, int is_probe, int *status_code) {
    int cfd;
    struct sockaddr_in cadd;
    struct hostent *host_entry;
    char host[BUFSIZE], path[BUFSIZE], request[REU_SIZE];

    if (sscanf(url, "%*[^//]//%[^/]%s", host, path) != 2) {
        fprintf(stderr, "Invalid URL format.\n");
        return -1;
    }

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

    if (is_probe) {
        snprintf(request, REU_SIZE,
                 "HEAD %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "User-Agent: curl/8.6.0\r\n"
                 "Connection: close\r\n\r\n", // 使用HEAD方法减少数据传输
                 path, host);
    } else {
        snprintf(request, REU_SIZE,
                 "GET %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "User-Agent: curl/8.6.0\r\n"
                 "Range: bytes=%ld-%ld\r\n"
                 "Connection: close\r\n\r\n",
                 path, host, start, end);
    }

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
    parse_response_header(response);

    *status_code = resp.status_code;
    return cfd; // 返回套接字描述符，调用者负责关闭
}

void* download_part(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int sockfd, fd;
    char buf[BUFSIZE];
    int status_code;

    sockfd = connect_to_url_part(data->url, data->start_byte, data->end_byte, 0, &status_code);
    if (sockfd < 0 || (status_code != 200 && status_code != 206)) {
        fprintf(stderr, "Thread %d failed to connect (status: %d)\n", data->thread_id, status_code);
        pthread_exit(NULL);
    }

    if ((fd = open(data->filename, O_WRONLY | O_CREAT, 0777)) == -1) {
        perror("Failed to open file");
        close(sockfd);
        pthread_exit(NULL);
    }

    lseek(fd, data->start_byte, SEEK_SET); // 预定位写入位置

    while (1) {
        int bytes_received = recv(sockfd, buf, BUFSIZE, 0);
        if (bytes_received <= 0) break;

        if (write(fd, buf, bytes_received) < 0) {
            perror("Write error");
            break;
        }

        atomic_fetch_add(data->total_downloaded, bytes_received);
        update_progress_bar(atomic_load(data->total_downloaded), resp.content_length);
    }

    close(fd);
    close(sockfd);
    pthread_exit(NULL);
}

int downloader(const char *url, const char *filename) {
    printf("%sUrl%s :  %s\n", COLOR_GREEN, COLOR_RESET, url);

    // Step 1: Probe server for headers
    int status;
    int probe_fd = connect_to_url_part(url, 0, 0, 1, &status);
    if (probe_fd >= 0) close(probe_fd); // 关闭探测连接

    if (status != 200 && status != 206) {
        fprintf(stderr, "Server does not support range requests (status: %d).\n", status);
        return -1;
    }

    // Step 2: Multi-thread download
    int num_threads = MAX_THREADS;
    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];
    long part_size = resp.content_length / num_threads;

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    close(fd); // 预创建并清空文件

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].url = url;
        thread_data[i].filename = filename;
        thread_data[i].start_byte = i * part_size;
        thread_data[i].end_byte = (i == num_threads-1) ? resp.content_length-1 : (i+1)*part_size-1;
        thread_data[i].total_downloaded = &global_downloaded;

        if (pthread_create(&threads[i], NULL, download_part, &thread_data[i])) {
            perror("Thread creation failed");
            return -1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nDownload complete.\n");
    return 0;
}