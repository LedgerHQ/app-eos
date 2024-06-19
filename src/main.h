/*****************************************************************************
 *   Ledger App EOS.
 *   (c) 2022 Ledger SAS.
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
 *****************************************************************************/
#include "eos_stream.h"

#define MAX_BIP32_PATH 10

typedef struct publicKeyContext_t {
    cx_ecfp_public_key_t publicKey;
    char address[60];
    uint8_t chainCode[32];
    bool getChaincode;
} publicKeyContext_t;

typedef struct transactionContext_t {
    uint8_t pathLength;
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint8_t hash[32];
} transactionContext_t;

typedef union sharedContext_t {
    publicKeyContext_t publicKeyContext;
    transactionContext_t transactionContext;
} sharedContext_t;

extern txProcessingContext_t txProcessingCtx;
extern txProcessingContent_t txContent;
extern sharedContext_t tmpCtx;

unsigned int user_action_tx_cancel(void);
unsigned int user_action_address_ok(void);
unsigned int user_action_address_cancel(void);
void user_action_sign_flow_ok(void);
