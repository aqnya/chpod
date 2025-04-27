#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "chd.h"

// 定义缓冲区大小，用于存储文件路径等信息
#define BUFFER_SIZE 4096

void deldir(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过当前目录和父目录
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[BUFFER_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat file_stat;
        if (stat(full_path, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        // 如果是文件，直接删除
        if (S_ISREG(file_stat.st_mode)) {
            if (unlink(full_path) == -1) {
                perror("unlink");
            }
        }
        // 如果是目录，递归删除其内容后再删除该目录本身
        else if (S_ISDIR(file_stat.st_mode)) {
            deldir(full_path);
            // 删除空目录
            if (rmdir(full_path) == -1) {
                perror("rmdir");
            }
        }
    }

    closedir(dir);
}

int delfs(char *dir){
char *path;
path= find_container_path(dir);
printf("%s\n",path);
return 0;
}
