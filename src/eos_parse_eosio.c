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

#include "eos_parse_eosio.h"
#include "eos_types.h"
#include "os.h"

void parseDelegate(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "From", arg, &read, &written);
    } else if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Receiver", arg, &read, &written);
    } else if (argNum == 2) {
        buffer += 2 * sizeof(name_t); bufferLength -= 2 * sizeof(name_t);
        parseAssetField(buffer, bufferLength, "NET", arg, &read, &written);
    } else if (argNum == 3) {
        buffer += 2 * sizeof(name_t) + sizeof(asset_t); bufferLength -= 2 * sizeof(name_t) + sizeof(asset_t);
        parseAssetField(buffer, bufferLength, "CPU", arg, &read, &written);
    } else if (argNum == 4) {
        printString("Yes", "Transfer Stake", arg);
    }
}

void parseUndelegate(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "From", arg, &read, &written);
    } else if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Receiver", arg, &read, &written);
    } else if (argNum == 2) {
        buffer += 2 * sizeof(name_t); bufferLength -= 2 * sizeof(name_t);
        parseAssetField(buffer, bufferLength, "NET", arg, &read, &written);
    } else if (argNum == 3) {
        buffer += 2 * sizeof(name_t) + sizeof(asset_t); bufferLength -= 2 * sizeof(name_t) + sizeof(asset_t);
        parseAssetField(buffer, bufferLength, "CPU", arg, &read, &written);
    }
}

void parseRefund(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Account", arg, &read, &written);
    }
}

void parseBuyRam(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Buyer", arg, &read, &written);
    } else if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Receiver", arg, &read, &written);
    } else if (argNum == 2) {
        buffer += 2 * sizeof(name_t); bufferLength -= 2 * sizeof(name_t);
        parseAssetField(buffer, bufferLength, "Tokens", arg, &read, &written);
    }
}

void parseBuyRamBytes(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Buyer", arg, &read, &written);
    } else if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Receiver", arg, &read, &written);
    } else if (argNum == 2) {
        buffer += 2 * sizeof(name_t); bufferLength -= 2 * sizeof(name_t);
        parseUint32Field(buffer, bufferLength, "Bytes", arg, &read, &written);
    }
}

void parseSellRam(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Receiver", arg, &read, &written);
    } else if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseUInt64Field(buffer, bufferLength, "Bytes", arg, &read, &written);
    }
}

void parseVoteProducer(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Account", arg, &read, &written);
        return;
    }

    uint8_t *proxyBuffer = buffer + sizeof(name_t);
    name_t proxy = 0;
    memmove(&proxy, proxyBuffer, sizeof(name_t));
    if (proxy != 0 && argNum == 1) {
        parseNameField(proxyBuffer, sizeof(name_t), "Proxy", arg, &read, &written);
        return;
    }

    // Move to producers list
    buffer += 2 * sizeof(name_t);
    bufferLength -= 2 * sizeof(name_t);

    uint32_t totalProducers = 0;
    read = unpack_variant32(buffer, bufferLength, &totalProducers);
    buffer += read; bufferLength -= read;

    uint32_t producerIndex = argNum - 1;
    buffer += producerIndex * sizeof(name_t);
    bufferLength -= producerIndex * sizeof(name_t);

    char label[14] = { 0 };
    snprintf(label, sizeof(label) - 1, "Producer #%d [%d]", argNum, totalProducers);
    parseNameField(buffer, bufferLength, label, arg, &read, &written);
}

void parseUpdateAuth(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Account", arg, &read, &written);
        return;
    } 
    
    if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Permission", arg, &read, &written);
        return;
    }
    
    uint8_t *parentBuffer = buffer + 2 * sizeof(name_t);
    name_t parent = 0;
    memmove(&parent, parentBuffer, sizeof(name_t));
    if (parent != 0 && argNum == 2) {
        parseNameField(parentBuffer, sizeof(name_t), "Parent", arg, &read, &written);
        return;
    } else if (argNum == 2) {
        uint8_t null[] = {4, 'N', 'U', 'L', 'L'};
        parseStringField(null, sizeof(null), "Parent", arg, &read, &written);
        return;
    }

    if (argNum == 3) {
        buffer += 3 * sizeof(name_t); bufferLength -= 3 * sizeof(name_t); 
        parseUint32Field(buffer, bufferLength, "Threshold", arg, &read, &written);
        return;
    }

    uint32_t prefixLength = 3 * sizeof(name_t) + sizeof(uint32_t);
    buffer += prefixLength;
    bufferLength -= prefixLength;
    uint8_t *keys = buffer;
    uint32_t keyBufferLength = bufferLength;

    uint32_t totalKeys = 0;
    read = unpack_variant32(keys, keyBufferLength, &totalKeys);
    keys += read; keyBufferLength -= read;
    
    uint8_t keysArgStart = 3;

    if (keysArgStart < argNum && argNum <= keysArgStart + totalKeys * 2) {
        for (uint8_t i = 0, argIndex = keysArgStart + 1; i < totalKeys; ++i, argIndex += 2) {
            
            char label[32] = { 0 };
            // Skip key Type
            keys += 1; keyBufferLength -= 1;
            
            if (argIndex == argNum) {
                snprintf(label, sizeof(label), "Key #%d", (i + 1));
                parsePublicKeyField(keys, keyBufferLength, label, arg, &read, &written);
                return;
            }

            keys += sizeof(public_key_t); keyBufferLength -= sizeof(public_key_t);

            if (argIndex + 1 == argNum) {
                snprintf(label, sizeof(label), "Key #%d Weight", (i + 1));
                parseUint16Field(keys, keyBufferLength, label, arg, &read, &written);
                return;
            }

            keys += sizeof(uint16_t); keyBufferLength -= sizeof(uint16_t);
        }
    }

    // Accounts
    const uint32_t keyStep = (1 + sizeof(public_key_t) + sizeof(uint16_t));
    uint8_t *accounts = keys + keyStep * totalKeys;
    uint32_t accountsBufferLen = keyBufferLength - keyStep * totalKeys;
    
    uint32_t totalAccounts = 0;
    read = unpack_variant32(accounts, accountsBufferLen, &totalAccounts);
    accounts += read; accountsBufferLen -= read;
    
    uint32_t accountsArgStart = keysArgStart + totalKeys * 2;
    
    if (accountsArgStart < argNum && argNum <= accountsArgStart + totalAccounts * 2) {
        for (uint8_t i = 0, argIndex = accountsArgStart + 1; i < totalAccounts; ++i, argIndex += 2) {
            char label[32] = { 0 };
            if (argIndex == argNum) {
                snprintf(label, sizeof(label), "Account #%d", (i + 1));
                parsePermissionField(accounts, accountsBufferLen, label, arg, &read, &written);
                return;
            }
            
            accounts += sizeof(permisssion_level_t); accountsBufferLen -= sizeof(permisssion_level_t);
            
            if (argIndex + 1 == argNum) {
                snprintf(label, sizeof(label), "Account #%d Weight", (i + 1));
                parseUint16Field(accounts, accountsBufferLen, label, arg, &read, &written);
                return;
            }
            
            accounts += sizeof(uint16_t); accountsBufferLen -= sizeof(uint16_t);
        }
    }

    // Delays
    const uint32_t accountStep = sizeof(permisssion_level_t) + sizeof(uint16_t);
    uint8_t *delays = accounts + accountStep * totalAccounts;
    uint32_t delayBufferLen = accountsBufferLen - accountStep * totalAccounts;

    uint32_t totalDelays = 0;
    read = unpack_variant32(delays, delayBufferLen, &totalDelays);
    delays += read; delayBufferLen -= read;
    
    uint32_t delaysArgStart = keysArgStart + totalKeys * 2 + totalAccounts * 2;

    if (delaysArgStart < argNum && argNum <= delaysArgStart + totalDelays * 2) {
        for (uint8_t i = 0, argIndex = delaysArgStart + 1; i < totalDelays; ++i, argIndex += 2) {
            char label[32] = { 0 };

            if (argIndex == argNum) {
                snprintf(label, sizeof(label), "Delay #%d", (i + 1));
                parseUint32Field(delays, delayBufferLen, label, arg, &read, &written);
                return;
            }
            
            delays += sizeof(uint32_t); delayBufferLen -= sizeof(uint32_t);
            
            if (argIndex + 1 == argNum) {
                snprintf(label, sizeof(label), "Delay #%d Weight", (i + 1));
                parseUint16Field(delays, delayBufferLen, label, arg, &read, &written);
                return;
            }
            
            delays += sizeof(uint16_t); delayBufferLen -= sizeof(uint16_t);
        }
    }
}

void parseDeleteAuth(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Account", arg, &read, &written);
        return;
    } 
    
    if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Permission", arg, &read, &written);
        return;
    }
}

void parseLinkAuth(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Account", arg, &read, &written);
        return;
    } 
    
    if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Contract", arg, &read, &written);
        return;
    }

    if (argNum == 2) {
        buffer += 2 * sizeof(name_t); bufferLength -= 2 * sizeof(name_t);
        parseNameField(buffer, bufferLength, "Action", arg, &read, &written);
        return;
    }

    if (argNum == 3) {
        buffer += 3 * sizeof(name_t); bufferLength -= 3 * sizeof(name_t);
        parseNameField(buffer, bufferLength, "Permission", arg, &read, &written);
        return;
    }
}

void parseUnlinkAuth(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;

    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Account", arg, &read, &written);
        return;
    } 
    
    if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Contract", arg, &read, &written);
        return;
    }

    if (argNum == 2) {
        buffer += 2 * sizeof(name_t); bufferLength -= 2 * sizeof(name_t);
        parseNameField(buffer, bufferLength, "Action", arg, &read, &written);
        return;
    }
}

void parseNewAccount(uint8_t *buffer, uint32_t bufferLength, uint8_t argNum, actionArgument_t *arg) {
    uint32_t read = 0;
    uint32_t written = 0;
    
    if (argNum == 0) {
        parseNameField(buffer, bufferLength, "Creator", arg, &read, &written);
        return;
    }
    
    if (argNum == 1) {
        buffer += sizeof(name_t); bufferLength -= sizeof(name_t);
        parseNameField(buffer, bufferLength, "Account", arg, &read, &written);
        return;
    }
    
    // Offset to auth structures: owner, active
    buffer += 2 * sizeof(name_t); bufferLength -= 2 * sizeof(name_t);
    // offset to owner key; skip threshold
    buffer += sizeof(uint32_t); bufferLength -= sizeof(uint32_t);
    // skip key counter and key type
    buffer += 2; bufferLength -= 2;
    
    if (argNum == 2) {
        parsePublicKeyField(buffer, bufferLength, "Owner key", arg, &read, &written);
        return;
    }
    // skip public key
    buffer += sizeof(public_key_t) + sizeof(uint16_t); bufferLength -= sizeof(public_key_t) + sizeof(uint16_t);
    // skip accounts, delays
    // Offset to "active"
    buffer += 2; bufferLength -= 2;
    
    // skip threshold
    // offset to active key; skip threshold
    buffer += sizeof(uint32_t); bufferLength -= sizeof(uint32_t);
    // skip key counter and key type
    buffer += 2; bufferLength -= 2;
    
    if (argNum == 3) {
        parsePublicKeyField(buffer, bufferLength, "Active key", arg, &read, &written);
        return;
    }
}
