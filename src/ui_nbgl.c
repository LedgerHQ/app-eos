
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

#ifdef HAVE_NBGL

/*********************
 *      INCLUDES
 *********************/

#include "os.h"
#include "ux.h"
#include "nbgl_use_case.h"

#include "glyphs.h"
#include "main.h"
#include "ui.h"
#include "config.h"
#include "eos_parse.h"

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;


static nbgl_layoutSwitch_t switches[1] = {0};
static const char* const INFO_TYPES[] = {"Version"};
static const char* const INFO_CONTENTS[] = {APPVERSION};

static bool nav_callback(uint8_t page, nbgl_pageContent_t *content) {
    // the first settings page contains the version of the app
    // the second settings page contains 1 switch
    if (page == 0) {
        switches[0].initState = is_data_allowed();
        switches[0].text = "Contract data";
        switches[0].subText = "Allow contract data\nin transactions";
        switches[0].token = FIRST_USER_TOKEN;
        switches[0].tuneId = TUNE_TAP_CASUAL;

        content->type = SWITCHES_LIST;
        content->switchesList.nbSwitches = 1;
        content->switchesList.switches = (nbgl_layoutSwitch_t*)switches;
    } else if (page == 1) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 1;
        content->infosList.infoTypes = (const char**) INFO_TYPES;
        content->infosList.infoContents = (const char**) INFO_CONTENTS;
    } else {
        return false;
    }
    // valid page so return true
    return true;
}


// controlsCallback callback called when any controls in the settings (radios, switches)
// is called (the tokens must be >= FIRST_USER_TOKEN)
static void controls_callback(int token, uint8_t state) {
    if (token == FIRST_USER_TOKEN) {
        if (state == 0 && is_data_allowed()) {
            toogle_data_allowed();
        } else if (state != 0 && !is_data_allowed()) {
            toogle_data_allowed();
        }
    }
}

#define SETTINGS_PAGE_NUMBERS  2
#define SETTINGS_INIT_PAGE_IDX 0
static void app_settings(void) {
    nbgl_useCaseSettings(APPNAME,
                         SETTINGS_INIT_PAGE_IDX,
                         SETTINGS_PAGE_NUMBERS,
                         false,
                         ui_idle,
                         nav_callback,
                         controls_callback);
}

static void app_quit(void) {
    // exit app here
    os_sched_exit(-1);
}

void ui_idle(void) {
    nbgl_useCaseHome(APPNAME,
                     &C_stax_app_eos_64px,
                     "This app confirms actions on\nthe EOS network",
                     true,
                     app_settings,
                     app_quit);
}

///////////////////////////////////////////////////////////////////////////////

static void address_verification_cancelled(void) {
    user_action_address_cancel();
}

static void display_address_callback(bool confirm) {
    if (confirm) {
        user_action_address_ok();
    } else {
        address_verification_cancelled();
    }
}

// called when tapping on review start page to actually display address
static void display_addr(void) {
    nbgl_useCaseAddressConfirmation(tmpCtx.publicKeyContext.address,
                                    &display_address_callback);
}

void ui_display_public_key_flow(void) {
    nbgl_useCaseReviewStart(&C_stax_app_eos_64px,
                            "Verify Eos\naddress", NULL, "Cancel",
                            display_addr, address_verification_cancelled);
}

void ui_display_public_key_done(bool validated) {
    if (validated) {
        nbgl_useCaseStatus("ADDRESS\nVERIFIED", true, ui_idle);
    } else {
        nbgl_useCaseStatus("Address verification\ncancelled", false, ui_idle);
    }
}

///////////////////////////////////////////////////////////////////////////////

static nbgl_layoutTagValue_t pair;
static nbgl_layoutTagValueList_t pairList = {0};
static nbgl_pageInfoLongPress_t infoLongPress;

#define MAX_TAG_VALUE_PAIRS_DISPLAYED 4
static actionArgument_t bkp_args[MAX_TAG_VALUE_PAIRS_DISPLAYED];

static char review_title[20];

static void transaction_rejected(void) {
    user_action_tx_cancel();
}

static void reject_confirmation(void) {
    nbgl_useCaseConfirm("Reject transaction?", NULL, "Yes, Reject", "Go back to transaction", transaction_rejected);
}

// called when long press button on 3rd page is long-touched or when reject footer is touched
static void review_choice(bool confirm) {
    if (confirm) {
        user_action_single_action_sign_flow_ok();
    } else {
        reject_confirmation();
    }
}


// function called by NBGL to get the pair indexed by "index"
static nbgl_layoutTagValue_t* get_single_action_review_pair(uint8_t index) {
    if (index == 0) {
        pair.item = "Contract";
        pair.value = txContent.contract;
    } else if (index == 1) {
        pair.item = "Action";
        pair.value = txContent.action;
    } else {
        // Retrieve action argument, with an index to action args offset
        printArgument(index - 2, &txProcessingCtx);

        // Backup action argument as MAX_TAG_VALUE_PAIRS_DISPLAYED can be displayed
        // simultaneously and their content must be store on app side buffer as
        // only the buffer pointer is copied by the SDK and not the buffer content.
        uint8_t bkp_index = index % MAX_TAG_VALUE_PAIRS_DISPLAYED;
        memcpy(bkp_args[bkp_index].label, txContent.arg.label, sizeof(txContent.arg.label));
        memcpy(bkp_args[bkp_index].data, txContent.arg.data, sizeof(txContent.arg.data));
        pair.item = bkp_args[bkp_index].label;
        pair.value = bkp_args[bkp_index].data;
    }
    return &pair;
}

static void single_action_review_continue(void) {
    infoLongPress.icon = &C_stax_app_eos_64px;

    if (txProcessingCtx.currentActionIndex == txProcessingCtx.currentActionNumer) {
        infoLongPress.text = "Sign transaction";
        infoLongPress.longPressText = "Hold to sign";
    } else {
        infoLongPress.text = "Accept & review next";
        infoLongPress.longPressText = "Hold to continue";
    }

    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = txContent.argumentCount + 2;
    pairList.pairs = NULL; // to indicate that callback should be used
    pairList.callback = get_single_action_review_pair;
    pairList.startIndex = 0;

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject transaction", review_choice);
}


void ui_display_single_action_sign_flow(void) {
    if (txProcessingCtx.currentActionNumer > 1) {
        snprintf(review_title, sizeof(review_title), "Review action #%d",
                 txProcessingCtx.currentActionIndex);
        review_title[sizeof(review_title) - 1] = '\0';
    } else {
        strlcpy(review_title, "Review transaction", sizeof(review_title));
    }

    nbgl_useCaseReviewStart(&C_stax_app_eos_64px,
                            review_title,
                            NULL,
                            "Reject transaction",
                            single_action_review_continue,
                            reject_confirmation);
}

void ui_display_action_sign_done(parserStatus_e status, bool validated) {
    if (status == STREAM_FINISHED) {
        if (validated) {
            nbgl_useCaseStatus("TRANSACTION\nSIGNED", true, ui_idle);
        } else {
            nbgl_useCaseStatus("Transaction rejected", false, ui_idle);
        }
    } else {
        // STREAM_PROCESSING
        // Display back the original UX to behave as on Nano
        // But might be good to display a processing screen instead.
        ui_idle();
    }
}

///////////////////////////////////////////////////////////////////////////////

void ui_display_multiple_action_sign_flow(void) {
    snprintf(review_title, sizeof(review_title), "With %d actions", txProcessingCtx.currentActionNumer);

    nbgl_useCaseReviewStart(&C_stax_app_eos_64px,
                            "Review transaction",
                            review_title,
                            "Reject transaction",
                            user_action_multipls_action_sign_flow_ok,
                            reject_confirmation);
}

#endif
