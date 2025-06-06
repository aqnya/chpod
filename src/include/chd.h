/*
 * Copyright (C) 2023-2024  April8th
 * chd - Containerized Hosting Daemon
 * Header file with API declarations
 */

#ifndef CHD_H
#define CHD_H

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "color.h"
#include "config.h"

#if !defined(__linux__)
#    error "This program requires Linux operating system"
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char* get_arch(void);
int execute_command(const char* cmd);
int get_file_type(char filedir[]);
char* calculate_file_sha256(const char* file_path);
bool check_sha256(const char* rfs_dir);
int delete_path(const char *path);
void remove_files(const char* path);
void init_container(const char *dir);
void run_proot_container(const char *container_dir);
char *find_container_path(const char *container_name);
void list_installed_containers(void);
int downloader(const char *url, const char *filename);
int pull(const char* pod_name, const char* pod_ver);
int extract(const char *filename, const char *destdir);
void printc(ForegroundColor fg, BackgroundColor bg, TextStyle style, 
            const char *format, ...);
void show_help(const char* file);
void down_help(void);

#ifdef __cplusplus
}
#endif

#endif // CHD_H