#include "chd.h"
#include "down.h"

#include <limits.h>
#include <sys/stat.h>

struct stat file_stat;


static char *tok_html(const char *tmp_file) {
  FILE *fp = fopen(tmp_file, "r");
  if (!fp) {
    perror("Failed to open file");
    return NULL;
  }

  // 获取文件大小
  fseek(fp, 0, SEEK_END);
  const size_t file_size = (size_t)ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // 分配内存并读取文件内容
  char *tmp = (char *)malloc(file_size + 1);
  if (!tmp) {
    perror("Failed to allocate memory");
    fclose(fp);
    return NULL;
  }

  size_t retfd = fread(tmp, 1, file_size, fp);
  fclose(fp);

  if (retfd != file_size) {
    printf("Read Error!\n");
    free(tmp);
    return NULL;
  }
  tmp[file_size] = '\0'; // 确保字符串以 null 结尾

  // 动态数组存储分割后的字符串
  char **tokens = NULL;
  size_t token_count = 0;
  char *ptr = NULL;
  char *token = strtok_r(tmp, "/", &ptr);

  while (token != NULL) {
    tokens = (char **)realloc(tokens, (token_count + 1) * sizeof(char *));
    tokens[token_count] = strdup(token);
    token_count++;
    token = strtok_r(NULL, " ", &ptr);
  }

  // 查找目标字符串
  char *ret3 = NULL;
  for (size_t j = 0; j < token_count; j++) {
    if (strstr(tokens[j], "title")) {
      if (j > 0 && strstr(tokens[j - 1], "href")) {
        ret3 = (char *)malloc(strlen(tokens[j - 1]) + 1);
        if (ret3) {
          sscanf(tokens[j - 1], "%*[^\"]\"%[^\"]", ret3);
        }
        break;
      }
    }
  }

  // 释放内存
  free(tmp);
  for (size_t i = 0; i < token_count; i++) {
    free(tokens[i]);
  }
  free(tokens);

  return ret3;
}

int pull(const char *pod_name, const char *pod_ver) {
  // 参数检查
  const char *pod_arch = get_arch();
  if (pod_name == NULL || pod_ver == NULL || pod_arch == NULL) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Invalid parameters!");
    return -1;
  }

  if (strlen(pod_name) + strlen(pod_ver) + strlen(pod_arch) > 64) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "OUT OF MEM!");
    return -1;
  }

  char htm_file[PATH_MAX] = {0};
  char sha_file[PATH_MAX] = "./SHA256SUMS";
  char def_link[PATH_MAX] = {0};
  char pod_url[PATH_MAX] = {0};
  char pod_sha[PATH_MAX] = {0};
  char extract_dir[PATH_MAX] = {0};
  char rfs_n[PATH_MAX] = {0};

  // 构造rootfs文件名
  if (snprintf(rfs_n, PATH_MAX, "%s/%s_%s+%s.tar.xz", cfg.tmp_dir, pod_name,
               pod_ver, pod_arch) >= PATH_MAX ||
      snprintf(htm_file, PATH_MAX, "%s/default.htm", cfg.tmp_dir) >= PATH_MAX) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "File name too long!");
    unlink(htm_file); // 删除临时文件
    return -1;
  }

  if (snprintf(extract_dir, PATH_MAX, "%s/%s_%s", cfg.cfg_path, pod_name,
               pod_ver) >= PATH_MAX) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "File name too long!");
    unlink(htm_file); // 删除临时文件
    return -1;
  }

  // 构造默认链接
  if (snprintf(def_link, PATH_MAX, "%s%s/%s/%s/default/", sou_link, pod_name,
               pod_ver, pod_arch) >= PATH_MAX) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Path too long!");
    return -1;
  }

  // 下载HTML文件
  if (downloader(def_link, htm_file) < 0) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Failed to get HTML file");
    return -1;
  }

  // 解析HTML文件
  char *ret = tok_html(htm_file);
  if (ret == NULL) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Failed to parse HTML file!");
    unlink(htm_file); // 删除临时文件
    return -1;
  }

  // 构造pod_url和pod_sha
  if (snprintf(pod_url, PATH_MAX, "%s%srootfs.tar.xz", def_link, ret) >=
          PATH_MAX ||
      snprintf(pod_sha, PATH_MAX, "%s%sSHA256SUMS", def_link, ret) >=
          PATH_MAX) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Path too long!");
    unlink(htm_file); // 删除临时文件
    return -1;
  }

  if (stat(rfs_n, &file_stat) != 0) {

    // 下载rootfs文件
    if (downloader(pod_url, rfs_n) < 0) {
      printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Failed to get rootfs file!");
      unlink(htm_file); // 删除临时文件
      return -1;
    }
  }

  // 下载SHA256文件
  if (downloader(pod_sha, sha_file) < 0) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Failed to get SHA256 file!");
    unlink(htm_file); // 删除临时文件
    unlink(rfs_n);    // 删除rootfs文件
    return -1;
  }

  // 校验SHA256
  if (check_sha256(rfs_n) == false) {
    unlink(htm_file); // 删除临时文件
    unlink(rfs_n);    // 删除rootfs文件
    unlink(sha_file); // 删除SHA256文件
    return -1;
  }

  // 清理临时文件
  if (unlink(htm_file) != 0 || unlink(sha_file) != 0) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Clear failed!");
    return -1;
  }

  if (extract(rfs_n, extract_dir) < 0) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Failed to extract rootfs file!");
    return -1;
  }

  init_container(extract_dir);

  return 0;
}
