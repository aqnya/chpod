/*
 * Copyright (C) 2023-2024  nyaaaww
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int execute_command(const char *cmd) {
  if (cmd == NULL) {
    fprintf(stderr, "Error: Command string is NULL\n");
    errno = EINVAL;
    return -1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork() failed");
    return -1;
  }

  /* 子进程执行分支 */
  if (pid == 0) {
    // 使用_exit避免刷新标准I/O缓冲区
    execlp("sh", "sh", "-c", cmd, (char *)NULL);

    // 如果执行到这里说明execlp失败
    perror("execlp() failed");
    _exit(127); // 使用shell约定的127退出码表示命令未找到
  }

  /* 父进程执行分支 */
  int status;
  int wait_ret;

  // 处理信号中断的等待循环
  do {
    wait_ret = waitpid(pid, &status, 0);
  } while (wait_ret == -1 && errno == EINTR);

  if (wait_ret == -1) {
    perror("waitpid() failed");
    return -1;
  }

  // 处理子进程状态
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  } else {
    fprintf(stderr, "Command terminated by signal %d\n", WTERMSIG(status));
    return -1;
  }
}