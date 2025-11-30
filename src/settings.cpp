#include "stdafx.h"
#include "settings.hpp"

Q_GLOBAL_STATIC_WITH_ARGS( QSettings, Settings, ( SETTINGS_FILE, QSettings::IniFormat ) );

QSettings* GetSettings()
{
    return Settings;
}
