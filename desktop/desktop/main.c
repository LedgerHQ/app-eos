//
//  main.c
//  desktop
//
//  Created by Taras Shchybovyk on 8/10/18.
//  Copyright © 2018 Taras Shchybovyk. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include "eos_types.h"
#include "eos_types.h"
#include "eos_stream.h"
#include "eos_parse.h"

txProcessingContext_t txProcessingCtx;
txProcessingContent_t txContent;
cx_sha256_t sha256;
cx_sha256_t sha256_arg;

static int hex_to_bytes(const char *hex, size_t hex_len, uint8_t *buffer, size_t buffer_len) {
    if (hex_len % 2 != 0) {
        return -1;
    }

    if (hex_len / 2 != buffer_len) {
        return -1;
    }

    const char *pos = hex;

    for (size_t count = 0; count < hex_len / 2; ++count) {
        sscanf(pos, "%2hhx", &buffer[count]);
        pos += 2;
    }
    return 0;
}

bool onActionReady(txProcessingContext_t *context) {
    printf("--------------- Confirm action #%d ---------------\n", context->currentActionIndex);

    puts("Contract");
    puts(context->content->contract);
    puts("\n");

    puts("Action");
    puts(context->content->action);
    puts("\n");

    for (uint8_t i = 0; i < context->content->argumentCount; ++i) {
        printArgument(i, context);
        puts(context->content->arg.label);
        puts(context->content->arg.data);

        puts("\n");
    }

    return true;
}

int main(int argc, const char *argv[]) {
#define NEWACCOUNT 1
#ifdef VOTE
    const char tx[] =
        "0420cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f0404d0d3495b0402271904"
        "04f0f48eb204010004010004010004010004010104080000000000ea305504087015d289deaa32dd0401010408"
        "10fc7566d15cfd45040800000000a8ed32320402a1010481a110fc7566d15cfd4500000000000000001280a932"
        "d3e5a9d8351030555d4db7b23b10f0a42ed25cfd45206952ea2e413055204dba2a63693055104208c1386c3055"
        "e0b3bbb4656d3055500f9bee3975305590293dd37577305500118d472d833055202932c94c833055301b9a744e"
        "83305550cf55d3a888305570d5be0a239330558021a2b761b7305580af9134fbb830551029adee50dd3055e0b3"
        "dbe632ec305504010004200000000000000000000000000000000000000000000000000000000000000000";
#elif PROXY
    const char tx[] =
        "0420cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f0404a0a9495b0402271904"
        "04f0f48eb204010004010004010004010004010104080000000000ea305504087015d289deaa32dd0401010408"
        "10fc7566d15cfd45040800000000a8ed3232040111041110fc7566d15cfd4520fc7566d15cfd45000401000420"
        "0000000000000000000000000000000000000000000000000000000000000000";

#elif UPDATE_AUTH
    const char tx[] =
        "0420cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f0404a0a9495b0402271904"
        "04f0f48eb204010004010004010004010004010104080000000000ea305504080040cbdaa86c52d50401010408"
        "10fc7566d15cfd45040800000000a8ed32320402970104819710fc7566d15cfd4500000000a8ed323200000000"
        "80ab26a701000000020003b6d4fb38dba56d59623c5e2be38b0cdf63f7958cd61d27b1044271bb04cb63c70100"
        "000260520ba1782b60f9a658aff7b6d8536cf9088d509608bca5aae66dc171cba90301000250fc7566d15cfd45"
        "00000000a8ed3232010000000000000040380000000080ab26a70100022800000001000c000000010004010004"
        "200000000000000000000000000000000000000000000000000000000000000000";
#elif DELETE_AUTH
    const char tx[] =
        "0420cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f0404a0a9495b0402271904"
        "04f0f48eb204010004010004010004010004010104080000000000ea305504080040cbdaa8aca24a0401010408"
        "10fc7566d15cfd45040800000000a8ed3232040110041010fc7566d15cfd4500000000a8ed3232040100042000"
        "00000000000000000000000000000000000000000000000000000000000000";
#elif ARBITRARY
    const char tx[] =
        "0420cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f0404d0d3495b0402271904"
        "04f0f48eb20401000401000401000401000401010408003232374f8a285d0408000098d46564ae390401010408"
        "10fc7566d15cfd45040800000000a8ed3232040105040504746573740401000420000000000000000000000000"
        "0000000000000000000000000000000000000000";
#elif NEWACCOUNT
    const char tx[] =
        "0420cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f0404a0a9495b0402271904"
        "04f0f48eb204010004010004010004010004010304080000000000ea3055040800409e9a2264b89a0401010408"
        "10fc7566d15cfd45040800000000a8ed3232040166046610fc7566d15cfd450000f02a5e230f3d010000000100"
        "03b6d4fb38dba56d59623c5e2be38b0cdf63f7958cd61d27b1044271bb04cb63c70100000001000000010003b6"
        "d4fb38dba56d59623c5e2be38b0cdf63f7958cd61d27b1044271bb04cb63c70100000004080000000000ea3055"
        "040800b0cafe4873bd3e040101040810fc7566d15cfd45040800000000a8ed3232040114041410fc7566d15cfd"
        "450000f02a5e230f3d0010000004080000000000ea3055040800003f2a1ba6a24a040101040810fc7566d15cfd"
        "45040800000000a8ed3232040131043110fc7566d15cfd450000f02a5e230f3d102700000000000004454f5300"
        "000000881300000000000004454f53000000000104010004200000000000000000000000000000000000000000"
        "000000000000000000000000";
#else
    const char tx[] =
        "0420cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f0404a0a9495b0402271904"
        "04f0f48eb204010004010004010004010004010104080000000000ea30550408000000409a1ba3c20401010408"
        "10fc7566d15cfd45040800000000a8ed3232040110041010fc7566d15cfd450004000000000000040100042000"
        "00000000000000000000000000000000000000000000000000000000000000";
#endif

    uint8_t buffer[strlen(tx) / 2];
    hex_to_bytes(tx, strlen(tx), buffer, sizeof(buffer));

    initTxContext(&txProcessingCtx, &sha256, &sha256_arg, &txContent, 1);
    uint8_t status = parseTx(&txProcessingCtx, buffer, sizeof(buffer));

    do {
        if (status == STREAM_FAULT) {
            printf("FAILED");
            return 1;
        }

        if (status == STREAM_ACTION_READY) {
            onActionReady(&txProcessingCtx);
        }

        status = parseTx(&txProcessingCtx, buffer, sizeof(buffer));

    } while (status != STREAM_FINISHED);

    unsigned char digest[32] = {0};
    cx_hash(&sha256.header, CX_LAST, digest, 0, digest);
    printf("Digest: ");
    for (int i = 0; i < 32; i++) {
        printf("%x", digest[i]);
    }

    puts("");

    return 0;
}
