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

#include "eos_parse_unknown.h"
#include "os.h"
#include "eos_utils.h"
#include <string.h>

void parseUnknownAction(uint8_t *buffer,
                        uint32_t bufferLength,
                        uint8_t argNum,
                        actionArgument_t *arg) {
    if (argNum == 0) {
        printString("Arbitrary Data", "WARNING", arg);
    } else if (argNum == 1) {
        printString("Verify checksum", "WARNING", arg);
    } else if (argNum == 2) {
        char checksum[65] = {0};
        array_hexstr(checksum, buffer, bufferLength);
        printString(checksum, "Checksum", arg);
    }
}
