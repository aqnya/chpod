#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义循环右移宏
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
// 定义逻辑函数
#define Ch(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define Sigma0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define Sigma1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sigma0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define sigma1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

// SHA-256常量 (前64个质数的立方根小数部分前32位)
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

// 初始哈希值 (前8个质数的平方根小数部分前32位)
static const uint32_t H0[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                               0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

// 处理单个512位块
static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
  uint32_t a, b, c, d, e, f, g, h;
  uint32_t W[64];

  // 将数据分割为16个32位大端字
  for (int i = 0, j = 0; i < 16; i++, j += 4) {
    W[i] = (uint32_t)data[j] << 24 | (uint32_t)data[j + 1] << 16 |
           (uint32_t)data[j + 2] << 8 | (uint32_t)data[j + 3];
  }

  // 扩展消息
  for (int i = 16; i < 64; i++) {
    W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
  }

  // 初始化工作变量
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  f = state[5];
  g = state[6];
  h = state[7];

  // 主循环
  for (int i = 0; i < 64; i++) {
    uint32_t T1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];
    uint32_t T2 = Sigma0(a) + Maj(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + T1;
    d = c;
    c = b;
    b = a;
    a = T1 + T2;
  }

  // 更新状态
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  state[5] += f;
  state[6] += g;
  state[7] += h;
}

// 计算SHA-256哈希
void sha256(const uint8_t *message, size_t len, uint8_t digest[32]) {
  uint32_t state[8];
  uint64_t bit_len = (uint64_t)len * 8;
  size_t new_len, block_len, pad_len;
  uint8_t *new_message;

  // 初始化状态
  memcpy(state, H0, sizeof(H0));

  // 计算填充长度
  new_len = len + 1; // 增加0x80字节
  pad_len = (new_len % 64 < 56) ? (56 - new_len % 64) : (120 - new_len % 64);
  new_len += pad_len + 8; // 添加填充和长度

  // 分配填充后的消息
  new_message = (uint8_t *)malloc(new_len);
  if (!new_message) {
    fprintf(stderr, "Memory allocation failed\n");
    return;
  }

  // 复制原始消息
  memcpy(new_message, message, len);
  // 添加0x80
  new_message[len] = 0x80;
  // 填充0
  memset(new_message + len + 1, 0, pad_len);
  // 添加大端表示的位长度
  for (int i = 0; i < 8; i++) {
    new_message[new_len - 8 + i] = (bit_len >> (56 - 8 * i)) & 0xFF;
  }

  // 处理每个512位块
  block_len = new_len / 64;
  for (size_t i = 0; i < block_len; i++) {
    sha256_transform(state, new_message + i * 64);
  }

  // 生成最终哈希值 (大端)
  for (int i = 0; i < 8; i++) {
    digest[i * 4] = (state[i] >> 24) & 0xFF;
    digest[i * 4 + 1] = (state[i] >> 16) & 0xFF;
    digest[i * 4 + 2] = (state[i] >> 8) & 0xFF;
    digest[i * 4 + 3] = state[i] & 0xFF;
  }

  // 清理
  free(new_message);
}

// 添加计算文件SHA-256哈希的函数
static void sha256_file(const char *filename, uint8_t digest[32]) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    return;
  }

  // 初始化状态
  uint32_t state[8];
  memcpy(state, H0, sizeof(H0));

  uint64_t total_bytes = 0; // 记录文件总字节数
  uint8_t block[64];        // 数据块缓冲区
  size_t bytes_read;        // 每次读取的字节数

  // 处理完整的64字节块
  while ((bytes_read = fread(block, 1, 64, file)) == 64) {
    sha256_transform(state, block);
    total_bytes += 64;
  }

  // 检查读取错误
  if (ferror(file)) {
    fprintf(stderr, "Error reading file: %s\n", strerror(errno));
    fclose(file);
    return;
  }

  // 更新最后不完整块的大小
  total_bytes += bytes_read;

  // 准备最后一块数据（包含填充和长度信息）
  uint8_t last_block[128]; // 最多可容纳两个块
  size_t last_block_size = 0;

  // 复制最后一块的原始数据
  if (bytes_read > 0) {
    memcpy(last_block, block, bytes_read);
    last_block_size = bytes_read;
  }

  // 添加填充字节0x80
  last_block[last_block_size++] = 0x80;

  // 如果当前块空间不足，填充至完整块并处理
  if (last_block_size > 56) {
    // 填充0直到块末尾
    while (last_block_size < 64) {
      last_block[last_block_size++] = 0;
    }
    sha256_transform(state, last_block);
    last_block_size = 0;
  }

  // 填充0直到长度字段前
  while (last_block_size < 56) {
    last_block[last_block_size++] = 0;
  }

  // 添加大端表示的位长度（64位）
  uint64_t bit_length = total_bytes * 8;
  for (int i = 0; i < 8; i++) {
    last_block[56 + i] = (bit_length >> (56 - 8 * i)) & 0xFF;
  }
  last_block_size = 64; // 最终块大小

  // 处理最后一块
  sha256_transform(state, last_block);

  // 生成最终哈希值（大端序）
  for (int i = 0; i < 8; i++) {
    digest[i * 4] = (state[i] >> 24) & 0xFF;
    digest[i * 4 + 1] = (state[i] >> 16) & 0xFF;
    digest[i * 4 + 2] = (state[i] >> 8) & 0xFF;
    digest[i * 4 + 3] = state[i] & 0xFF;
  }

  fclose(file);
}

// 将哈希值转换为可比较的字符串
void sha256_to_string(const uint8_t digest[32], char output[65]) {
  for (int i = 0; i < 32; i++) {
    sprintf(output + (i * 2), "%02x", digest[i]);
  }
  output[64] = '\0'; // 确保字符串以空字符结尾
}

// 比较两个哈希值是否相等
static int sha256_compare(const uint8_t digest1[32],
                          const uint8_t digest2[32]) {
  return memcmp(digest1, digest2, 32) == 0;
}

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

  uint8_t file_digest[32];
  sha256_file(rfs_dir, file_digest);
  uint8_t expected_digest[32];
  // 将预期哈希字符串转换为二进制格式
  for (int i = 0; i < 32; i++) {
    sscanf(data + i * 2, "%2hhx", &expected_digest[i]);
  }

  if (sha256_compare(file_digest, expected_digest)) {
    return true;
  } else {
    return false;
  }
}