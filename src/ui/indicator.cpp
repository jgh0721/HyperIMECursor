#include "stdafx.h"
#include "indicator.hpp"

UiIndicator::UiIndicator( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
{
    Ui.setupUi( this );

}
