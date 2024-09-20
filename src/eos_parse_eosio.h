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

#ifndef __EOS_PARSE_EOSIO_H__
#define __EOS_PARSE_EOSIO_H__

#include "eos_parse.h"

void parseDelegate(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseUndelegate(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseRefund(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseBuyRam(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseBuyRamBytes(uint8_t *buffer,
                      uint32_t bufferLength,
                      uint8_t argNum,
                      actionArgument_t *arg);
void parseSellRam(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseVoteProducer(uint8_t *buffer,
                       uint32_t bufferLength,
                       uint8_t argNum,
                       actionArgument_t *arg);
void parseUpdateAuth(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseDeleteAuth(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseLinkAuth(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseUnlinkAuth(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);
void parseNewAccount(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg);

#endif
