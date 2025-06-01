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

bool check_sha256(const char *rfs_dir) {
  FILE *fp = fopen("./SHA256SUMS", "r");
  if (!fp) {
    fprintf(stderr, "No SHA256SUMS file!\n");
    return false;
  }

  char data[100];
  char *temp_p = fgets(data, sizeof(data), fp);
  fclose(fp);
  if (!temp_p) {
    return false;
  }

  char *digest = calculate_file_sha256(rfs_dir);
  if (!digest) {
    return false;
  }

  int is_valid = (memcmp(digest, data, 64) == 0);

  if (!is_valid) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Check sha256 failed!");
  }

  free(digest);

  return true;
}