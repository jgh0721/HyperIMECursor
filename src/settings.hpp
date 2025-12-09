#pragma once

#include <QSettings>

extern HINSTANCE            hInstance;
extern std::atomic<bool>    IsKoreanMode;
extern std::atomic<bool>    IsKoreanModeOnHook;
extern uint64_t             IMEActiveCheckTime;

#define SETTINGS_FILE                                   "HyperIMEIndicator.ini"
#ifdef _DEBUG
#define SETTINGS_TASK_NAME                              L"HyperIMEIndicator Launch (DEBUG)"
#else
#define SETTINGS_TASK_NAME                              L"HyperIMEIndicator Launch"
#endif

#define DEFAULT_BUILTIN_FONT_FAMILY                     "Sarasa Mono K"

#define OPTION_DETECT_GROUP                             "Engine"
#define OPTION_DETECT_ATTACH_THREAD_INPUT               OPTION_DETECT_GROUP "/AttachThreadInput"
#define OPTION_DETECT_ATTACH_THREAD_INPUT_DEFAULT       false
#define OPTION_DETECT_SENDMESSAGE                       OPTION_DETECT_GROUP "/SendMessage"
#define OPTION_DETECT_SENDMESSAGE_DEFAULT               true
#define OPTION_DETECT_KEYBOARD_HOOK                     OPTION_DETECT_GROUP "/KeyboardHook"
#define OPTION_DETECT_KEYBOARD_HOOK_DEFAULT             false
#define OPTION_DETECT_DETECT_POLLING_MS                 OPTION_DETECT_GROUP "/PollingMs"
#define OPTION_DETECT_DETECT_POLLING_MS_DEFAULT         1000

#define OPTION_STARTUP_GROUP                            "AutoStart"
#define OPTION_STARTUP_START_ON_WINDOWS_BOOT            OPTION_STARTUP_GROUP "/StartOnWindowsBoot"
#define OPTION_STARTUP_START_ON_WINDOWS_BOOT_DEFAULT    false
#define OPTION_STARTUP_START_AS_ADMIN_ON_WINDOWS_BOOT   OPTION_STARTUP_GROUP "/StartAsAdminOnWindowsBoot"
#define OPTION_STARTUP_START_AS_ADMIN_ON_WINDOWS_BOOT_DEFAULT false

#define OPTION_ENGINE_CARET_GROUP                       "EngineCaret"
#define OPTION_ENGINE_CARET_IS_USE                      OPTION_ENGINE_CARET_GROUP "/Use"
#define OPTION_ENGINE_CARET_IS_USE_DEFAULT              true
#define OPTION_ENGINE_CARET_IS_CHECK_IME                OPTION_ENGINE_CARET_GROUP "/CheckIme"
#define OPTION_ENGINE_CARET_IS_CHECK_IME_DEFAULT        true
#define OPTION_ENGINE_CARET_IS_CHECK_NUMLOCK            OPTION_ENGINE_CARET_GROUP "/CheckNumlock"
#define OPTION_ENGINE_CARET_IS_CHECK_NUMLOCK_DEFAULT    false
#define OPTION_ENGINE_CARET_POLLING_MS                  OPTION_ENGINE_CARET_GROUP "/PollingMs"
#define OPTION_ENGINE_CARET_POLLING_MS_DEFAULT          100
#define OPTION_ENGINE_CARET_OFFSET_X                    OPTION_ENGINE_CARET_GROUP "/OffsetX"
#define OPTION_ENGINE_CARET_OFFSET_X_DEFAULT            10
#define OPTION_ENGINE_CARET_OFFSET_Y                    OPTION_ENGINE_CARET_GROUP "/OffsetY"
#define OPTION_ENGINE_CARET_OFFSET_Y_DEFAULT            6

#define OPTION_ENGINE_CARET_STYLESHEET                  OPTION_ENGINE_CARET_GROUP "/StyleSheet"
#define OPTION_ENGINE_CARET_STYLESHEET_DEFAULT          R"(QLabel {
    background-color: rgba( 0, 0, 0, 150 );
    color: white;
    border: 1px solid gray;
    border-radius: 6px;
    padding: 4px;
    font-weight: semi-bold;
    font-size: 13px;
})"

#define OPTION_ENGINE_POPUP_GROUP                       "EnginePopup"
#define OPTION_ENGINE_POPUP_IS_USE                      OPTION_ENGINE_POPUP_GROUP "/Use"
#define OPTION_ENGINE_POPUP_IS_USE_DEFAULT              false

#define GET_VALUE( Value ) \
    GetSettings()->value( Value, Value##_DEFAULT )

#define SET_VALUE( Key, Value ) \
    GetSettings()->setValue( Key, Value )

QSettings* GetSettings();
