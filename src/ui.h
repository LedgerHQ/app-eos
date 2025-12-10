#pragma once

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define ICON_APP_EOS     C_nano_app_eos
#define ICON_APP_HOME    C_home_app_eos
#define ICON_APP_WARNING C_icon_warning
#elif defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_EOS     C_app_eos_64px
#define ICON_APP_HOME    ICON_APP_EOS
#define ICON_APP_WARNING LARGE_WARNING_ICON
#elif defined(TARGET_APEX_P)
#define ICON_APP_EOS     C_app_eos_48px
#define ICON_APP_HOME    ICON_APP_EOS
#define ICON_APP_WARNING LARGE_WARNING_ICON
#endif

void ui_idle(void);
void ui_display_public_key_flow(void);
void ui_display_public_key_done(bool validated);
void ui_display_single_action_sign_flow(void);
void ui_display_multiple_action_sign_flow(void);
void ui_display_action_sign_done(parserStatus_e status, bool validated);
