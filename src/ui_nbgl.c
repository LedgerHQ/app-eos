
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

void app_exit(void);

static nbgl_contentSwitch_t switches[1] = {0};
static const char* const INFO_TYPES[] = {"Version"};
static const char* const INFO_CONTENTS[] = {APPVERSION};

static void controlsCallback(int token, uint8_t index, int page) {
    UNUSED(index);
    UNUSED(page);

    if (token != FIRST_USER_TOKEN) {
        return;
    }
    toogle_data_allowed();
    switches[0].initState = is_data_allowed();
}

void ui_idle(void) {
    static nbgl_contentInfoList_t infosList = {0};
    static nbgl_content_t contents[1] = {0};
    static nbgl_genericContents_t settingContents = {0};

    switches[0].initState = is_data_allowed();
    switches[0].text = "Contract data";
    switches[0].subText = "Allow contract data in transactions";
    switches[0].token = FIRST_USER_TOKEN;

    contents[0].type = SWITCHES_LIST;
    contents[0].content.switchesList.nbSwitches = 1;
    contents[0].content.switchesList.switches = switches;
    contents[0].contentActionCallback = controlsCallback;

    settingContents.callbackCallNeeded = false;
    settingContents.contentsList = contents;
    settingContents.nbContents = 1;

    infosList.nbInfos = 1;
    infosList.infoTypes = INFO_TYPES;
    infosList.infoContents = INFO_CONTENTS;

    nbgl_useCaseHomeAndSettings(APPNAME,
                                &C_stax_app_eos_64px,
                                NULL,
                                INIT_HOME_PAGE,
                                &settingContents,
                                &infosList,
                                NULL,
                                app_exit);
}

///////////////////////////////////////////////////////////////////////////////

static void display_address_callback(bool confirm) {
    if (confirm) {
        user_action_address_ok();
    } else {
        user_action_address_cancel();
    }
}

void ui_display_public_key_flow(void) {
    nbgl_useCaseAddressReview(tmpCtx.publicKeyContext.address,
                              NULL,
                              &C_stax_app_eos_64px,
                              "Verify Eos address",
                              NULL,
                              display_address_callback);
}

void ui_display_public_key_done(bool validated) {
    if (validated) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_VERIFIED, ui_idle);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_REJECTED, ui_idle);
    }
}

///////////////////////////////////////////////////////////////////////////////

static nbgl_contentTagValue_t pair;
static nbgl_contentTagValueList_t pairList = {0};

static actionArgument_t bkp_args[NB_MAX_DISPLAYED_PAIRS_IN_REVIEW];

// function called by NBGL to get the pair indexed by "index"
static nbgl_contentTagValue_t* get_single_action_review_pair(uint8_t index) {
    explicit_bzero(&pair, sizeof(pair));
    if (index == 0) {
        pair.item = "Contract";
        pair.value = txContent.contract;
    } else if (index == 1) {
        pair.item = "Action";
        pair.value = txContent.action;
    } else {
        // Retrieve action argument, with an index to action args offset
        printArgument(index - 2, &txProcessingCtx);

        // Backup action argument as NB_MAX_DISPLAYED_PAIRS_IN_REVIEW can be displayed
        // simultaneously and their content must be store on app side buffer as
        // only the buffer pointer is copied by the SDK and not the buffer content.
        uint8_t bkp_index = index % NB_MAX_DISPLAYED_PAIRS_IN_REVIEW;
        memcpy(bkp_args[bkp_index].label, txContent.arg.label, sizeof(txContent.arg.label));
        memcpy(bkp_args[bkp_index].data, txContent.arg.data, sizeof(txContent.arg.data));
        pair.item = bkp_args[bkp_index].label;
        pair.value = bkp_args[bkp_index].data;
    }
    return &pair;
}

// function called by NBGL to get the pair indexed by "index"
static nbgl_contentTagValue_t* get_multi_action_review_pair(uint8_t index) {
    static char review_action[10] = {0};

    explicit_bzero(&pair, sizeof(pair));
    if (index == 0) {
        pair.item = "Review action";
        snprintf(review_action,
                 sizeof(review_action),
                 "%d of %d",
                 txProcessingCtx.currentActionIndex,
                 txProcessingCtx.currentActionNumber);
        pair.value = review_action;
        pair.centeredInfo = 1;
        pair.valueIcon = &C_stax_app_eos_64px;
        return &pair;
    }
    return get_single_action_review_pair(index - 1);
}

static void review_choice_single(bool confirm) {
    if (confirm) {
        user_action_sign_flow_ok();
    } else {
        user_action_tx_cancel();
    }
}

static void review_choice_multi(bool confirm) {
    if (confirm) {
        if (txProcessingCtx.currentActionIndex == txProcessingCtx.currentActionNumber) {
            nbgl_useCaseReviewStreamingFinish("Sign transaction", review_choice_single);
        } else {
            user_action_sign_flow_ok();
        }
    } else {
        user_action_tx_cancel();
    }
}

void ui_display_single_action_sign_flow(void) {
    explicit_bzero(&pairList, sizeof(pairList));

    if (txProcessingCtx.currentActionNumber == 1) {
        pairList.nbPairs = txContent.argumentCount + 2;
        pairList.callback = get_single_action_review_pair;

        nbgl_useCaseReview(TYPE_TRANSACTION,
                           &pairList,
                           &C_stax_app_eos_64px,
                           "Review transaction",
                           NULL,
                           "Sign transaction",
                           review_choice_single);
    } else {
        pairList.nbPairs = txContent.argumentCount + 3;
        pairList.callback = get_multi_action_review_pair;

        nbgl_useCaseReviewStreamingContinue(&pairList, review_choice_multi);
    }
}

void ui_display_action_sign_done(parserStatus_e status, bool validated) {
    if (status == STREAM_FINISHED) {
        if (validated) {
            nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_SIGNED, ui_idle);
        } else {
            nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED, ui_idle);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void ui_display_multiple_action_sign_flow(void) {
    static char review_subtitle[20] = {0};

    snprintf(review_subtitle,
             sizeof(review_subtitle),
             "With %d actions",
             txProcessingCtx.currentActionNumber);
    nbgl_useCaseReviewStreamingStart(TYPE_TRANSACTION,
                                     &C_stax_app_eos_64px,
                                     "Review transaction",
                                     review_subtitle,
                                     review_choice_single);
}

#endif
