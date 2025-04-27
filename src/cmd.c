#include "include/chd.h"
#include <dirent.h>  // For directory handling
#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // For exit()
#include <unistd.h>   // For access()

int check_proot(void) {
  if (execute_command("proot >/dev/null 2>&1") == 127) {
    cperror(RED, "You have not install proot.\n");
    return -1;
  }
  return 0;
}

// 检查容器目录是否存在
void list_installed_containers() {
    DIR *dir = opendir(config_h);
    if (!dir) {
        fprintf(stderr, "Error: Unable to open containers directory at %s\n", config_h);
        exit(EXIT_FAILURE);
    }

    printf("Available containers in %s:\n", config_h);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name,".tmp") != 0) {
            printf("  - %s\n", entry->d_name);
        }
    }
    closedir(dir);
}

// 自动查找容器路径
char *find_container_path(const char *container_name) {
    static char container_path[1024];
    snprintf(container_path, sizeof(container_path), "%s/%s", config_h, container_name);

    // 检查路径是否存在并可访问
    if (access(container_path, F_OK) != 0) {
        fprintf(stderr, "Error: Container '%s' not found in %s\n", container_name, config_h);
        return NULL;
    }

    return container_path;
}

// 运行 Proot 容器逻辑
void run_proot_container(const char *container_name) {
    if (check_proot() == -1) {
        goto EXIT;
    }

    char *container_path;
    if (container_name == NULL || strlen(container_name) == 0) {
        printf("No container specified. Searching for installed containers...\n");
        list_installed_containers();
        exit(EXIT_SUCCESS);
    } else {
        container_path = find_container_path(container_name);
        if (container_path == NULL) {
            exit(EXIT_FAILURE);
        }
    }

    char proot_cmd[2048] = {0};
    snprintf(proot_cmd, sizeof(proot_cmd),
             "proot --link2symlink -0 -r %s -b /dev -b /proc -b %s/root:/dev/shm "
             "-w /root /usr/bin/env -i HOME=/root "
             "PATH=/usr/local/sbin:/usr/local/bin:/bin:/sbin:/usr/sbin:/usr/games:/usr/local/games "
             "SHELL=/bin/bash TERM=$TERM LANG=C.UTF-8 /bin/bash --login",
             container_path, container_path);

    if (execute_command(proot_cmd) == -1) {
    EXIT:
        fprintf(stderr,
                "Failed to execute `%s`\nexecv() returned: %d\nerror reason: "
                "%s\nNote: unset $LD_PRELOAD before running may fix this issue.\n",
                proot_cmd, errno, strerror(errno));
    }
}