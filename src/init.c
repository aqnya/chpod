#include "chd.h"

int fix_dns(const char *dir) {
  char dns_dir[PATH_MAX] = {0};
  if (snprintf(dns_dir, PATH_MAX, "%s%s", dir, "/etc/resolv.conf") >=
      PATH_MAX) {
    plog(ERROR, "Path too long!");
    return -1;
  }
  unlink(dns_dir);

  FILE *dns_ptr;
  const char *filename = dns_dir;

  // 打开文件（写入模式，若存在则清空内容）
  dns_ptr = fopen(filename, "w");

  // 检查文件是否成功打开
  if (dns_ptr == NULL) {
    perror("Error opening file");
    return -1;
  }

  // 写入内容
  fprintf(dns_ptr, "nameserver 8.8.8.8\n");

  // 关闭文件
  if (fclose(dns_ptr) != 0) {
    perror("Error closing file");
    return -1;
  }

  printf("Fixed DNS file.\n");
  return 0;
}

void init_container(const char *dir) {
  if (fix_dns(dir) < 0) {
    exit(EXIT_FAILURE);
  }
}
