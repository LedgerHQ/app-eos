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
#include "os.h"

typedef struct internalStorage_t {
    uint8_t unknownActionAllowed;
    uint8_t verbose;
    uint8_t initialized;
} internalStorage_t;

const internalStorage_t N_storage_real;
#define N_storage (*(volatile internalStorage_t *) PIC(&N_storage_real))

void config_init(void) {
    if (N_storage.initialized != 0x01) {
        internalStorage_t storage;
        storage.unknownActionAllowed = 0x00;
        storage.verbose = 0x00;
        storage.initialized = 0x01;
        nvm_write((void *) &N_storage, (void *) &storage, sizeof(internalStorage_t));
    }
}

/*
 * Allow unknown actions
 * This app contains a fixed list of supported contracts
 * Contracts not matching this list are considered "unknown"
 * This setting allows for review and signing of unknown contracts
 * By default Off
 */
bool is_unknown_action_allowed(void) {
    return N_storage.unknownActionAllowed == 1;
}

void toogle_unknown_action_allowed(void) {
    uint8_t value = (is_unknown_action_allowed() ? 0 : 1);
    nvm_write((void *) &N_storage.unknownActionAllowed, (void *) &value, sizeof(uint8_t));
}

/*
 * Show verbose settings
 * Presents null.vaulta actions for review otherwise signed blind
 * Shows checksums on unknown actions
 * Shows authorities on actions
 */
bool is_verbose(void) {
    return N_storage.verbose == 1;
}

void toogle_verbose_config(void) {
    uint8_t value = (is_verbose() ? 0 : 1);
    nvm_write((void *) &N_storage.verbose, (void *) &value, sizeof(uint8_t));
}
