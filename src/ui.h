#pragma once


void ui_idle(void);
void ui_display_public_key_flow(void);
void ui_display_public_key_done(bool validated);
void ui_display_single_action_sign_flow(void);
void ui_display_multiple_action_sign_flow(void);
void ui_display_action_sign_done(parserStatus_e status, bool validated);
