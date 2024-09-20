//
//  cx.c
//  desktop
//
//  Created by Taras Shchybovyk on 11/6/18.
//  Copyright © 2018 Taras Shchybovyk. All rights reserved.
//

#include "cx.h"

void cx_sha256_init(CC_SHA256_CTX *x) {
    CC_SHA256_Init(x);
}

void cx_hash(CC_SHA256_CTX *x,
             unsigned char flag,
             unsigned char *buffer,
             unsigned int size,
             unsigned char *outbuff) {
    if (flag & CX_LAST) {
        CC_SHA256_Update(x, buffer, size);
        CC_SHA256_Final(outbuff, x);
    } else {
        CC_SHA256_Update(x, buffer, size);
    }
}
