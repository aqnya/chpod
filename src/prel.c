#include "include/chd.h"

#include <sys/ptrace.h>

#ifdef NO_DEBUG
__attribute((constructor)) void is_be_trace() {
  if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == 0) {
  } else {
    printf("chd is running in debug!\n");
    exit(8);
  }
}
#endif
