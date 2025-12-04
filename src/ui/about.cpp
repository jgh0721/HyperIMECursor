#include "stdafx.h"
#include "about.hpp"

CAbout::CAbout( QWidget* parent )
    : QDialog( parent )
{
    Ui.setupUi( this );

    setWindowIcon( QIcon( ":/res/app_icon.ico" ) );
}
