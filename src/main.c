#include "chd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 命令处理函数声明
void handle_install(int argc, char *argv[]);
void handle_help(int argc, char *argv[]);
void handle_run(int argc, char *argv[]);
void handle_del(int argc, char *argv[]);

// 命令选项结构体
typedef struct {
    const char *short_opt;
    const char *long_opt;
    void (*handler)(int argc, char *argv[]);
    const char *description;
} CommandOption;

// 命令注册表
CommandOption options[] = {
    {"-i", "--install", handle_install, "Install rootfs."},
    {"-h", "--help", handle_help, "Show help information."},
    {"-r", "--run", handle_run, "Run container with proot."},
    {"-d","--del",handle_del,"Delete rootfs."},
    {NULL, NULL, NULL, NULL}
};

// 处理安装命令
void handle_install(int argc, char *argv[]) {
    if (argc >= 4) {
        pull(argv[2], argv[3]);
    } else {
        fprintf(stderr, "Error: insufficient arguments for install\n");
        handle_help(argc, argv);
    }
}

void handle_del(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Error: missing container name\n");
        fprintf(stderr, "Usage: %s --del <container_name>\n", argv[0]);
        return ;
    }

    const char *container_name = argv[2];
    char *path = find_container_path(container_name);
    
    if (!path) {
        fprintf(stderr, "Error: container '%s' not found\n", container_name);
        return ;
    }

    printf("Deleting container: %s\n", container_name);
    
    int ret = delete_path(path);
    free(path);  // 释放find_container_path分配的内存
    
    if (ret != 0) {
        fprintf(stderr, "Failed to delete container: %s (error %d)\n", 
                container_name, ret);
        return ;
    }

    printf("Successfully deleted: %s\n", container_name);
}

// 处理帮助命令
void handle_help(int argc, char *argv[]) {
(void)argc;
    printf("Usage: %s [options] program [arg...]\n", argv[0]);
    printf("Options:\n");
    for (size_t i = 0; options[i].short_opt != NULL; i++) {
        printf("  %s, %s\t%s\n", 
               options[i].short_opt, 
               options[i].long_opt, 
               options[i].description);
    }
}

// 处理运行命令
void handle_run(int argc, char *argv[]) {
    const char *container_name = argc >= 3 ? argv[2] : NULL;
    run_proot_container(container_name);
}

// 解析命令行参数并分发
void dispatch_arguments(int argc, char *argv[]) {
    if (argc < 2) {
        handle_help(argc, argv);
        return;
    }

    const char *option = argv[1];
    for (size_t i = 0; options[i].short_opt != NULL; i++) {
        if (strcmp(option, options[i].short_opt) == 0 || 
            strcmp(option, options[i].long_opt) == 0) {
            options[i].handler(argc, argv);
            return;
        }
    }

    fprintf(stderr, "Error: unknown option \"%s\"\n", option);
    handle_help(argc, argv);
}

int main(int argc, char *argv[]) {
    // 初始化配置
    config_init();

    // 分发命令行参数
    dispatch_arguments(argc, argv);

    // 清理配置
    config_cleanup();
    return 0;
}