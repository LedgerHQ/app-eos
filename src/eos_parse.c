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

#include "eos_parse.h"
#include "os.h"
#include "cx.h"
#include "eos_types.h"
#include "eos_utils.h"
#include <stdbool.h>
#include <string.h>
#include "ledger_assert.h"

void printString(const char in[], const char fieldName[], actionArgument_t *arg) {
    uint32_t inLength = strlen(in);
    uint32_t labelLength = strlen(fieldName);

    memset(arg->label, 0, sizeof(arg->label));
    memset(arg->data, 0, sizeof(arg->data));

    memmove(arg->label, fieldName, labelLength);
    memmove(arg->data, in, inLength);
}

void parseNameField(uint8_t *in,
                    uint32_t inLength,
                    const char fieldName[],
                    actionArgument_t *arg,
                    uint32_t *read,
                    uint32_t *written) {
    LEDGER_ASSERT(inLength >= sizeof(name_t), "parseActionData Insufficient buffer");
    uint32_t labelLength = strlen(fieldName);
    LEDGER_ASSERT(labelLength <= sizeof(arg->label), "parseActionData Label too long");

    memset(arg->label, 0, sizeof(arg->label));
    memset(arg->data, 0, sizeof(arg->data));

    memmove(arg->label, fieldName, labelLength);
    name_t name = buffer_to_name_type(in, sizeof(name_t));
    uint32_t writtenToBuff = name_to_string(name, arg->data, sizeof(arg->data) - 1);

    *read = sizeof(name_t);
    *written = writtenToBuff;
}

void parsePublicKeyField(uint8_t *in,
                         uint32_t inLength,
                         const char fieldName[],
                         actionArgument_t *arg,
                         uint32_t *read,
                         uint32_t *written) {
    LEDGER_ASSERT(inLength >= 33, "parseActionData Insufficient buffer");
    uint32_t labelLength = strlen(fieldName);
    LEDGER_ASSERT(labelLength <= sizeof(arg->label), "parseActionData Label too long");

    memset(arg->label, 0, sizeof(arg->label));
    memset(arg->data, 0, sizeof(arg->data));

    memmove(arg->label, fieldName, labelLength);
    uint32_t writtenToBuff = compressed_public_key_to_wif(in, 33, arg->data, sizeof(arg->data) - 1);

    *read = 33;
    *written = writtenToBuff;
}

void parseUint16Field(uint8_t *in,
                      uint32_t inLength,
                      const char fieldName[],
                      actionArgument_t *arg,
                      uint32_t *read,
                      uint32_t *written) {
    LEDGER_ASSERT(inLength >= sizeof(uint16_t), "parseActionData Insufficient buffer");
    uint32_t labelLength = strlen(fieldName);
    LEDGER_ASSERT(labelLength <= sizeof(arg->label), "parseActionData Label too long");

    memset(arg->label, 0, sizeof(arg->label));
    memset(arg->data, 0, sizeof(arg->data));

    memmove(arg->label, fieldName, labelLength);
    uint16_t value;
    memmove(&value, in, sizeof(uint16_t));
    snprintf(arg->data, sizeof(arg->data) - 1, "%d", value);

    *read = sizeof(uint16_t);
    *written = strlen(arg->data);
}

void parseUint32Field(uint8_t *in,
                      uint32_t inLength,
                      const char fieldName[],
                      actionArgument_t *arg,
                      uint32_t *read,
                      uint32_t *written) {
    LEDGER_ASSERT(inLength >= sizeof(uint32_t), "parseActionData Insufficient buffer");
    uint32_t labelLength = strlen(fieldName);
    LEDGER_ASSERT(labelLength <= sizeof(arg->label), "parseActionData Label too long");

    memset(arg->label, 0, sizeof(arg->label));
    memset(arg->data, 0, sizeof(arg->data));

    memmove(arg->label, fieldName, labelLength);
    uint32_t value;
    memmove(&value, in, sizeof(uint32_t));
    snprintf(arg->data, sizeof(arg->data) - 1, "%d", value);

    *read = sizeof(uint32_t);
    *written = strlen(arg->data);
}

void parseUInt64Field(uint8_t *in,
                      uint32_t inLength,
                      const char fieldName[],
                      actionArgument_t *arg,
                      uint32_t *read,
                      uint32_t *written) {
    LEDGER_ASSERT(inLength >= sizeof(uint64_t), "parseActionData Insufficient buffer");
    uint32_t labelLength = strlen(fieldName);
    LEDGER_ASSERT(labelLength <= sizeof(arg->label), "parseActionData Label too long");

    memset(arg->label, 0, sizeof(arg->label));
    memset(arg->data, 0, sizeof(arg->data));

    memmove(arg->label, fieldName, labelLength);
    uint64_t value;
    memmove(&value, in, sizeof(uint64_t));
    ui64toa(value, arg->data);

    *read = sizeof(uint64_t);
    *written = strlen(arg->data);
}

void parseAssetField(uint8_t *in,
                     uint32_t inLength,
                     const char fieldName[],
                     actionArgument_t *arg,
                     uint32_t *read,
                     uint32_t *written) {
    LEDGER_ASSERT(inLength >= sizeof(asset_t), "parseActionData Insufficient buffer");
    uint32_t labelLength = strlen(fieldName);
    LEDGER_ASSERT(labelLength <= sizeof(arg->label), "parseActionData Label too long");

    memset(arg->label, 0, sizeof(arg->label));
    memset(arg->data, 0, sizeof(arg->data));

    memmove(arg->label, fieldName, labelLength);
    asset_t asset;
    memmove(&asset, in, sizeof(asset));
    uint32_t writtenToBuff = asset_to_string(&asset, arg->data, sizeof(arg->data) - 1);

    *read = sizeof(asset_t);
    *written = writtenToBuff;
}

void parseStringField(uint8_t *in,
                      uint32_t inLength,
                      const char fieldName[],
                      actionArgument_t *arg,
                      uint32_t *read,
                      uint32_t *written) {
    uint32_t labelLength = strlen(fieldName);
    LEDGER_ASSERT(labelLength <= sizeof(arg->label), "parseActionData Label too long");

    memset(arg->label, 0, sizeof(arg->label));
    memset(arg->data, 0, sizeof(arg->data));

    memmove(arg->label, fieldName, labelLength);

    uint32_t fieldLength = 0;
    uint32_t readFromBuffer = unpack_variant32(in, inLength, &fieldLength);
    LEDGER_ASSERT(fieldLength <= sizeof(arg->data) - 1, "parseActionData Insufficient buffer");
    LEDGER_ASSERT(inLength >= fieldLength, "parseActionData Insufficient buffer");

    in += readFromBuffer;

    memmove(arg->data, in, fieldLength);

    *read = readFromBuffer + fieldLength;
    *written = fieldLength;
}

void parsePermissionField(uint8_t *in,
                          uint32_t inLength,
                          const char fieldName[],
                          actionArgument_t *arg,
                          uint32_t *read,
                          uint32_t *written) {
    uint32_t accountWrittenLength = 0;

    parseNameField(in, inLength, fieldName, arg, read, &accountWrittenLength);
    strlcat(arg->data, "@", sizeof(arg->data));

    in += *read;
    inLength -= *read;
    LEDGER_ASSERT(inLength >= sizeof(name_t), "parseActionData Insufficient buffer");
    name_t name = buffer_to_name_type(in, sizeof(name_t));

    *written = name_to_string(name,
                              arg->data + accountWrittenLength + 1,
                              sizeof(arg->data) - accountWrittenLength - 1) +
               accountWrittenLength;
    *read += sizeof(name_t);
}
