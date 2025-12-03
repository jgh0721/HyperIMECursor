#include "stdafx.h"
#include "indicator.hpp"

UiIndicator::UiIndicator( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
{
    setAttribute( Qt::WA_ShowWithoutActivating );
    setAttribute( Qt::WA_TransparentForMouseEvents );

    Ui.setupUi( this );

    Timer = new QTimer( this );
    Timer->setSingleShot( true );
    connect( Timer, &QTimer::timeout, this, &UiIndicator::hide );

    (void)winId();
}

void UiIndicator::SetIMEMode( bool IsKoreanMode )
{
    QString Label;

    if( IsKoreanMode )
        Label = "<font size=5 color=\"red\">한글</font>";
    else
        Label = "<font size=5 color=\"blue\">AAaa</font>";

    Ui.lblStatusText->setText( Label );
}

void UiIndicator::Show( int WaitSecs )
{
    if( WaitSecs > 0 )
    {
        if( Timer->isActive() == true )
            Timer->stop();

        Timer->start( WaitSecs * 1000 );
    }

    // 위젯 크기
    const int widgetWidth = size().width();
    const int widgetHeight = size().height();

    QPoint cursorPos = QCursor::pos();

    QScreen* screen = QGuiApplication::screenAt(cursorPos);

    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (!screen) {
        return;
    }

    QRect availableGeometry = screen->availableGeometry();

    const int x = availableGeometry.left() + (availableGeometry.width() - widgetWidth) / 2;
    const int y = availableGeometry.bottom() - widgetHeight - 50;  // bottom()은 inclusive

    setScreen( screen );
    setFixedSize(widgetWidth, widgetHeight);
    move(x, y);

    show();
}
