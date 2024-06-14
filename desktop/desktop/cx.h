//
//  cx.h
//  desktop
//
//  Created by Taras Shchybovyk on 8/10/18.
//  Copyright © 2018 Taras Shchybovyk. All rights reserved.
//

#ifndef cx_h
#define cx_h

#include <CommonCrypto/CommonDigest.h>

typedef struct {
    CC_SHA256_CTX header;
} cx_sha256_t;

typedef struct {
    CC_SHA256_CTX header;
} cx_ripemd160_t;

typedef struct {
} cx_hmac_sha256_t;

typedef struct {
} cx_hmac_t;

//#define cx_hash(x, y, z, l, m)
#define cx_hmac(x, y, z, l, m)
#define cx_hmac_sha256_init(x, y, z)
#define cx_ripemd160_init(x)
//#define cx_sha256_init(x) SHA256_Init

#define CX_LAST 1
void cx_sha256_init(CC_SHA256_CTX *x);

void cx_hash(CC_SHA256_CTX *x,
             unsigned char flag,
             unsigned char *buffer,
             unsigned int size,
             unsigned char *outbuff);

#endif /* cx_h */
