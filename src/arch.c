#include <stdio.h>

const char *get_arch(void) {
#if defined(__x86_64__) || defined(_M_X64)
  return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
  return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
  return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
  return "arm";
#elif defined(__powerpc64__)
  return "PowerPC64";
#elif defined(__ppc__)
  return "PowerPC";
#elif defined(__mips__)
  return "MIPS";
#else
  return "Unknown";
#endif
}