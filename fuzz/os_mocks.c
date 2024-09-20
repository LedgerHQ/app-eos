#include "os.h"
#include "cx.h"
#include "ux.h"
#include "cx_hash.h"
#include "lcx_sha256.h"
#include "lcx_ripemd160.h"

size_t strlcat(char *dst, const char *src, size_t Size) {
    size_t ld = strlen(dst);
    size_t ls = strlen(src);
    if (ld >= Size) return 0;
    if (ld + ls >= Size) ls = Size - ld - 1;
    strncat(dst, src, ls);
    return ld + ls;
}

void assert_exit(bool confirm) {
    exit(-1);
}

void os_longjmp(unsigned int exception) {
    longjmp(try_context_get()->jmp_buf, exception);
}

try_context_t *current_context = NULL;
try_context_t *try_context_get(void) {
    return current_context;
}

try_context_t *try_context_set(try_context_t *ctx) {
    try_context_t *previous_ctx = current_context;
    current_context = ctx;
    return previous_ctx;
}

bolos_task_status_t os_sched_last_status(unsigned int task_idx) {
    return 1;
};
unsigned short io_exchange(unsigned char chan, unsigned short tx_len) {
    return 0;
};
void *pic(void *linked_addr) {
    return linked_addr;
}

void io_seproxyhal_display_default(const bagl_element_t *bagl) {
    if (bagl->text) {
        printf("[-] %s\n", bagl->text);
    }
}

void ui_idle(){};

unsigned int os_ux(bolos_ux_params_t *params) {
    return 0;
};
void io_seproxyhal_init_ux(void){};
unsigned int io_seph_is_status_sent(void) {
    return 0;
};
bolos_bool_t os_perso_isonboarded(void) {
    return (bolos_bool_t) BOLOS_UX_OK;
};
void io_seproxyhal_general_status(void){};
void io_seph_send(const unsigned char *buffer, unsigned short length){};
unsigned short io_seph_recv(unsigned char *buffer, unsigned short maxlength, unsigned int flags) {
    return 0;
};
void halt() {
    for (;;)
        ;
};
bolos_bool_t os_global_pin_is_validated(void) {
    return (bolos_bool_t) BOLOS_UX_OK;
};
cx_err_t cx_hash_no_throw(cx_hash_t *hash,
                          uint32_t mode,
                          const uint8_t *in,
                          size_t len,
                          uint8_t *out,
                          size_t out_len) {
    return 0;
};

cx_err_t cx_hmac_sha256_init_no_throw(cx_hmac_sha256_t *hmac, const uint8_t *key, size_t key_len) {
    return 0;
};
cx_err_t cx_hmac_no_throw(cx_hmac_t *hmac,
                          uint32_t mode,
                          const uint8_t *in,
                          size_t len,
                          uint8_t *mac,
                          size_t mac_len) {
    return 0;
};

cx_err_t fake_update(cx_hash_t *ctx, const uint8_t *data, size_t len) {
    return 0;
}
cx_err_t fake_final(cx_hash_t *ctx, uint8_t *digest) {
    return 0;
}
static const uint32_t hzero[] = {0x6a09e667,
                                 0xbb67ae85,
                                 0x3c6ef372,
                                 0xa54ff53a,
                                 0x510e527f,
                                 0x9b05688c,
                                 0x1f83d9ab,
                                 0x5be0cd19};

const cx_hash_info_t cx_sha256_info = {
    CX_SHA256,
    CX_SHA256_SIZE,
    SHA256_BLOCK_SIZE,
    (cx_err_t(*)(cx_hash_t * ctx)) cx_sha256_init_no_throw,
    (cx_err_t(*)(cx_hash_t * ctx, const uint8_t *data, size_t len)) fake_update,
    (cx_err_t(*)(cx_hash_t * ctx, uint8_t *digest)) fake_final,
    NULL,
    NULL};

#define RIPEMD_BLOCK_SIZE 64

const cx_hash_info_t cx_ripemd160_info = {
    CX_RIPEMD160,
    CX_RIPEMD160_SIZE,
    RIPEMD_BLOCK_SIZE,
    (cx_err_t(*)(cx_hash_t * ctx)) cx_ripemd160_init_no_throw,
    (cx_err_t(*)(cx_hash_t * ctx, const uint8_t *data, size_t len)) fake_update,
    (cx_err_t(*)(cx_hash_t * ctx, uint8_t *digest)) fake_final,
    NULL,
    NULL};

cx_err_t cx_sha256_init_no_throw(cx_sha256_t *hash) {
    memset(hash, 0, sizeof(cx_sha256_t));
    hash->header.info = &cx_sha256_info;
    memmove(hash->acc, hzero, sizeof(hzero));
    return CX_OK;
}

static const uint32_t hzero_ripemd160[] = {0x67452301UL,
                                           0xEFCDAB89UL,
                                           0x98BADCFEUL,
                                           0x10325476UL,
                                           0xC3D2E1F0UL};
cx_err_t cx_ripemd160_init_no_throw(cx_ripemd160_t *hash) {
    memset(hash, 0, sizeof(cx_ripemd160_t));
    hash->header.info = &cx_ripemd160_info;
    memmove(hash->acc, hzero_ripemd160, sizeof(hzero_ripemd160));
    return CX_OK;
}

size_t cx_hash_get_size(const cx_hash_t *ctx) {
    const cx_hash_info_t *info = ctx->info;
    if (info->output_size) {
        return info->output_size;
    }
    return info->output_size_func(ctx);
}

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}
