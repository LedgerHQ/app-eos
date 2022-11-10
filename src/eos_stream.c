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

#define EOSIO_TOKEN          0x5530EA033482A600
#define EOSIO_TOKEN_TRANSFER 0xCDCD3C2D57000000

#define EOSIO                0x5530EA0000000000
#define EOSIO_DELEGATEBW     0x4AA2A61B2A3F0000
#define EOSIO_UNDELEGATEBW   0xD4D2A8A986CA8FC0
#define EOSIO_VOTEPRODUCER   0xDD32AADE89D21570
#define EOSIO_BUYRAM         0x3EBD734800000000
#define EOSIO_BUYRAMBYTES    0x3EBD7348FECAB000
#define EOSIO_SELLRAM        0xC2A31B9A40000000
#define EOSIO_UPDATE_AUTH    0xD5526CA8DACB4000
#define EOSIO_DELETE_AUTH    0x4AA2ACA8DACB4000
#define EOSIO_REFUND         0xBA97A9A400000000
#define EOSIO_LINK_AUTH      0x8BA7036B2D000000
#define EOSIO_UNLINK_AUTH    0xD4E2E9C0DACB4000
#define EOSIO_NEW_ACCOUNT    0x9AB864229A9E4000

void initTxContext(txProcessingContext_t *context, 
                   cx_sha256_t *sha256, 
                   cx_sha256_t *dataSha256, 
                   txProcessingContent_t *processingContent,
                   uint8_t dataAllowed) {
    memset(context, 0, sizeof(txProcessingContext_t));
    context->sha256 = sha256;
    context->dataSha256 = dataSha256;
    context->content = processingContent;
    context->state = TLV_CHAIN_ID;
    context->dataAllowed = dataAllowed;
    cx_sha256_init(context->sha256);
    cx_sha256_init(context->dataSha256);
}

uint8_t readTxByte(txProcessingContext_t *context) {
    uint8_t data;
    if (context->commandLength < 1) {
        PRINTF("readTxByte Underflow\n");
        THROW(EXCEPTION);
    }
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

static inline void eos_assert(x) {
    if (!x) {
        THROW(STREAM_FAULT);
    }
}

static void processEosioUpdateAuth(txProcessingContext_t *context) {
    uint32_t bufferLength = context->currentActionDataBufferLength;
    uint8_t *buffer = context->actionDataBuffer;

    context->content->argumentCount = 4;

    uint32_t offset_update = 3 * sizeof(name_t) + sizeof(uint32_t);

    eos_assert(offset_update < bufferLength);
    buffer += offset_update;
    bufferLength -= offset_update;

    uint32_t totalKeys = 0;
    uint32_t read = unpack_variant32(buffer, bufferLength, &totalKeys);

    eos_assert(read < bufferLength);
    buffer += read;
    bufferLength -= read;

    offset_update = 1 + sizeof(public_key_t) + sizeof(uint16_t);
    eos_assert(!__builtin_mul_overflow(offset_update, totalKeys, &offset_update));

    eos_assert(offset_update < bufferLength);
    buffer += offset_update;
    bufferLength -= offset_update;

    eos_assert(!__builtin_add_overflow(totalKeys, totalKeys, &totalKeys));  // totalKeys *= 2
    eos_assert(!__builtin_add_overflow(context->content->argumentCount, totalKeys, &context->content->argumentCount));

    uint32_t totalAccounts = 0;
    read = unpack_variant32(buffer, bufferLength, &totalAccounts);

    // accounts data begins here
    eos_assert(read < bufferLength);
    buffer += read;
    bufferLength -= read;

    offset_update = sizeof(permisssion_level_t) + sizeof(uint16_t);
    eos_assert(!__builtin_mul_overflow(offset_update, totalAccounts, &offset_update));

    eos_assert(offset_update < bufferLength);
    buffer += offset_update;
    bufferLength -= offset_update;

    eos_assert(!__builtin_add_overflow(totalAccounts, totalAccounts, &totalAccounts)); // totalAccounts *= 2
    eos_assert(!__builtin_add_overflow(context->content->argumentCount, totalAccounts, &context->content->argumentCount));

    uint32_t totalWaits = 0;
    unpack_variant32(buffer, bufferLength, &totalWaits);

    eos_assert(!__builtin_add_overflow(totalWaits, totalWaits, &totalWaits)); // totalWaits *= 2
    eos_assert(!__builtin_add_overflow(context->content->argumentCount, totalWaits, &context->content->argumentCount));
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
    cx_hash(&context->dataSha256->header, CX_LAST, context->dataChecksum, 0,
            context->dataChecksum, sizeof(context->dataChecksum));
    context->content->argumentCount = 3;  
}

static void processEosioNewAccountAction(txProcessingContext_t *context) {
    uint8_t *buffer = context->actionDataBuffer;
    uint32_t bufferLength = context->currentActionDataBufferLength;
    
    // Offset to auth structures: owner, active
    buffer += 2 * sizeof(name_t); bufferLength -= 2 * sizeof(name_t);
    
    // Read owner key threshold
    uint32_t threshold = 0;
    memmove(&threshold, buffer, sizeof(threshold));
    if (threshold != 1) {
        PRINTF("Owner Threshold should be 1");
        THROW(EXCEPTION);
    }
    buffer += sizeof(threshold); bufferLength -= sizeof(threshold);
    
    uint32_t size = 0;
    uint32_t read = unpack_variant32(buffer, bufferLength, &size);
    if (size != 1) {
        PRINTF("Owner key must be 1");
        THROW(EXCEPTION);
    }
    buffer += read; bufferLength -= read;
    // Offset to key weight
    buffer += 1 + sizeof(public_key_t); bufferLength -= 1 + sizeof(public_key_t);
    uint16_t weight = 0;
    memmove(&weight, buffer, sizeof(weight));
    if (weight != 1) {
        PRINTF("Owner key weight must be 1");
        THROW(EXCEPTION);
    }
    buffer += sizeof(weight); bufferLength -= sizeof(weight);
    
    read = unpack_variant32(buffer, bufferLength, &size);
    if (size != 0) {
        PRINTF("No accounts allowed");
        THROW(EXCEPTION);
    }
    buffer += read; bufferLength -= read;
    read = unpack_variant32(buffer, bufferLength, &size);
    if (size != 0) {
        PRINTF("No delays allowed");
        THROW(EXCEPTION);
    }
    buffer += read; bufferLength -= read;
    
    // process Active authorization
    // -----------------------------------
    
    memmove(&threshold, buffer, sizeof(threshold));
    if (threshold != 1) {
        PRINTF("Active Threshold should be 1");
        THROW(EXCEPTION);
    }
    buffer += sizeof(threshold); bufferLength -= sizeof(threshold);
    
    read = unpack_variant32(buffer, bufferLength, &size);
    if (size != 1) {
        PRINTF("Active key must be 1");
        THROW(EXCEPTION);
    }
    buffer += read; bufferLength -= read;
    // Offset to key weight
    buffer += 1 + sizeof(public_key_t); bufferLength -= 1 + sizeof(public_key_t);
    weight = 0;
    memmove(&weight, buffer, sizeof(weight));
    if (weight != 1) {
        PRINTF("Active key weight must be 1");
        THROW(EXCEPTION);
    }
    buffer += sizeof(weight); bufferLength -= sizeof(weight);
    
    read = unpack_variant32(buffer, bufferLength, &size);
    if (size != 0) {
        PRINTF("No accounts allowed");
        THROW(EXCEPTION);
    }
    buffer += read; bufferLength -= read;
    unpack_variant32(buffer, bufferLength, &size);
    if (size != 0) {
        PRINTF("No delays allowed");
        THROW(EXCEPTION);
    }
    
    context->content->argumentCount = 4;
}

void printArgument(uint8_t argNum, txProcessingContext_t *context) {
    name_t contractName = context->contractName;
    name_t actionName = context->contractActionName;
    uint8_t *buffer = context->actionDataBuffer;
    uint32_t bufferLength = context->currentActionDataBufferLength;
    actionArgument_t *arg =  &context->content->arg;

    if (actionName == EOSIO_TOKEN_TRANSFER) {
        parseTokenTransfer(buffer, bufferLength, argNum, arg);
        return;
    }

    if (contractName == EOSIO) {
        switch (actionName) {
        case EOSIO_DELEGATEBW:
            parseDelegate(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_UNDELEGATEBW:
            parseUndelegate(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_REFUND:
            parseRefund(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_BUYRAM:
            parseBuyRam(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_BUYRAMBYTES:
            parseBuyRamBytes(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_SELLRAM:
            parseSellRam(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_VOTEPRODUCER:
            parseVoteProducer(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_UPDATE_AUTH:
            parseUpdateAuth(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_DELETE_AUTH:
            parseDeleteAuth(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_LINK_AUTH:
            parseLinkAuth(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_UNLINK_AUTH:
            parseUnlinkAuth(buffer, bufferLength, argNum, arg);
            break;
        case EOSIO_NEW_ACCOUNT:
            parseNewAccount(buffer, bufferLength, argNum, arg);
            break;
        default:
            if (context->dataAllowed == 1) {
                parseUnknownAction(context->dataChecksum, sizeof(context->dataChecksum), argNum, arg);
            }
        }
        return;
    }
    
    if (context->dataAllowed == 1) {
        parseUnknownAction(context->dataChecksum, sizeof(context->dataChecksum), argNum, arg);
    }
}

static bool isKnownAction(txProcessingContext_t *context) {
    name_t contractName = context->contractName;
    name_t actionName = context->contractActionName;
    if (actionName == EOSIO_TOKEN_TRANSFER) {
        return true;
    }

    if (contractName == EOSIO) {
        switch (actionName) {
        case EOSIO_DELEGATEBW:
        case EOSIO_UNDELEGATEBW:
        case EOSIO_REFUND:
        case EOSIO_BUYRAM:
        case EOSIO_BUYRAMBYTES:
        case EOSIO_SELLRAM:
        case EOSIO_VOTEPRODUCER:
        case EOSIO_UPDATE_AUTH:
        case EOSIO_DELETE_AUTH:
        case EOSIO_LINK_AUTH:
        case EOSIO_UNLINK_AUTH:
        case EOSIO_NEW_ACCOUNT:
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
    cx_hash(&context->sha256->header, 0, buffer, length, NULL, 0);
}

static void hashActionData(txProcessingContext_t *context, uint8_t *buffer, uint32_t length) {
    cx_hash(&context->dataSha256->header, 0, buffer, length, NULL, 0);
}

/**
 * Process all fields that do not requre any processing except hashing.
 * The data comes in by chucks, so it may happen that buffer may contain 
 * incomplete data for particular field. Function designed to process 
 * everything until it receives all data for a particular field 
 * and after that will move to next field.
*/
static void processField(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length = 
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
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
 * caches an incomming data. So, when all bytes for particulat field are received
 * do additional processing: Read actual number of actions encoded in buffer.
 * Throw exception if number is not '0'.
*/
static void processZeroSizeField(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length = 
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
        hashTxData(context, context->workBuffer, length);

        // Store data into a buffer
        if (length > sizeof(context->sizeBuffer) - context->currentFieldPos) {
            THROW(EXCEPTION);
        }
        memmove(context->sizeBuffer + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        uint32_t sizeValue = 0;
        unpack_variant32(context->sizeBuffer, context->currentFieldPos + 1, &sizeValue);
        if (sizeValue != 0) {
            PRINTF("processCtxFreeAction Action Number must be 0\n");
            THROW(EXCEPTION);
        }
        // Reset size buffer
        memset(context->sizeBuffer, 0, sizeof(context->sizeBuffer));

        // Move to next state
        context->state++;
        context->processingField = false;
    }
}

/**
 * Process Action Number Field. Except hashing the data, function
 * caches an incomming data. So, when all bytes for particulat field are received
 * do additional processing: Read actual number of actions encoded in buffer.
*/
static void processActionListSizeField(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length = 
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
        hashTxData(context, context->workBuffer, length);

        // Store data into a buffer
        if (length > sizeof(context->sizeBuffer) - context->currentFieldPos) {
            THROW(EXCEPTION);
        }
        memmove(context->sizeBuffer + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        unpack_variant32(context->sizeBuffer, context->currentFieldPos + 1, &context->currentActionNumer);
        context->currentActionIndex = 0;
        
        // Reset size buffer
        memset(context->sizeBuffer, 0, sizeof(context->sizeBuffer));

        context->state++;
        context->processingField = false;
        if (context->currentActionNumer > 1) {
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
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
        hashTxData(context, context->workBuffer, length);
        
        uint8_t *pContract = (uint8_t *)&context->contractName;

        if (length > sizeof(context->contractName) - context->currentFieldPos) {
            THROW(EXCEPTION);
        }
        memmove(pContract + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->state++;
        context->processingField = false;

        memset(context->content->contract, 0, sizeof(context->content->contract));
        name_to_string(context->contractName, context->content->contract, sizeof(context->content->contract));
    }
}

/**
 * Process Action Name Field. Cache a data of the field in order to 
 * display it for validation.
*/
static void processActionName(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length = 
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
        hashTxData(context, context->workBuffer, length);

        uint8_t *pAction = (uint8_t *)&context->contractActionName;
        if (length > sizeof(context->contractActionName) - context->currentFieldPos) {
            THROW(EXCEPTION);
        }
        memmove(pAction + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->state++;
        context->processingField = false;

        memset(context->content->action, 0, sizeof(context->content->action));
        name_to_string(context->contractActionName, context->content->action, sizeof(context->content->action));
    }
}

/**
 * Process Authorization Number Field. Initializa context action number 
 * index and context action number. 
*/
static void processAuthorizationListSizeField(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length = 
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
        hashTxData(context, context->workBuffer, length);

        // Store data into a buffer
        if (length > sizeof(context->sizeBuffer) - context->currentFieldPos) {
            THROW(EXCEPTION);
        }
        memmove(context->sizeBuffer + context->currentFieldPos, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        unpack_variant32(context->sizeBuffer, context->currentFieldPos + 1, &context->currentAutorizationNumber);
        context->currentAutorizationIndex = 0;
        // Reset size buffer
        memset(context->sizeBuffer, 0, sizeof(context->sizeBuffer));

        // Move to next state
        context->state++;
        context->processingField = false;
    }
}

/**
 * Process Authorization Permission Field. When the field is processed 
 * start over authorization processing if the there is data for that.
*/
static void processAuthorizationPermission(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length = 
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
        hashTxData(context, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentAutorizationIndex++;
        // Reset size buffer
        memset(context->sizeBuffer, 0, sizeof(context->sizeBuffer));

        // Start over reading Authorization data or move to the next state
        // if all authorization data have beed read
        if (context->currentAutorizationIndex != context->currentAutorizationNumber) {
            context->state = TLV_AUTHORIZATION_ACTOR;
        } else {
            context->state++;
        }
        context->processingField = false;
    }
}


static void processUnknownActionDataSize(txProcessingContext_t *context) {
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length = 
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
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
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                ? context->commandLength
                : context->currentFieldLength - context->currentFieldPos);

        if (length > context->commandLength) {
            THROW(EXCEPTION);
        }
        hashTxData(context, context->workBuffer, length);
        hashActionData(context, context->workBuffer, length);

        context->workBuffer += length;
        context->commandLength -= length;
        context->currentFieldPos += length;
    }

    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentActionDataBufferLength = context->currentFieldLength;

        processUnknownAction(context);

        if (++context->currentActionIndex < context->currentActionNumer) {
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
    if (context->currentFieldLength > sizeof(context->actionDataBuffer) - 1) {
        PRINTF("processActionData data overflow\n");
        THROW(EXCEPTION);
    }

    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t length = 
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
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

        if (context->contractActionName == EOSIO_TOKEN_TRANSFER) {
            processTokenTransfer(context);
        } else if (context->contractName == EOSIO) {
            switch (context->contractActionName) {
            case EOSIO_DELEGATEBW:
                processEosioDelegate(context);
                break;
            case EOSIO_UNDELEGATEBW:
                processEosioUndelegate(context);
                break;
            case EOSIO_REFUND:
                processEosioRefund(context);
                break;
            case EOSIO_VOTEPRODUCER:
                processEosioVoteProducer(context);
                break;
            case EOSIO_BUYRAM:
            case EOSIO_BUYRAMBYTES:
                processEosioBuyRam(context);
                break;
            case EOSIO_SELLRAM:
                processEosioSellRam(context);
                break;
            case EOSIO_UPDATE_AUTH:
                processEosioUpdateAuth(context);
                break;
            case EOSIO_DELETE_AUTH:
                processEosioDeleteAuth(context);
                break;
            case EOSIO_LINK_AUTH:
                processEosioLinkAuth(context);
                break;
            case EOSIO_UNLINK_AUTH:
                processEosioUnlinkAuth(context);
                break;
            case EOSIO_NEW_ACCOUNT:
                processEosioNewAccountAction(context);
                break;
            default:
                THROW(EXCEPTION);
            }
        }
        
        if (++context->currentActionIndex < context->currentActionNumer) {
            context->state = TLV_ACTION_ACCOUNT;
        } else {
            context->state = TLV_TX_EXTENSION_LIST_SIZE;
        }
        
        context->processingField = false;
        context->actionReady = true;
    }
}

static parserStatus_e processTxInternal(txProcessingContext_t *context) {
    for(;;) {
        if (context->confirmProcessing) {
            context->confirmProcessing = false;
            return STREAM_CONFIRM_PROCESSING;
        }
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
                context->tlvBuffer[context->tlvBufferPos++] =
                    readTxByte(context);

                decoded = tlvTryDecode(context->tlvBuffer, context->tlvBufferPos, 
                    &context->currentFieldLength, &valid);

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
            processField(context);
            break;

        case TLV_AUTHORIZATION_PERMISSION:
            processAuthorizationPermission(context);
            break;
        
        case TLV_ACTION_DATA_SIZE:
            if (isKnownAction(context) || context->dataAllowed == 0) {
                processField(context);
            } else {
                processUnknownActionDataSize(context);
            }
            break;
        
        case TLV_ACTION_DATA:
            if (isKnownAction(context)) {
                processActionData(context);
            } else if (context->dataAllowed == 1) {
                processUnknownActionData(context);
            } else {
                PRINTF("UNKNOWN ACTION");
                THROW(EXCEPTION);
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
 * could be very expencive. Due to these features and limitations
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
 * More infomation about DER Tag Length Value encoding is here: http://luca.ntop.org/Teaching/Appunti/asn1.html.
 * Only octet tag number is allowed. 
 * Value is encoded as octet string.
 * The length of the string is stored in Length byte(s)
 * 
 * Detailed flat map representation of incoming data:
 * [CHAIN ID][HEADER][CTX_FREE_ACTION_NUMBER][ACTION_NUMBER][ACTION 0][TX_EXTENSION_NUMBER][CTX_FREE_ACTION_DATA_NUMBER]
 * 
 * CHAIN ID:
 * [32 BYTES]
 * 
 * HEADER size may vary due to MAX_NET_USAGE_WORDS and DELAY_SEC serialization:
 * [EXPIRATION][REF_BLOCK_NUM][REF_BLOCK_PREFIX][MAX_NET_USAGE_WORDS][MAX_CPU_USAGE_MS][DELAY_SEC]
 * 
 * CTX_FREE_ACTION_NUMBER theoretically is not fixed due to serialization. Ledger accepts only 0 as encoded value.
 * ACTION_NUMBER theoretically is not fixed due to serialization.
 * 
 * ACTION size may vary as authorization list and action data is dynamic:
 * [ACCOUNT][NAME][AUTHORIZATION_NUMBER][AUTHORIZATION 0][AUTHORIZATION 1]..[AUTHORIZATION N][ACTION_DATA]
 * ACCOUNT and NAME are 8 bytes long, both.
 * AUTHORIZATION_NUMBER theoretically is not fixed due to serialization.
 * ACTION_DATA is octet string of bytes.
 *  
 * AUTHORIZATION is 16 bytes long:
 * [ACTOR][PERMISSION]
 * ACTOR and PERMISSION are 8 bites long, both.
 * 
 * TX_EXTENSION_NUMBER theoretically is not fixed due to serialization. Ledger accepts only 0 as encoded value.
 * CTX_FREE_ACTION_DATA_NUMBER theoretically is not fixed due to serialization. Ledger accepts only 0 as encoded value.
*/
parserStatus_e parseTx(txProcessingContext_t *context, uint8_t *buffer, uint32_t length) {
    parserStatus_e result;
#ifdef DEBUG_APP
    // Do not catch exceptions.
    context->workBuffer = buffer;
    context->commandLength = length;
    result = processTxInternal(context);
#else
    BEGIN_TRY {
        TRY {
            if (context->commandLength == 0) {
                context->workBuffer = buffer;
                context->commandLength = length;
            }
            result = processTxInternal(context);
        }
        CATCH_OTHER(e) {
            result = STREAM_FAULT;
        }
        FINALLY {
        }
    }
    END_TRY;
#endif
    return result;
}
