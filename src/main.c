/*******************************************************************************
 *   Taras Shchybovyk
 *   (c) 2018 Taras Shchybovyk
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include <stdbool.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "ledger_assert.h"
#include "offsets.h"
#include "errors.h"

#include "eos_utils.h"
#include "eos_stream.h"
#include "config.h"
#include "ui.h"
#include "main.h"

uint32_t get_public_key_and_set_result(void);
uint32_t sign_hash_and_set_result(void);

#define CLA                       0xD4
#define INS_GET_PUBLIC_KEY        0x02
#define INS_SIGN                  0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define P1_CONFIRM                0x01
#define P1_NON_CONFIRM            0x00
#define P2_NO_CHAINCODE           0x00
#define P2_CHAINCODE              0x01
#define P1_FIRST                  0x00
#define P1_MORE                   0x80

uint8_t const SECP256K1_N[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xfe, 0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48,
                               0xa0, 0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41};

cx_sha256_t sha256;
cx_sha256_t dataSha256;

txProcessingContext_t txProcessingCtx;
txProcessingContent_t txContent;
sharedContext_t tmpCtx;

static void io_exchange_with_code(uint16_t code, uint32_t tx) {
    G_io_apdu_buffer[tx++] = code >> 8;
    G_io_apdu_buffer[tx++] = code & 0xFF;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
}

unsigned int user_action_address_ok(void) {
    uint32_t tx = get_public_key_and_set_result();
    io_exchange_with_code(0x9000, tx);

    ui_display_public_key_done(true);
    return 0;
}

unsigned int user_action_address_cancel(void) {
    io_exchange_with_code(0x6985, 0);

    ui_display_public_key_done(false);
    return 0;
}

unsigned int user_action_tx_cancel(void) {
    io_exchange_with_code(0x6985, 0);

    ui_display_action_sign_done(STREAM_FINISHED, false);
    return 0;
}

void user_action_sign_flow_ok(void) {
    parserStatus_e txResult = parseTx(&txProcessingCtx, NULL, 0);
    switch (txResult) {
        case STREAM_ACTION_READY:
            ui_display_single_action_sign_flow();
            break;
        case STREAM_PROCESSING:
            io_exchange_with_code(0x9000, 0);
            ui_display_action_sign_done(STREAM_PROCESSING, true);
            break;
        case STREAM_FINISHED:
            io_exchange_with_code(0x9000, sign_hash_and_set_result());
            ui_display_action_sign_done(STREAM_FINISHED, true);
            break;
        default:
            io_exchange_with_code(0x6A80, 0);
            // Display back the original UX
            ui_idle();
            break;
    }
}

uint32_t get_public_key_and_set_result() {
    uint32_t tx = 0;
    G_io_apdu_buffer[tx++] = 65;
    memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.publicKey.W, 65);
    tx += 65;

    uint32_t addressLength = strlen(tmpCtx.publicKeyContext.address);

    G_io_apdu_buffer[tx++] = addressLength;
    memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.address, addressLength);
    tx += addressLength;
    if (tmpCtx.publicKeyContext.getChaincode) {
        memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.chainCode, 32);
        tx += 32;
    }
    return tx;
}

uint32_t handleGetPublicKey(uint8_t p1,
                            uint8_t p2,
                            uint8_t *dataBuffer,
                            uint16_t dataLength,
                            volatile unsigned int *flags,
                            volatile unsigned int *tx) {
    UNUSED(dataLength);
    uint8_t privateKeyData[64];
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint32_t i;
    uint8_t bip32PathLength = *(dataBuffer++);
    cx_ecfp_private_key_t privateKey;

    if ((bip32PathLength < 0x01) || (bip32PathLength > MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        return 0x6a80;
    }
    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        return 0x6B00;
    }
    if ((p2 != P2_CHAINCODE) && (p2 != P2_NO_CHAINCODE)) {
        return 0x6B00;
    }
    for (i = 0; i < bip32PathLength; i++) {
        bip32Path[i] =
            (dataBuffer[0] << 24) | (dataBuffer[1] << 16) | (dataBuffer[2] << 8) | (dataBuffer[3]);
        dataBuffer += 4;
    }
    tmpCtx.publicKeyContext.getChaincode = (p2 == P2_CHAINCODE);
    CX_ASSERT(os_derive_bip32_no_throw(
        CX_CURVE_256K1,
        bip32Path,
        bip32PathLength,
        privateKeyData,
        (tmpCtx.publicKeyContext.getChaincode ? tmpCtx.publicKeyContext.chainCode : NULL)));
    CX_ASSERT(cx_ecfp_init_private_key_no_throw(CX_CURVE_256K1, privateKeyData, 32, &privateKey));
    CX_ASSERT(cx_ecfp_generate_pair_no_throw(CX_CURVE_256K1,
                                             &tmpCtx.publicKeyContext.publicKey,
                                             &privateKey,
                                             1));
    memset(&privateKey, 0, sizeof(privateKey));
    memset(privateKeyData, 0, sizeof(privateKeyData));
    public_key_to_wif(tmpCtx.publicKeyContext.publicKey.W,
                      sizeof(tmpCtx.publicKeyContext.publicKey.W),
                      tmpCtx.publicKeyContext.address,
                      sizeof(tmpCtx.publicKeyContext.address));
    if (p1 == P1_NON_CONFIRM) {
        *tx = get_public_key_and_set_result();
    } else {
        ui_display_public_key_flow();

        *flags |= IO_ASYNCH_REPLY;
    }
    return SWO_SUCCESS;
}

uint32_t handleGetAppConfiguration(uint8_t p1,
                                   uint8_t p2,
                                   uint8_t *workBuffer,
                                   uint16_t dataLength,
                                   volatile unsigned int *flags,
                                   volatile unsigned int *tx) {
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(workBuffer);
    UNUSED(dataLength);
    UNUSED(flags);
    G_io_apdu_buffer[0] = (is_data_allowed() ? 0x01 : 0x00);
    G_io_apdu_buffer[1] = MAJOR_VERSION;
    G_io_apdu_buffer[2] = MINOR_VERSION;
    G_io_apdu_buffer[3] = PATCH_VERSION;
    *tx = 4;
    return SWO_SUCCESS;
}

uint32_t sign_hash_and_set_result(void) {
    // store hash
    CX_ASSERT(cx_hash_no_throw(&sha256.header,
                               CX_LAST,
                               tmpCtx.transactionContext.hash,
                               0,
                               tmpCtx.transactionContext.hash,
                               sizeof(tmpCtx.transactionContext.hash)));

    uint8_t privateKeyData[64];
    cx_ecfp_private_key_t privateKey;
    uint32_t tx = 0;
    uint8_t V[33];
    uint8_t K[32];
    int tries = 0;

    CX_ASSERT(os_derive_bip32_no_throw(CX_CURVE_256K1,
                                       tmpCtx.transactionContext.bip32Path,
                                       tmpCtx.transactionContext.pathLength,
                                       privateKeyData,
                                       NULL));
    CX_ASSERT(cx_ecfp_init_private_key_no_throw(CX_CURVE_256K1, privateKeyData, 32, &privateKey));
    memset(privateKeyData, 0, sizeof(privateKeyData));

    // Loop until a candidate matching the canonical signature is found

    for (;;) {
        if (tries == 0) {
            rng_rfc6979(G_io_apdu_buffer + 100,
                        tmpCtx.transactionContext.hash,
                        privateKey.d,
                        privateKey.d_len,
                        SECP256K1_N,
                        32,
                        V,
                        K);
        } else {
            rng_rfc6979(G_io_apdu_buffer + 100,
                        tmpCtx.transactionContext.hash,
                        NULL,
                        0,
                        SECP256K1_N,
                        32,
                        V,
                        K);
        }
        uint32_t infos;
        uint32_t sig_len = 100;
        CX_ASSERT(cx_ecdsa_sign_no_throw(&privateKey,
                                         CX_NO_CANONICAL | CX_RND_PROVIDED | CX_LAST,
                                         CX_SHA256,
                                         tmpCtx.transactionContext.hash,
                                         32,
                                         G_io_apdu_buffer + 100,
                                         &sig_len,
                                         &infos));
        if ((infos & CX_ECCINFO_PARITY_ODD) != 0) {
            G_io_apdu_buffer[100] |= 0x01;
        }
        G_io_apdu_buffer[0] = 27 + 4 + (G_io_apdu_buffer[100] & 0x01);
        ecdsa_der_to_sig(G_io_apdu_buffer + 100, G_io_apdu_buffer + 1);
        if (check_canonical(G_io_apdu_buffer + 1)) {
            tx = 1 + 64;
            break;
        } else {
            tries++;
        }
    }

    memset(&privateKey, 0, sizeof(privateKey));

    return tx;
}

uint32_t handleSign(uint8_t p1,
                    uint8_t p2,
                    uint8_t *workBuffer,
                    uint16_t dataLength,
                    volatile unsigned int *flags,
                    volatile unsigned int *tx) {
    uint32_t i;
    parserStatus_e txResult;
    if (p1 == P1_FIRST) {
        tmpCtx.transactionContext.pathLength = workBuffer[0];
        if ((tmpCtx.transactionContext.pathLength < 0x01) ||
            (tmpCtx.transactionContext.pathLength > MAX_BIP32_PATH)) {
            PRINTF("Invalid path\n");
            return 0x6a80;
        }
        workBuffer++;
        dataLength--;
        for (i = 0; i < tmpCtx.transactionContext.pathLength; i++) {
            tmpCtx.transactionContext.bip32Path[i] = (workBuffer[0] << 24) | (workBuffer[1] << 16) |
                                                     (workBuffer[2] << 8) | (workBuffer[3]);
            workBuffer += 4;
            dataLength -= 4;
        }
        initTxContext(&txProcessingCtx,
                      &sha256,
                      &dataSha256,
                      &txContent,
                      is_data_allowed() ? 0x01 : 0x00);
    } else if (p1 != P1_MORE) {
        return 0x6B00;
    }
    if (p2 != 0) {
        return 0x6B00;
    }
    if (txProcessingCtx.state == TLV_NONE) {
        PRINTF("Parser not initialized\n");
        return 0x6985;
    }

    txResult = parseTx(&txProcessingCtx, workBuffer, dataLength);
    switch (txResult) {
        case STREAM_CONFIRM_PROCESSING:
            ui_display_multiple_action_sign_flow();
            *flags |= IO_ASYNCH_REPLY;
            break;
        case STREAM_ACTION_READY:
            ui_display_single_action_sign_flow();
            *flags |= IO_ASYNCH_REPLY;
            break;
        case STREAM_FINISHED:
            *tx = sign_hash_and_set_result();
            break;
        case STREAM_PROCESSING:
            break;
        case STREAM_FAULT:
            return 0x6A80;
        default:
            PRINTF("Unexpected parser status\n");
            return 0x6A80;
    }
    return SWO_SUCCESS;
}

uint32_t handleApdu(volatile unsigned int *flags, volatile unsigned int *tx) {
    uint32_t sw = EXCEPTION;

    if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        return 0x6E00;
    }

    switch (G_io_apdu_buffer[OFFSET_INS]) {
        case INS_GET_PUBLIC_KEY:
            sw = handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1],
                                    G_io_apdu_buffer[OFFSET_P2],
                                    G_io_apdu_buffer + OFFSET_CDATA,
                                    G_io_apdu_buffer[OFFSET_LC],
                                    flags,
                                    tx);
            break;

        case INS_SIGN:
            sw = handleSign(G_io_apdu_buffer[OFFSET_P1],
                            G_io_apdu_buffer[OFFSET_P2],
                            G_io_apdu_buffer + OFFSET_CDATA,
                            G_io_apdu_buffer[OFFSET_LC],
                            flags,
                            tx);
            break;

        case INS_GET_APP_CONFIGURATION:
            sw = handleGetAppConfiguration(G_io_apdu_buffer[OFFSET_P1],
                                           G_io_apdu_buffer[OFFSET_P2],
                                           G_io_apdu_buffer + OFFSET_CDATA,
                                           G_io_apdu_buffer[OFFSET_LC],
                                           flags,
                                           tx);
            break;

        default:
            sw = 0x6D00;
            break;
    }
    return sw;
}

void app_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    config_init();
    ui_idle();

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile uint32_t sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0;  // ensure no race in catch_other if io_exchange throws
                         // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    sw = 0x6982;
                } else {
                    sw = handleApdu(&flags, &tx);
                }
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                    case EXCEPTION_IO_RESET:
                    case 0x6000:
                        // Wipe the transaction context and report the exception
                        sw = e;
                        break;
                    default:
                        // Internal error
                        sw = 0x6800 | (e & 0x7FF);
                        break;
                }
            }
            FINALLY {
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
        }
        END_TRY;
    }
}
