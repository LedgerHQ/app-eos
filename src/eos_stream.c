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

#include <string.h>
#include "eos_stream.h"
#include "os.h"
#include "cx.h"
#include "eos_types.h"
#include "eos_utils.h"
#include "eos_parse.h"
#include "eos_parse_token.h"
#include "eos_parse_eosio.h"
#include "eos_parse_unknown.h"
#include "state_neutral.h"

/* CONTRACT OWNERS */
#define CORE_VAULTA 0x452EA06CDA8E4C00
#define EOSIO       0x5530EA0000000000
#define EOSIO_TOKEN 0x5530EA033482A600
/* Allow no-op signing requests from trusted account */
#define NULL_VAULTA 0x9EA3106CDA8E4C00

/* ACTIONS */
#define TOKEN_TRANSFER_ACTION 0xCDCD3C2D57000000
#define VAULTA_SWAPTO_ACTION  0xC70D5CD000000000

#define DELEGATEBW_ACTION   0x4AA2A61B2A3F0000
#define UNDELEGATEBW_ACTION 0xD4D2A8A986CA8FC0
#define VOTEPRODUCER_ACTION 0xDD32AADE89D21570
#define BUYRAM_ACTION       0x3EBD734800000000

#define BUYRAM_ACTIONBYTES 0x3EBD7348FECAB000
#define SELLRAM_ACTION     0xC2A31B9A40000000
#define UPDATE_AUTH_ACTION 0xD5526CA8DACB4000
#define DELETE_AUTH_ACTION 0x4AA2ACA8DACB4000
#define REFUND_ACTION      0xBA97A9A400000000
#define LINK_AUTH_ACTION   0x8BA7036B2D000000
#define UNLINK_AUTH_ACTION 0xD4E2E9C0DACB4000
#define NEW_ACCOUNT_ACTION 0x9AB864229A9E4000
#define NOOP_ACTION        0x9D29500000000000
#define IDENTITY           0x72553CBB3E000000

void initTxContext(txProcessingContext_t *context,
                   cx_sha256_t *sha256,
                   cx_sha256_t *dataSha256,
                   txProcessingContent_t *processingContent,
                   uint8_t allowUnknownAction,
                   uint8_t verboseSetting) {
    memset(context, 0, sizeof(txProcessingContext_t));
    context->sha256 = sha256;
    context->dataSha256 = dataSha256;
    context->content = processingContent;
    context->state = TLV_CHAIN_ID;
    context->unknownActionAllowed = allowUnknownAction;
    context->isVerbose = verboseSetting;
    context->content->noData = 0;
    cx_sha256_init(context->sha256);
    cx_sha256_init(context->dataSha256);
}

uint8_t readTxByte(txProcessingContext_t *context) {
    uint8_t data;
    LEDGER_ASSERT(context->commandLength >= 1, "readTxByte Underflow");
    data = *context->workBuffer;
    context->workBuffer++;
    context->commandLength--;
    return data;
}

static void processTokenTransfer(txProcessingContext_t *context) {
    context->content->argumentCount = 3;
    uint32_t bufferLength = context->currentActionDataBufferLength;
    uint8_t *buffer = context->actionDataBuffer;

    buffer += 2 * sizeof(name_t) + sizeof(asset_t);
    bufferLength -= 2 * sizeof(name_t) + sizeof(asset_t);
    uint32_t memoLength = 0;
    unpack_variant32(buffer, bufferLength, &memoLength);
    if (memoLength > 0) {
        context->content->argumentCount++;
    }
}

static void processNoOperation(txProcessingContext_t *context) {
    context->content->argumentCount = 1;
}

static void processEosioDelegate(txProcessingContext_t *context) {
    context->content->argumentCount = 4;
    uint8_t *buffer = context->actionDataBuffer;

    buffer += 2 * sizeof(name_t) + 2 * sizeof(asset_t);

    if (buffer[0] != 0) {
        context->content->argumentCount += 1;
    }
}

static void processEosioUndelegate(txProcessingContext_t *context) {
    context->content->argumentCount = 4;
}

static void processEosioRefund(txProcessingContext_t *context) {
    context->content->argumentCount = 1;
}

static void processEosioBuyRam(txProcessingContext_t *context) {
    context->content->argumentCount = 3;
}

static void processEosioSellRam(txProcessingContext_t *context) {
    context->content->argumentCount = 2;
}

static void processEosioVoteProducer(txProcessingContext_t *context) {
    uint32_t bufferLength = context->currentActionDataBufferLength;
    uint8_t *buffer = context->actionDataBuffer;

    context->content->argumentCount = 1;
    buffer += sizeof(name_t);

    name_t proxy = 0;
    memmove(&proxy, buffer, sizeof(name_t));
    if (proxy != 0) {
        context->content->argumentCount++;
        return;
    }
    buffer += sizeof(name_t);

    uint32_t totalProducers = 0;
    unpack_variant32(buffer, bufferLength, &totalProducers);
    context->content->argumentCount += totalProducers;
}

static void processEosioUpdateAuth(txProcessingContext_t *context) {
    uint32_t bufferLength = context->currentActionDataBufferLength;
    uint8_t *buffer = context->actionDataBuffer;

    context->content->argumentCount = 4;

    uint32_t offset_update = 3 * sizeof(name_t) + sizeof(uint32_t);

    LEDGER_ASSERT(offset_update < bufferLength, "processEosioUpdateAuth");
    buffer += offset_update;
    bufferLength -= offset_update;

    uint32_t totalKeys = 0;
    uint32_t read = unpack_variant32(buffer, bufferLength, &totalKeys);

    LEDGER_ASSERT(read < bufferLength, "processEosioUpdateAuth");
    buffer += read;
    bufferLength -= read;

    offset_update = 1 + sizeof(public_key_t) + sizeof(uint16_t);
    LEDGER_ASSERT(!__builtin_mul_overflow(offset_update, totalKeys, &offset_update),
                  "processEosioUpdateAuth");

    LEDGER_ASSERT(offset_update < bufferLength, "processEosioUpdateAuth");
    buffer += offset_update;
    bufferLength -= offset_update;

    LEDGER_ASSERT(!__builtin_add_overflow(totalKeys, totalKeys, &totalKeys),
                  "processEosioUpdateAuth");  // totalKeys *= 2
    LEDGER_ASSERT(!__builtin_add_overflow(context->content->argumentCount,
                                          totalKeys,
                                          &context->content->argumentCount),
                  "processEosioUpdateAuth");

    uint32_t totalAccounts = 0;
    read = unpack_variant32(buffer, bufferLength, &totalAccounts);

    // accounts data begins here
    LEDGER_ASSERT(read < bufferLength, "processEosioUpdateAuth");
    buffer += read;
    bufferLength -= read;

    offset_update = sizeof(permisssion_level_t) + sizeof(uint16_t);
    LEDGER_ASSERT(!__builtin_mul_overflow(offset_update, totalAccounts, &offset_update),
                  "processEosioUpdateAuth");

    LEDGER_ASSERT(offset_update < bufferLength, "processEosioUpdateAuth");
    buffer += offset_update;
    bufferLength -= offset_update;

    LEDGER_ASSERT(!__builtin_add_overflow(totalAccounts, totalAccounts, &totalAccounts),
                  "processEosioUpdateAuth");  // totalAccounts *= 2
    LEDGER_ASSERT(!__builtin_add_overflow(context->content->argumentCount,
                                          totalAccounts,
                                          &context->content->argumentCount),
                  "processEosioUpdateAuth");

    uint32_t totalWaits = 0;
    unpack_variant32(buffer, bufferLength, &totalWaits);

    LEDGER_ASSERT(!__builtin_add_overflow(totalWaits, totalWaits, &totalWaits),
                  "processEosioUpdateAuth");  // totalWaits *= 2
    LEDGER_ASSERT(!__builtin_add_overflow(context->content->argumentCount,
                                          totalWaits,
                                          &context->content->argumentCount),
                  "processEosioUpdateAuth");
}

static void processEosioDeleteAuth(txProcessingContext_t *context) {
    context->content->argumentCount = 2;
}

static void processEosioLinkAuth(txProcessingContext_t *context) {
    context->content->argumentCount = 4;
}

static void processEosioUnlinkAuth(txProcessingContext_t *context) {
    context->content->argumentCount = 3;
}

static void processUnknownAction(txProcessingContext_t *context) {
    CX_ASSERT(cx_hash_no_throw(&context->dataSha256->header,
                               CX_LAST,
                               context->dataChecksum,
                               0,
                               context->dataChecksum,
                               sizeof(context->dataChecksum)));
    // if verbose ON argument count of 3 to trigger checksum screens
    // if verbose OFF argument count of 1 to only showing single warning screen
    if (context->isVerbose == 1) {
        context->content->argumentCount = 3;
    } else {
        context->content->argumentCount = 1;
    }
}

static void processEosioNewAccountAction(txProcessingContext_t *context) {
    uint8_t *buffer = context->actionDataBuffer;
    uint32_t bufferLength = context->currentActionDataBufferLength;

    // Offset to auth structures: owner, active
    buffer += 2 * sizeof(name_t);
    bufferLength -= 2 * sizeof(name_t);

    // Read owner key threshold
    uint32_t threshold = 0;
    memmove(&threshold, buffer, sizeof(threshold));
    LEDGER_ASSERT(threshold == 1, "Owner Threshold should be 1");
    buffer += sizeof(threshold);
    bufferLength -= sizeof(threshold);

    uint32_t size = 0;
    uint32_t read = unpack_variant32(buffer, bufferLength, &size);
    LEDGER_ASSERT(size == 1, "Owner key must be 1");
    buffer += read;
    bufferLength -= read;
    // Offset to key weight
    buffer += 1 + sizeof(public_key_t);
    bufferLength -= 1 + sizeof(public_key_t);
    uint16_t weight = 0;
    memmove(&weight, buffer, sizeof(weight));
    LEDGER_ASSERT(weight == 1, "Owner key weight must be 1");
    buffer += sizeof(weight);
    bufferLength -= sizeof(weight);

    read = unpack_variant32(buffer, bufferLength, &size);
    LEDGER_ASSERT(size == 0, "No accounts allowed");
    buffer += read;
    bufferLength -= read;
    read = unpack_variant32(buffer, bufferLength, &size);
    LEDGER_ASSERT(size == 0, "No delays allowed");
    buffer += read;
    bufferLength -= read;

    // process Active authorization
    // -----------------------------------

    memmove(&threshold, buffer, sizeof(threshold));
    LEDGER_ASSERT(threshold == 1, "Active Threshold should be 1");
    buffer += sizeof(threshold);
    bufferLength -= sizeof(threshold);

    read = unpack_variant32(buffer, bufferLength, &size);
    LEDGER_ASSERT(size == 1, "Active key must be 1");
    buffer += read;
    bufferLength -= read;
    // Offset to key weight
    buffer += 1 + sizeof(public_key_t);
    bufferLength -= 1 + sizeof(public_key_t);
    weight = 0;
    memmove(&weight, buffer, sizeof(weight));
    LEDGER_ASSERT(weight == 1, "Active key weight must be 1");
    buffer += sizeof(weight);
    bufferLength -= sizeof(weight);

    read = unpack_variant32(buffer, bufferLength, &size);
    LEDGER_ASSERT(size == 0, "No accounts allowed");
    buffer += read;
    bufferLength -= read;
    unpack_variant32(buffer, bufferLength, &size);
    LEDGER_ASSERT(size == 0, "No delays allowed");
    context->content->argumentCount = 4;
}

void printArgument(uint8_t argNum, txProcessingContext_t *context) {
    name_t contractName = context->contractName;
    name_t actionName = context->contractActionName;
    uint8_t *buffer = context->actionDataBuffer;
    uint32_t bufferLength = context->currentActionDataBufferLength;
    actionArgument_t *arg = &context->content->arg;

    if (actionName == VAULTA_SWAPTO_ACTION && contractName == CORE_VAULTA) {
        parseTokenTransfer(buffer, bufferLength, argNum, arg);
        return;
    }

    /* *
     * Actions from trusted account do not change on-chain state
     * These actions are used to set authorization for future on-chain transactions
     * only 2 actions null::vaulta and 0x00::identity
     * */
    if ((actionName == NOOP_ACTION && contractName == NULL_VAULTA) ||
        (actionName == IDENTITY && contractName == 0x00)) {
        parseNoOperation(bufferLength, arg);
        return;
    }

    if (actionName == TOKEN_TRANSFER_ACTION &&
        (contractName == EOSIO_TOKEN || contractName == CORE_VAULTA)) {
        parseTokenTransfer(buffer, bufferLength, argNum, arg);
        return;
    }

    if (contractName == EOSIO || contractName == CORE_VAULTA) {
        switch (actionName) {
            case DELEGATEBW_ACTION:
                parseDelegate(buffer, bufferLength, argNum, arg);
                break;
            case UNDELEGATEBW_ACTION:
                parseUndelegate(buffer, bufferLength, argNum, arg);
                break;
            case REFUND_ACTION:
                parseRefund(buffer, bufferLength, argNum, arg);
                break;
            case BUYRAM_ACTION:
                parseBuyRam(buffer, bufferLength, argNum, arg);
                break;
            case BUYRAM_ACTIONBYTES:
                parseBuyRamBytes(buffer, bufferLength, argNum, arg);
                break;
            case SELLRAM_ACTION:
                parseSellRam(buffer, bufferLength, argNum, arg);
                break;
            case VOTEPRODUCER_ACTION:
                parseVoteProducer(buffer, bufferLength, argNum, arg);
                break;
            case UPDATE_AUTH_ACTION:
                parseUpdateAuth(buffer, bufferLength, argNum, arg);
                break;
            case DELETE_AUTH_ACTION:
                parseDeleteAuth(buffer, bufferLength, argNum, arg);
                break;
            case LINK_AUTH_ACTION:
                parseLinkAuth(buffer, bufferLength, argNum, arg);
                break;
            case UNLINK_AUTH_ACTION:
                parseUnlinkAuth(buffer, bufferLength, argNum, arg);
                break;
            case NEW_ACCOUNT_ACTION:
                parseNewAccount(buffer, bufferLength, argNum, arg);
                break;
            default:
                if (context->unknownActionAllowed == 1) {
                    parseUnknownAction(context->dataChecksum,
                                       sizeof(context->dataChecksum),
                                       argNum,
                                       arg);
                }
        }
        return;
    }

    if (context->unknownActionAllowed == 1) {
        parseUnknownAction(context->dataChecksum, sizeof(context->dataChecksum), argNum, arg);
    }
}

static bool isKnownAction(txProcessingContext_t *context) {
    name_t contractName = context->contractName;
    name_t actionName = context->contractActionName;

    if (actionName == VAULTA_SWAPTO_ACTION && contractName == CORE_VAULTA) {
        return true;
    }

    /* *
     * Actions from trusted account do not change on-chain state
     * These actions are used to set authorization for future on-chain transactions
     * */
    if ((actionName == NOOP_ACTION && contractName == NULL_VAULTA) ||
        (actionName == IDENTITY && contractName == 0x00)) {
        return true;
    }

    if (actionName == TOKEN_TRANSFER_ACTION &&
        (contractName == EOSIO_TOKEN || contractName == CORE_VAULTA)) {
        return true;
    }

    if (contractName == EOSIO || contractName == CORE_VAULTA) {
        switch (actionName) {
            case DELEGATEBW_ACTION:
            case UNDELEGATEBW_ACTION:
            case REFUND_ACTION:
            case BUYRAM_ACTION:
            case BUYRAM_ACTIONBYTES:
            case SELLRAM_ACTION:
            case VOTEPRODUCER_ACTION:
            case UPDATE_AUTH_ACTION:
            case DELETE_AUTH_ACTION:
            case LINK_AUTH_ACTION:
            case UNLINK_AUTH_ACTION:
            case NEW_ACCOUNT_ACTION:
                return true;
        }
    }
    return false;
}

/**
 * Sequentially hash an incoming data.
 * Hash functionality is moved out here in order to reduce
 * dependencies on specific hash implementation.
 */
static void hashTxData(txProcessingContext_t *context, uint8_t *buffer, uint32_t length) {
    CX_ASSERT(cx_hash_no_throw(&context->sha256->header, 0, buffer, length, NULL, 0));
}

static void hashActionData(txProcessingContext_t *context, uint8_t *buffer, uint32_t length) {
    CX_ASSERT(cx_hash_no_throw(&context->dataSha256->header, 0, buffer, length, NULL, 0));
}

/**
 * Process all fields that do not require any processing except hashing.
 * The data comes in by chucks, so it may happen that buffer may contain
 * incomplete data for particular field. Function designed to process
 * everything until it receives all data for a particular field
 * and after that will move to next field.
 */
static void processField(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processField");
        hashTxData(context, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->state++;
        context->processingField = false;
    }
}

/**
 * Process Size fields that are expected to have Zero value. Except hashing the data, function
 * caches an incommng data. So, when all bytes for particular field are received
 * do additional processing: Read actual number of actions encoded in buffer.
 * Throw exception if number is not '0'.
 */
static void processZeroSizeField(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processZeroSizeField");
        hashTxData(context, context->workBuffer, length);

        // Store data into a buffer
        LEDGER_ASSERT(length <= sizeof(context->sizeBuffer) - context->currentFieldPos,
                      "processZeroSizeField");
        memmove(context->sizeBuffer + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        uint32_t sizeValue = 0;
        unpack_variant32(context->sizeBuffer, context->currentFieldPos + 1, &sizeValue);
        LEDGER_ASSERT(sizeValue == 0, "processCtxFreeAction Action Number must be 0");
        // Reset size buffer
        memset(context->sizeBuffer, 0, sizeof(context->sizeBuffer));

        // Move to next state
        context->state++;
        context->processingField = false;
    }
}

/**
 * Process Action Number Field. Except hashing the data, function
 * caches an incoming data. So, when all bytes for particular field are received
 * do additional processing: Read actual number of actions encoded in buffer.
 */
static void processActionListSizeField(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processActionListSizeField");
        hashTxData(context, context->workBuffer, length);

        // Store data into a buffer
        LEDGER_ASSERT(length <= sizeof(context->sizeBuffer) - context->currentFieldPos,
                      "processActionListSizeField");
        memmove(context->sizeBuffer + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        unpack_variant32(context->sizeBuffer,
                         context->currentFieldPos + 1,
                         &context->currentActionNumber);
        context->currentActionIndex = 0;

        // Reset size buffer
        memset(context->sizeBuffer, 0, sizeof(context->sizeBuffer));

        context->state++;
        context->processingField = false;
        if (context->currentActionNumber > 1) {
            context->confirmProcessing = true;
        }
    }
}

/**
 * Process Action Account Field. Cache a data of the field in order to
 * display it for validation.
 */
static void processActionAccount(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processActionAccount");
        hashTxData(context, context->workBuffer, length);

        uint8_t *pContract = (uint8_t *) &context->contractName;

        LEDGER_ASSERT(length <= sizeof(context->sizeBuffer) - context->currentFieldPos,
                      "processActionAccount");
        memmove(pContract + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->state++;
        context->processingField = false;

        memset(context->content->contract, 0, sizeof(context->content->contract));
        name_to_string(context->contractName,
                       context->content->contract,
                       sizeof(context->content->contract));
    }
}

/**
 * Process Action Name Field. Cache a data of the field in order to
 * display it for validation.
 */
static void processActionName(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processActionName");
        hashTxData(context, context->workBuffer, length);

        uint8_t *pAction = (uint8_t *) &context->contractActionName;
        LEDGER_ASSERT(length <= sizeof(context->sizeBuffer) - context->currentFieldPos,
                      "processActionName");
        memmove(pAction + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->state++;
        context->processingField = false;

        memset(context->content->action, 0, sizeof(context->content->action));
        name_to_string(context->contractActionName,
                       context->content->action,
                       sizeof(context->content->action));
    }
}

/**
 * Process Authorization Number Field. Initialize context action number
 * index and context action number.
 */
static void processAuthorizationListSizeField(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processAuthorizationListSizeField");
        hashTxData(context, context->workBuffer, length);

        // Store data into a buffer
        LEDGER_ASSERT(length <= sizeof(context->sizeBuffer) - context->currentFieldPos,
                      "processAuthorizationListSizeField");
        memmove(context->sizeBuffer + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        unpack_variant32(context->sizeBuffer,
                         context->currentFieldPos + 1,
                         &context->currentAutorizationNumber);
        context->currentAutorizationIndex = 0;
        // Reset size buffer
        memset(context->sizeBuffer, 0, sizeof(context->sizeBuffer));

        // Move to next state
        context->state++;
        context->processingField = false;
    }
}

/**
 * Process Authorization Account Name Field.
 */
static void processAuthorizationAccount(txProcessingContext_t *context) {
    // hold uint representing authorization name
    name_t authorizationName = 0;

    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processField");
        hashTxData(context, context->workBuffer, length);

        uint8_t *pAuthName = (uint8_t *) &authorizationName;
        LEDGER_ASSERT(length <= sizeof(context->sizeBuffer) - context->currentFieldPos,
                      "processAuthorizationPermission");
        memmove(pAuthName + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->state++;
        context->processingField = false;

        memset(context->currentAuthorizationName, 0, sizeof(context->currentAuthorizationName));
        name_to_string(authorizationName,
                       context->currentAuthorizationName,
                       sizeof(context->currentAuthorizationName));
    }
}

/**
 * Process Authorization Permission Field. When the field is processed
 * start over authorization processing if the there is data for that.
 */
static void processAuthorizationPermission(txProcessingContext_t *context) {
    // hold uint representing authorization permission
    name_t authorizationPermission = 0;

    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processAuthorizationPermission");
        hashTxData(context, context->workBuffer, length);

        uint8_t *pAuthPerms = (uint8_t *) &authorizationPermission;
        LEDGER_ASSERT(length <= sizeof(context->sizeBuffer) - context->currentFieldPos,
                      "processAuthorizationPermission");
        memmove(pAuthPerms + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentAutorizationIndex++;
        // Reset size buffer
        memset(context->sizeBuffer, 0, sizeof(context->sizeBuffer));

        // Start over reading Authorization data or move to the next state
        // if all authorization data have been read
        if (context->currentAutorizationIndex != context->currentAutorizationNumber) {
            context->state = TLV_AUTHORIZATION_ACTOR;
        } else {
            context->state++;
        }
        context->processingField = false;

        memset(context->currentAuthorizationPermission,
               0,
               sizeof(context->currentAuthorizationPermission));
        name_to_string(authorizationPermission,
                       context->currentAuthorizationPermission,
                       sizeof(context->currentAuthorizationPermission));
    }
}

static void processUnknownActionDataSize(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processUnknownActionDataSize");
        hashTxData(context, context->workBuffer, length);
        hashActionData(context, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->state++;
        context->processingField = false;
    }
}

/**
 * Process current unknown action data field and calculate checksum.
 */
static void processUnknownActionData(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        LEDGER_ASSERT(length <= context->commandLength, "processUnknownActionData");
        hashTxData(context, context->workBuffer, length);
        hashActionData(context, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentActionDataBufferLength = context->currentFieldLength;

        processUnknownAction(context);

        if (++context->currentActionIndex < context->currentActionNumber) {
            context->state = TLV_ACTION_ACCOUNT;
        } else {
            context->state = TLV_TX_EXTENSION_LIST_SIZE;
        }

        context->processingField = false;
        context->actionReady = true;

        cx_sha256_init(context->dataSha256);
    }
}

/**
 * Process current action data field and store in into data buffer.
 */
static void processActionData(txProcessingContext_t *context) {
    LEDGER_ASSERT(context->currentFieldLength <= sizeof(context->actionDataBuffer) - 1,
                  "processActionData data overflow");

    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length =
            (context->commandLength < ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);

        hashTxData(context, context->workBuffer, length);
        memmove(context->actionDataBuffer + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentActionDataBufferLength = context->currentFieldLength;

        if (context->contractActionName == VAULTA_SWAPTO_ACTION &&
            context->contractName == CORE_VAULTA) {
            processTokenTransfer(context);

            // no args or data expected
        } else if ((context->contractActionName == NOOP_ACTION &&
                    context->contractName == NULL_VAULTA) ||
                   (context->contractActionName == IDENTITY && context->contractName == 0x00)) {
            processNoOperation(context);

        } else if (context->contractActionName == TOKEN_TRANSFER_ACTION &&
                   (context->contractName == EOSIO_TOKEN || context->contractName == CORE_VAULTA)) {
            processTokenTransfer(context);

        } else if (context->contractName == EOSIO || context->contractName == CORE_VAULTA) {
            switch (context->contractActionName) {
                case DELEGATEBW_ACTION:
                    processEosioDelegate(context);
                    break;
                case UNDELEGATEBW_ACTION:
                    processEosioUndelegate(context);
                    break;
                case REFUND_ACTION:
                    processEosioRefund(context);
                    break;
                case VOTEPRODUCER_ACTION:
                    processEosioVoteProducer(context);
                    break;
                case BUYRAM_ACTION:
                case BUYRAM_ACTIONBYTES:
                    processEosioBuyRam(context);
                    break;
                case SELLRAM_ACTION:
                    processEosioSellRam(context);
                    break;
                case UPDATE_AUTH_ACTION:
                    processEosioUpdateAuth(context);
                    break;
                case DELETE_AUTH_ACTION:
                    processEosioDeleteAuth(context);
                    break;
                case LINK_AUTH_ACTION:
                    processEosioLinkAuth(context);
                    break;
                case UNLINK_AUTH_ACTION:
                    processEosioUnlinkAuth(context);
                    break;
                case NEW_ACCOUNT_ACTION:
                    processEosioNewAccountAction(context);
                    break;
                default:
                    LEDGER_ASSERT(false, "processActionData");
            }
        }

        if (++context->currentActionIndex < context->currentActionNumber) {
            context->state = TLV_ACTION_ACCOUNT;
        } else {
            context->state = TLV_TX_EXTENSION_LIST_SIZE;
        }
        context->processingField = false;
        context->actionReady = true;
    }
}

static parserStatus_e processTxInternal(txProcessingContext_t *context) {
    for (;;) {
        // this is multi action processing
        if (context->confirmProcessing) {
            context->confirmProcessing = false;
            return STREAM_CONFIRM_PROCESSING;
        }
        // this is single action only
        if (context->actionReady) {
            context->actionReady = false;
            return STREAM_ACTION_READY;
        }
        if (context->state == TLV_DONE) {
            return STREAM_FINISHED;
        }
        if (context->commandLength == 0) {
            return STREAM_PROCESSING;
        }
        if (!context->processingField) {
            // While we are not processing a field, we should TLV parameters
            bool decoded = false;
            while (context->commandLength != 0) {
                bool valid;
                // Feed the TLV buffer until the length can be decoded
                context->tlvBuffer[context->tlvBufferPos++] = readTxByte(context);

                decoded = tlvTryDecode(context->tlvBuffer,
                                       context->tlvBufferPos,
                                       &context->currentFieldLength,
                                       &valid);

                if (!valid) {
                    PRINTF("TLV decoding error\n");
                    return STREAM_FAULT;
                }
                if (decoded) {
                    break;
                }

                // Cannot decode yet
                // Sanity check
                if (context->tlvBufferPos == sizeof(context->tlvBuffer)) {
                    PRINTF("TLV pre-decode logic error\n");
                    return STREAM_FAULT;
                }
            }
            if (!decoded) {
                return STREAM_PROCESSING;
            }
            context->currentFieldPos = 0;
            context->tlvBufferPos = 0;
            context->processingField = true;
            context->content->noData = (context->currentFieldLength == 0);
        }
        switch (context->state) {
            case TLV_CHAIN_ID:
            case TLV_HEADER_EXPITATION:
            case TLV_HEADER_REF_BLOCK_NUM:
            case TLV_HEADER_REF_BLOCK_PREFIX:
            case TLV_HEADER_MAX_CPU_USAGE_MS:
            case TLV_HEADER_MAX_NET_USAGE_WORDS:
            case TLV_HEADER_DELAY_SEC:
                processField(context);
                break;

            case TLV_CFA_LIST_SIZE:
                processZeroSizeField(context);
                break;

            case TLV_ACTION_LIST_SIZE:
                processActionListSizeField(context);
                break;

            case TLV_ACTION_ACCOUNT:
                processActionAccount(context);
                break;

            case TLV_ACTION_NAME:
                processActionName(context);
                break;

            case TLV_AUTHORIZATION_LIST_SIZE:
                processAuthorizationListSizeField(context);
                break;

            case TLV_AUTHORIZATION_ACTOR:
                processAuthorizationAccount(context);
                break;

            case TLV_AUTHORIZATION_PERMISSION:
                processAuthorizationPermission(context);
                break;

            case TLV_ACTION_DATA_SIZE:
                if (isKnownAction(context) || context->unknownActionAllowed != 1) {
                    processField(context);
                } else {
                    processUnknownActionDataSize(context);
                }
                break;

            case TLV_ACTION_DATA:
                if (isKnownAction(context)) {
                    processActionData(context);
                } else if (context->unknownActionAllowed == 1) {
                    processUnknownActionData(context);
                } else {
                    PRINTF("UNKNOWN ACTION");
                    return STREAM_NOT_ALLOWED;
                }
                break;

            case TLV_TX_EXTENSION_LIST_SIZE:
                processZeroSizeField(context);
                break;

            case TLV_CONTEXT_FREE_DATA:
                processField(context);
                break;

            default:
                PRINTF("Invalid TLV decoder context\n");
                return STREAM_FAULT;
        }
    }
}

/**
 * Transaction processing should be done in a most efficient
 * way as possible, as EOS transaction size isn't fixed
 * and depends on action size.
 * Also, Ledger Nano S have limited RAM resource, so data caching
 * could be very expensive. Due to these features and limitations
 * only some fields are cached before processing.
 * All data is encoded by DER.ASN1 rules in plain way and serialized as a flat map.
 *
 * Flat map is used in order to avoid nesting complexity.
 *
 * Buffer serialization example:
 * [chain id][header][action number (1)][action 0][...]
 * Each field is encoded by DER rules.
 * Chain id field will be encoded next way:
 *  [Tag][Length][Value]
 * [0x04][ 0x20 ][chain id as octet string]
 *
 * More information about DER Tag Length Value encoding is here:
 * http://luca.ntop.org/Teaching/Appunti/asn1.html. Only octet tag number is allowed. Value is
 * encoded as octet string. The length of the string is stored in Length byte(s)
 *
 * Detailed flat map representation of incoming data:
 * [CHAIN ID][HEADER][CTX_FREE_ACTION_NUMBER][ACTION_NUMBER][ACTION
 * 0][TX_EXTENSION_NUMBER][CTX_FREE_ACTION_DATA_NUMBER]
 *
 * CHAIN ID:
 * [32 BYTES]
 *
 * HEADER size may vary due to MAX_NET_USAGE_WORDS and DELAY_SEC serialization:
 * [EXPIRATION][REF_BLOCK_NUM][REF_BLOCK_PREFIX][MAX_NET_USAGE_WORDS][MAX_CPU_USAGE_MS][DELAY_SEC]
 *
 * CTX_FREE_ACTION_NUMBER theoretically is not fixed due to serialization. Ledger accepts only 0
 * as encoded value. ACTION_NUMBER theoretically is not fixed due to serialization.
 *
 * ACTION size may vary as authorization list and action data is dynamic:
 * [ACCOUNT][NAME][AUTHORIZATION_NUMBER][AUTHORIZATION 0][AUTHORIZATION 1]..[AUTHORIZATION
 * N][ACTION_DATA] ACCOUNT and NAME are 8 bytes long, both. AUTHORIZATION_NUMBER theoretically
 * is not fixed due to serialization. ACTION_DATA is octet string of bytes.
 *
 * AUTHORIZATION is 16 bytes long:
 * [ACTOR][PERMISSION]
 * ACTOR and PERMISSION are 8 bites long, both.
 *
 * TX_EXTENSION_NUMBER theoretically is not fixed due to serialization. Ledger accepts only 0 as
 * encoded value. CTX_FREE_ACTION_DATA_NUMBER theoretically is not fixed due to serialization.
 * Ledger accepts only 0 as encoded value.
 */
parserStatus_e parseTx(txProcessingContext_t *context, uint8_t *buffer, uint32_t length) {
#ifdef DEBUG_APP
    // Do not catch exceptions.
    context->workBuffer = buffer;
    context->commandLength = length;
#else
    if (context->commandLength == 0) {
        context->workBuffer = buffer;
        context->commandLength = length;
    }
#endif
    return processTxInternal(context);
}

/**
 * preparseTransaction
 *
 * Pre-processing loop on a transaction to count the number of state-neutral actions.
 * This count is needed for the UI to handle transactions properly.
 *
 * @param workBuffer Pointer to the transaction data buffer.
 * @param scratchLength Length of the transaction data buffer.
 * @param verbose Verbose flag to control processing detail.
 * @return The count of state-neutral actions in the transaction.
 */
unsigned int preparseTransaction(uint8_t *workBuffer, uint16_t scratchLength, uint8_t verbose) {
    txProcessingContext_t localCtx;
    txProcessingContent_t localContent;
    cx_sha256_t sha256, dataSha256;

    // copy work buffer
    uint8_t scratch[scratchLength];
    memcpy(scratch, workBuffer, scratchLength);

    // initialize a local context (stack only, no side-effects)
    initTxContext(&localCtx,
                  &sha256,
                  &dataSha256,
                  &localContent,
                  1,         // allowUnknownAction
                  verbose);  // verbose off

    localCtx.workBuffer = scratch;
    localCtx.commandLength = scratchLength;

    unsigned int count = 0;

    // process until transaction finished or buffer consumed
    for (;;) {
        parserStatus_e st = processTxInternal(&localCtx);
        if (st == STREAM_ACTION_READY) {
            // We have a fully decoded action
            name_t contract = localCtx.contractName;
            name_t action = localCtx.contractActionName;
            char contractOwner[sizeof(localCtx.content->contract)];
            char contractActionName[sizeof(localCtx.content->action)];
            name_to_string(contract, contractOwner, sizeof(localCtx.content->contract));
            name_to_string(action, contractActionName, sizeof(localCtx.content->action));

            if (!verbose && (isStateNeutralAction(contractOwner,
                                                  contractActionName,
                                                  localCtx.content->noData))) {
                count++;
            }
        } else if (st == STREAM_FINISHED || st == STREAM_FAULT || st == STREAM_NOT_ALLOWED) {
            break;
        } else {
            // STREAM_PROCESSING or STREAM_CONFIRM_PROCESSING: continue loop
            if (localCtx.commandLength == 0) {
                // No more data to feed
                break;
            }
        }
    }
    // clear out scratch for extra security
    memset(scratch, 0, sizeof(scratch));

    return count;
}