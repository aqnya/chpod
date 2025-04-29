#include <archive.h>
#include <archive_entry.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "chd.h"

static void safe_path(const char *destdir, const char *path, char *safe_path,
                      size_t size) {
  char resolved_dest[PATH_MAX];
  if (realpath(destdir, resolved_dest) == NULL) {
    printc(FG_RED,BG_DEFAULT,STYLE_RESET,"realpath failed for destination directory");
    exit(1);
  }

  char full_path[PATH_MAX];
  snprintf(full_path, sizeof(full_path), "%s/%s", resolved_dest, path);

  char resolved_full_path[PATH_MAX];
  if (realpath(full_path, resolved_full_path) == NULL) {
    // 处理路径不存在的情况（可能需要创建目录）
    snprintf(safe_path, size, "%s/%s", resolved_dest, path);
    return;
  }

  // 检查解析后的路径是否在目标目录下
  if (strncmp(resolved_full_path, resolved_dest, strlen(resolved_dest)) != 0) {
    printc(FG_RED,BG_DEFAULT,STYLE_RESET,"[ERROR] Potential attacks：%s\n", path);
    exit(1);
  }

  strncpy(safe_path, resolved_full_path, size);
  safe_path[size - 1] = '\0';
}

int extract(const char *filename, const char *destdir) {
  struct archive *a = archive_read_new();
  struct archive *ext = archive_write_disk_new();
  struct archive_entry *entry;
  int flags;
  int r;

  // 确保目标目录存在
  mkdir(destdir, 0755);

  // 设置读取选项：支持所有格式和xz压缩
  archive_read_support_format_all(a);
  archive_read_support_filter_xz(a);

  // 打开压缩文件
  if ((r = archive_read_open_filename(a, filename, 10240))) {
    fprintf(stderr, "无法打开文件: %s\n", archive_error_string(a));
    return 1;
  }

  // 设置解压选项：保留权限和时间
  flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL |
          ARCHIVE_EXTRACT_FFLAGS;
  archive_write_disk_set_options(ext, flags);

  for (;;) {
    r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF)
      break;
    if (r < ARCHIVE_OK)
      printc(FG_RED,BG_DEFAULT,STYLE_RESET,"错误: %s\n", archive_error_string(a));
    if (r < ARCHIVE_WARN)
      goto cleanup;

    // 处理路径安全
    const char *pathname = archive_entry_pathname(entry);
    char safepath[PATH_MAX];
    safe_path(destdir, pathname, safepath, sizeof(safepath));

    // 更新条目路径
    archive_entry_set_pathname(entry, safepath);

    // 写入文件到磁盘
    r = archive_write_header(ext, entry);
    if (r < ARCHIVE_OK)
      printc(FG_RED,BG_DEFAULT,STYLE_RESET,"[Warning]: %s\n", archive_error_string(ext));
    if (r == ARCHIVE_FATAL)
      goto cleanup;

    if (archive_entry_size(entry) > 0) {
      // 复制文件数据
      const void *buff;
      size_t size;
      la_int64_t offset;

      while ((r = archive_read_data_block(a, &buff, &size, &offset)) !=
             ARCHIVE_EOF) {
        if (r < ARCHIVE_OK) {
          fprintf(stderr, "数据读取错误: %s\n", archive_error_string(a));
          goto cleanup;
        }
        if (archive_write_data_block(ext, buff, size, offset) < ARCHIVE_OK) {
          fprintf(stderr, "写入错误: %s\n", archive_error_string(ext));
          goto cleanup;
        }
      }
    }

    r = archive_write_finish_entry(ext);
    if (r < ARCHIVE_OK)
      printc(FG_RED,BG_DEFAULT,STYLE_RESET, "完成条目失败: %s\n", archive_error_string(ext));
    if (r == ARCHIVE_FATAL)
      goto cleanup;
  }

cleanup:
  archive_read_close(a);
  archive_read_free(a);
  archive_write_close(ext);
  archive_write_free(ext);
  return 0;
}
