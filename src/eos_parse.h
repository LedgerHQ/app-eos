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

#ifndef __EOS_PARSE_H__
#define __EOS_PARSE_H__

#include <stdint.h>

typedef struct actionArgument_t {
    char label[32];
    char data[128];
} actionArgument_t;

void printString(const char in[], const char fieldName[], actionArgument_t *arg);
void parseNameField(uint8_t *in,
                    uint32_t inLength,
                    const char fieldName[],
                    actionArgument_t *arg,
                    uint32_t *read,
                    uint32_t *written);
void parsePublicKeyField(uint8_t *in,
                         uint32_t inLength,
                         const char fieldName[],
                         actionArgument_t *arg,
                         uint32_t *read,
                         uint32_t *written);
void parseUint16Field(uint8_t *in,
                      uint32_t inLength,
                      const char fieldName[],
                      actionArgument_t *arg,
                      uint32_t *read,
                      uint32_t *written);
void parseUint32Field(uint8_t *in,
                      uint32_t inLength,
                      const char fieldName[],
                      actionArgument_t *arg,
                      uint32_t *read,
                      uint32_t *written);
void parseUInt64Field(uint8_t *in,
                      uint32_t inLength,
                      const char fieldName[],
                      actionArgument_t *arg,
                      uint32_t *read,
                      uint32_t *written);
void parseAssetField(uint8_t *in,
                     uint32_t inLength,
                     const char fieldName[],
                     actionArgument_t *arg,
                     uint32_t *read,
                     uint32_t *written);
void parseStringField(uint8_t *in,
                      uint32_t inLength,
                      const char fieldName[],
                      actionArgument_t *arg,
                      uint32_t *read,
                      uint32_t *written);
void parsePermissionField(uint8_t *in,
                          uint32_t inLength,
                          const char fieldName[],
                          actionArgument_t *arg,
                          uint32_t *read,
                          uint32_t *written);

#endif
