
#include <errno.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#define MAX_BUF_LEN 4096

typedef struct {
  FILE *file;
  EVP_MD_CTX *md_ctx;
  unsigned char *digest;
  size_t digest_len;
} Sha256Context;

// 初始化SHA-256上下文
static int init_sha256_context(Sha256Context *ctx, const char *file_path) {
  ctx->file = fopen(file_path, "rb");
  if (!ctx->file) {
    perror("Error opening file");
    return 0;
  }
  ctx->md_ctx = EVP_MD_CTX_new();
  if (!ctx->md_ctx) {
    perror("Error allocating MD context");
    fclose(ctx->file);
    return 0;
  }
  if (!EVP_DigestInit_ex(ctx->md_ctx, EVP_sha256(), NULL)) {
    perror("Error initializing digest");
    EVP_MD_CTX_free(ctx->md_ctx);
    fclose(ctx->file);
    return 0;
  }
  ctx->digest = malloc(SHA256_DIGEST_LENGTH);
  if (!ctx->digest) {
    perror("Error allocating digest buffer");
    EVP_MD_CTX_free(ctx->md_ctx);
    fclose(ctx->file);
    return 0;
  }
  ctx->digest_len = SHA256_DIGEST_LENGTH;
  return 1;
}

static void free_sha256_context(Sha256Context *ctx) {
  if (ctx->digest) {
    free(ctx->digest);
  }
  if (ctx->md_ctx) {
    EVP_MD_CTX_free(ctx->md_ctx);
  }
  if (ctx->file) {
    fclose(ctx->file);
  }
}

char *calculate_file_sha256(const char *file_path) {
  Sha256Context ctx;
  if (!init_sha256_context(&ctx, file_path)) {
    return NULL;
  }

  char buf[MAX_BUF_LEN];
  size_t len;
  while ((len = fread(buf, 1, MAX_BUF_LEN, ctx.file)) > 0) {
    if (!EVP_DigestUpdate(ctx.md_ctx, buf, len)) {
      perror("Error updating digest");
      free_sha256_context(&ctx);
      return NULL;
    }
  }
  if (ferror(ctx.file)) {
    perror("Error reading file");
    free_sha256_context(&ctx);
    return NULL;
  }

  if (!EVP_DigestFinal_ex(ctx.md_ctx, ctx.digest, NULL)) {
    perror("Error finalizing digest");
    free_sha256_context(&ctx);
    return NULL;
  }

  char *hex_digest = malloc(2 * ctx.digest_len + 1);
  if (!hex_digest) {
    perror("Error allocating hex digest buffer");
    free_sha256_context(&ctx);
    return NULL;
  }

  for (size_t i = 0; i < ctx.digest_len; ++i) {
    snprintf(&hex_digest[i * 2], 3, "%02x", ctx.digest[i]);
  }

  free_sha256_context(&ctx);
  return hex_digest;
}
