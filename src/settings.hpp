#pragma once

#include <QSettings>

#define SETTINGS_FILE                                   "HyperIMEIndicator.ini"

#define OPTION_DETECT_GROUP                             "DetectGroup"
#define OPTION_DETECT_ATTACH_THREAD_INPUT               OPTION_DETECT_GROUP "/AttachThreadInput"
#define OPTION_DETECT_ATTACH_THREAD_INPUT_DEFAULT       false
#define OPTION_DETECT_SENDMESSAGE                       OPTION_DETECT_GROUP "/SendMessage"
#define OPTION_DETECT_SENDMESSAGE_DEFAULT               true

#define OPTION_NOTIFY_GROUP                             "NotifyGroup"
#define OPTION_NOTIFY_SHOW_CURSOR                       OPTION_NOTIFY_GROUP "/OnCursor"
#define OPTION_NOTIFY_SHOW_CURSOR_DEFAULT               false
#define OPTION_NOTIFY_SHOW_CARET                        OPTION_NOTIFY_GROUP "/OnCaret"
#define OPTION_NOTIFY_SHOW_CARET_DEFAULT                false
#define OPTION_NOTIFY_SHOW_POPUP                        OPTION_NOTIFY_GROUP "/OnPopup"
#define OPTION_NOTIFY_SHOW_POPUP_DEFAULT                true
#define OPTION_NOTIFY_SHOW_BORDER                       OPTION_NOTIFY_GROUP "/OnBorder"
#define OPTION_NOTIFY_SHOW_BORDER_DEFAULT               false
#define OPTION_NOTIFY_SHOW_NOTIFICATION_ICON            OPTION_NOTIFY_GROUP "/ByNotificationIcon"
#define OPTION_NOTIFY_SHOW_NOTIFICATION_ICON_DEFAULT    false

QSettings* GetSettings();
