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

#ifdef HAVE_BAGL

#include <string.h>

#include "os.h"
#include "ux.h"

#include "glyphs.h"
#include "main.h"
#include "ui.h"
#include "config.h"
#include "state_neutral.h"

static char actionCounter[32];
static char confirmLabel[32];
static char smallConfirmLabel[16];

// display stepped screens
static unsigned int ux_step;
static unsigned int ux_step_count;

// count for multi-actions screens
static unsigned int effectiveActionIndex;

static char confirm_text1[16];
static char confirm_text2[16];

static void display_settings_flow(void);
static void switch_settings_contract_data(void);
static void switch_settings_verbose_config(void);
static void display_next_state(uint8_t state);

static void maybe_push_stack(void);

///////////////////////////COMMON FUNCTIONS////////////////////////////
static void maybe_push_stack(void) {
    // reserve a display stack slot if none yet
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
}
///////////////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(ux_idle_ready_step,
             nn,  // pnn,
             {
                 "Application",
                 "is ready",
             });
UX_STEP_NOCB(ux_display_version_step,
             bn,
             {
                 "Version",
                 APPVERSION,
             });
UX_STEP_CB(ux_settings_step,
           pb,
           display_settings_flow(),
           {
               &C_icon_coggle,
               "Settings",
           });
UX_STEP_CB(ux_quit_step,
           pb,
           os_sched_exit(-1),
           {
               &C_icon_dashboard_x,
               "Quit",
           });

UX_FLOW(ux_idle_flow,
        &ux_idle_ready_step,
        &ux_display_version_step,
        &ux_settings_step,
        &ux_quit_step);

void ui_idle(void) {
    // reserve a display stack slot if none yet
    maybe_push_stack();

    ux_flow_init(0, ux_idle_flow, NULL);
}

////////////////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(ux_abort_warning_step,
             bnn,  // pnn,
             {"Aborting", "Detected", "Unknown Trx"});

UX_FLOW(ux_abort_flow,
        &ux_abort_warning_step,
        &ux_display_version_step,
        &ux_display_version_step,
        &ux_quit_step);

void ui_abort_unknown_action(void) {
    // reserve a display stack slot if none yet
    maybe_push_stack();

    ux_flow_init(0, ux_abort_flow, NULL);
}

///////////////////////////////////////////////////////////////////////////////

UX_STEP_CB(ux_settings_flow_1_step,
           bnnn,
           switch_settings_contract_data(),
           {
               "Contract data",
               "Unknown Action",
               "in transactions",
               confirmLabel,
           });

UX_STEP_CB(ux_settings_flow_2_step,
           bnnn,
           switch_settings_verbose_config(),
           {
               "Verbose",
               "Show Details",
               "in transactions",
               smallConfirmLabel,
           });

UX_STEP_CB(ux_settings_flow_3_step,
           pb,
           ui_idle(),
           {
               &C_icon_back_x,
               "Back",
           });

UX_FLOW(ux_settings_flow,
        &ux_settings_flow_1_step,
        &ux_settings_flow_2_step,
        &ux_settings_flow_3_step);

static void display_settings_flow(void) {
    strlcpy(confirmLabel,
            (is_unknown_action_allowed() ? "Allowed" : "NOT Allowed"),
            sizeof(confirmLabel));

    strlcpy(smallConfirmLabel, (is_verbose() ? "On" : "Off"), sizeof(smallConfirmLabel));

    ux_flow_init(0, ux_settings_flow, NULL);
}

static void switch_settings_contract_data(void) {
    toogle_unknown_action_allowed();
    display_settings_flow();
}

static void switch_settings_verbose_config(void) {
    toogle_verbose_config();
    display_settings_flow();
}

///////////////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(ux_display_public_flow_1_step,
             pnn,
             {
                 &C_icon_eye,
                 "Verify",
                 "Public Key",
             });
UX_STEP_NOCB(ux_display_public_flow_2_step,
             bnnn_paging,
             {
                 .title = "Public Key",
                 .text = tmpCtx.publicKeyContext.address,
             });
UX_STEP_CB(ux_display_public_flow_3_step,
           pb,
           user_action_address_ok(),
           {
               &C_icon_validate_14,
               "Approve",
           });
UX_STEP_CB(ux_display_public_flow_4_step,
           pb,
           user_action_address_cancel(),
           {
               &C_icon_crossmark,
               "Reject",
           });

UX_FLOW(ux_display_public_flow,
        &ux_display_public_flow_1_step,
        &ux_display_public_flow_2_step,
        &ux_display_public_flow_3_step,
        &ux_display_public_flow_4_step);

void ui_display_public_key_flow(void) {
    ux_flow_init(0, ux_display_public_flow, NULL);
}

void ui_display_public_key_done(bool validated) {
    UNUSED(validated);
    // Display back the original UX
    ui_idle();
}

///////////////////////////////////////////////////////////////////////////////

// State machine constants used in display_next_state()
// These control how the UI moves between argument pages and action details.
#define STATE_LEFT_BORDER  0  // Starting boundary before first variable argument
#define STATE_VARIABLE     1  // Displaying a variable argument (paged)
#define STATE_RIGHT_BORDER 2  // Ending boundary after last variable argument

// Step 1: Intro screen telling the user they're reviewing a single action
UX_STEP_NOCB(ux_single_action_sign_flow_1_step,
             pnn,
             {
                 &C_icon_certificate,
                 "Review",
                 confirmLabel,  // Will be "Transaction" or "Action #N"
             });

// Step 2: Show the contract/account this action is targeting
UX_STEP_NOCB(ux_single_action_sign_flow_2_step,
             bn,
             {
                 "Contract",
                 txContent.contract,
             });

// Step 3: Show the action/method being invoked on the contract
UX_STEP_NOCB(ux_single_action_sign_flow_3_step,
             bn,
             {
                 "Action",
                 txContent.action,
             });

// When Verbose: Authorization Screen
UX_STEP_NOCB(ux_authorization_flow_1_step,
             bn,
             {
                 "Authorization",
                 txProcessingCtx.currentAuthorizationName,
             });

// Step 3: Show the action/method being invoked on the contract
UX_STEP_NOCB(ux_authorization_flow_2_step,
             bn,
             {
                 "Permission",
                 txProcessingCtx.currentAuthorizationPermission,
             });

// Step 4: State boundary logic before displaying the first argument
UX_STEP_INIT(ux_init_left_border, NULL, NULL, { display_next_state(STATE_LEFT_BORDER); });

// Step 5: Display a single argument (label + data), paging if necessary
UX_STEP_NOCB_INIT(ux_single_action_sign_flow_variable_step,
                  bnnn_paging,
                  { display_next_state(STATE_VARIABLE); },
                  {
                      .title = txContent.arg.label,
                      .text = txContent.arg.data,
                  });

// Step 6: State boundary logic after displaying the last argument
UX_STEP_INIT(ux_init_right_border, NULL, NULL, { display_next_state(STATE_RIGHT_BORDER); });

// Step 7: Final approve screen
UX_STEP_CB(ux_single_action_sign_flow_7_step,
           pbb,
           user_action_sign_flow_ok(),
           {
               &C_icon_validate_14,
               confirm_text1,  // "Sign" or "Accept"
               confirm_text2,  // "transaction" or "& review next"
           });

// Step 8: Final reject screen
UX_STEP_CB(ux_single_action_sign_flow_8_step,
           pbb,
           user_action_tx_cancel(),
           {
               &C_icon_crossmark,
               "Cancel",
               "signature",
           });

////////////////////////////////////////////////////////////////////////////////
// When Not Verbose: Single Action Flow
// Full multi-step review flow for a single action with arguments
UX_FLOW(ux_single_action_sign_flow,
        &ux_single_action_sign_flow_1_step,
        &ux_single_action_sign_flow_2_step,
        &ux_single_action_sign_flow_3_step,
        &ux_init_left_border,
        &ux_single_action_sign_flow_variable_step,
        &ux_init_right_border,
        &ux_single_action_sign_flow_7_step,
        &ux_single_action_sign_flow_8_step);

////////////////////////////////////////////////////////////////////////////////

UX_FLOW(ux_verbose_single_action_flow,
        &ux_single_action_sign_flow_1_step,
        &ux_single_action_sign_flow_2_step,
        &ux_single_action_sign_flow_3_step,
        &ux_authorization_flow_1_step,
        &ux_authorization_flow_2_step,
        &ux_init_left_border,
        &ux_single_action_sign_flow_variable_step,
        &ux_init_right_border,
        &ux_single_action_sign_flow_7_step,
        &ux_single_action_sign_flow_8_step);

// Helper to handle moving between argument screens and boundaries
static void display_next_state(uint8_t state) {
    if (state == STATE_LEFT_BORDER) {
        // Before first argument
        if (ux_step == 0) {  // Just entered from intro
            ux_step = 1;
            ux_flow_next();         // Move forward to first argument
        } else if (ux_step == 1) {  // Already on first argument
            --ux_step;
            ux_flow_prev();        // Go back to intro
        } else if (ux_step > 1) {  // Middle of arguments
            --ux_step;
            ux_flow_next();  // Go forward
        }
    } else if (state == STATE_VARIABLE) {
        // Display the Nth argument based on current step index
        printArgument(ux_step - 1, &txProcessingCtx);
    } else if (state == STATE_RIGHT_BORDER) {
        // After last argument
        if (ux_step < ux_step_count) {  // More arguments ahead
            ++ux_step;
            ux_flow_prev();
        } else if (ux_step == ux_step_count) {  // Just finished last argument
            ++ux_step;
            ux_flow_next();                    // Move to approval/reject
        } else if (ux_step > ux_step_count) {  // Already past last argument
            ux_step = ux_step_count;
            ux_flow_prev();
        }
    }
}

// Entry point for displaying a single action
void ui_display_single_action_sign_flow() {
    ux_step = 0;
    ux_step_count = txContent.argumentCount;
    uint8_t effectiveActions = txProcessingCtx.currentActionNumber - countStateNeutralActions;

    // Label the review screen differently for single vs. multi-action transactions
    if (effectiveActions > 1) {
        // must have previous gone to multi action flow init effectiveActionIndex
        snprintf(confirmLabel, sizeof(confirmLabel), "Action #%d", effectiveActionIndex);
    } else {
        strlcpy(confirmLabel, "Transaction", sizeof(confirmLabel));
    }

    /*
    ** defines two main states for handling transaction actions in the UI flow:
    ** State-Neutral Action State:
    **
    ** If the transaction has multiple actions, the review is skipped for state neutral actions
    ** and the action is auto-approved. If the transaction has a single state-neutral action, a
    ** short confirmation UI flow is shown with text "Sign [action]" and "From [contract]". If text
    ** formatting fails, the UI aborts with ui_abort_unknown_action().
    **
    ** Default (Non State-Neutral) Action State:
    **
    ** Triggered when the action is not state-neutral or verbose mode is enabled.
    ** If the current action is the last in the transaction, a transaction review screen is show.
    ** Otherwise, the review continues with the next action.
    ** The code also manages UI steps and transitions for reviewing single or multiple actions,
    ** with specific UX flows for each case.
    */
    if (!txProcessingCtx.isVerbose &&
        isStateNeutralAction(txContent.contract, txContent.action, txContent.noData)) {
        user_action_sign_flow_ok();
    } else {
        // UI update the state neutral action ""::identity to null::identity
        if (strcmp(txContent.contract, "") == 0 && strcmp(txContent.action, "identity") == 0) {
            strlcpy(txContent.contract, "null", sizeof(txContent.contract));
        }
        // --- Default: Full review flow : not state-neutral action ---
        if (txProcessingCtx.currentActionIndex == txProcessingCtx.currentActionNumber ||
            effectiveActionIndex == effectiveActions) {
            strlcpy(confirm_text1, "Sign", sizeof(confirm_text1));
            strlcpy(confirm_text2, "transaction", sizeof(confirm_text2));
        } else {
            strlcpy(confirm_text1, "Accept", sizeof(confirm_text1));
            strlcpy(confirm_text2, "& review next", sizeof(confirm_text2));
        }
        if (txProcessingCtx.isVerbose == 1) {
            ux_flow_init(0, ux_verbose_single_action_flow, NULL);
        } else {
            ux_flow_init(0, ux_single_action_sign_flow, NULL);
        }
        effectiveActionIndex++;
    }
}

void ui_display_action_sign_done(parserStatus_e status, bool validated) {
    UNUSED(status);
    UNUSED(validated);
    // Return to idle screen after signature or cancellation
    ui_idle();
}

///////////////////////////////////////////////////////////////////////////////

UX_FLOW_DEF_NOCB(ux_multiple_action_sign_flow_1_step,
                 pnn,
                 {
                     &C_icon_certificate,
                     "Review",
                     "Transaction",
                 });
UX_FLOW_DEF_NOCB(ux_multiple_action_sign_flow_2_step,
                 bn,  // pnn,
                 {
                     "With",
                     actionCounter,
                 });
UX_FLOW_DEF_VALID(ux_multiple_action_sign_flow_3_step,
                  pbb,
                  user_action_sign_flow_ok(),
                  {&C_icon_validate_14, "Continue", "review"});
UX_FLOW_DEF_VALID(ux_multiple_action_sign_flow_4_step,
                  pbb,
                  user_action_tx_cancel(),
                  {
                      &C_icon_crossmark,
                      "Cancel",
                      "review",
                  });

UX_FLOW(ux_multiple_action_sign_flow,
        &ux_multiple_action_sign_flow_1_step,
        &ux_multiple_action_sign_flow_2_step,
        &ux_multiple_action_sign_flow_3_step,
        &ux_multiple_action_sign_flow_4_step);

void ui_display_multiple_action_sign_flow(void) {
    uint8_t effectiveActions = txProcessingCtx.currentActionNumber - countStateNeutralActions;
    effectiveActionIndex = 1;

    if (effectiveActions > 1) {
        snprintf(actionCounter, sizeof(actionCounter), "%d actions", effectiveActions);
        ux_flow_init(0, ux_multiple_action_sign_flow, NULL);
    } else {
        user_action_sign_flow_ok();
        ui_display_single_action_sign_flow();
    }
}

#endif
