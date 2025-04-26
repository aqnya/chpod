#include <stdio.h>
extern char config_h[PATH_MAX];
extern char config_tmp[PATH_MAX];
extern char* v_home;

// 声明初始化/清理函数（如果需要）
void config_init(void);
void config_cleanup(void);