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

#include "include/chd.h"

int check_proot(void) {
  if (execute_command("proot >/dev/null 2>&1") == 127) {
    cperror(RED, "You have not install proot.\n");
    return -1;
  }
  return 0;
}

void run_proot_container(const char *container_dir) {
  if (check_proot() == -1) {
    goto EXIT;
  }
  if (strlen(container_dir) * 2 > 1024) {
    fprintf(stderr, "PATH too long!\n");
  }
  char proot_cmd[2048] = {0};
  strncat(proot_cmd, "proot", 5);
  strncat(proot_cmd, " --link2symlink", 15);
  strncat(proot_cmd, " -0", 3);
  strncat(proot_cmd, " -r ", 4);
  //	strncat(proot_cmd, container_dir, strlen(container_dir));
  snprintf(proot_cmd, sizeof(proot_cmd), "%s%s", proot_cmd, container_dir);

  strncat(proot_cmd, " -b /dev", 8);
  strncat(proot_cmd, " -b /proc", 9);
  strncat(proot_cmd, " -b ", 4);
  //	strncat(proot_cmd, container_dir, strlen(container_dir));
  snprintf(proot_cmd, sizeof(proot_cmd), "%s%s", proot_cmd, container_dir);
  strncat(proot_cmd, "/root:/dev/shm", 14);
  strncat(proot_cmd, " -w /root /usr/bin/env", 22);
  strncat(proot_cmd, " -i HOME=/root", 14);
  strncat(proot_cmd,
          " PATH=/usr/local/sbin:/usr/local/bin:/bin:/sbin:/usr/sbin:/usr/"
          "games:/usr/local/games",
          86);
  strncat(proot_cmd, " SHELL=/bin/bash", 16);
  strncat(proot_cmd, " TERM=$TERM", 11);
  strncat(proot_cmd, " LANG=C.UTF-8", 13);
  strncat(proot_cmd, " /bin/bash --login", 18);
  if (execute_command(proot_cmd) == -1) {
  EXIT:
    fprintf(stderr,
            "Failed to execute `%s`\nexecv() returned: %d\nerror reason: "
            "%s\nNote: unset $LD_PRELOAD before running ruri might fix this.",
            proot_cmd, errno, strerror(errno));
  }
}
