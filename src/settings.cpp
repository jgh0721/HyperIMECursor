#include "stdafx.h"
#include "settings.hpp"

Q_GLOBAL_STATIC_WITH_ARGS( QSettings, Settings, ( QString( "%1/%2" ).arg( qApp->applicationDirPath() ).arg( SETTINGS_FILE ), QSettings::IniFormat ));

QSettings* GetSettings()
{
    return Settings;
}
