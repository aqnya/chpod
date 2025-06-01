typedef struct {
    char tmp_dir[PATH_MAX];
    char cfg_path[PATH_MAX];
    char home[PATH_MAX];
} cfg_head;

extern cfg_head cfg;

// 声明初始化/清理函数（如果需要）
void config_init(void);
void config_cleanup(void);