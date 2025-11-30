#include "stdafx.h"
#include "opt.hpp"

#include "settings.hpp"

UiOpt::UiOpt( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
{
    Ui.setupUi( this );

    setWindowTitle( tr( "HyperIMEIndicator 옵션" ) );
    LoadSettings();
}

void UiOpt::LoadSettings()
{
    const auto Settings = GetSettings();

    Settings->beginGroup( OPTION_DETECT_GROUP );
    const auto DetectAttachThreadInput = Settings->value( OPTION_DETECT_ATTACH_THREAD_INPUT, OPTION_DETECT_ATTACH_THREAD_INPUT_DEFAULT ).toBool();
    const auto DetectSendMessage = Settings->value( OPTION_DETECT_SENDMESSAGE, OPTION_DETECT_SENDMESSAGE_DEFAULT ).toBool();
    if( DetectAttachThreadInput )
        Ui.rdoDetectMethod_A->setChecked( true );
    if( DetectSendMessage )
        Ui.rdoDetectMethod_B->setChecked( true );
    Settings->endGroup();

    Settings->beginGroup( OPTION_NOTIFY_GROUP );
    Ui.chkNotifyMethod_Cursor->setChecked( Settings->value(OPTION_NOTIFY_SHOW_CURSOR, OPTION_NOTIFY_SHOW_CURSOR_DEFAULT).toBool() );
    Ui.chkNotifyMethod_Caret->setChecked( Settings->value( OPTION_NOTIFY_SHOW_CARET, OPTION_NOTIFY_SHOW_CARET_DEFAULT ).toBool() );
    Ui.chkNotifyMethod_Popup->setChecked( Settings->value( OPTION_NOTIFY_SHOW_POPUP, OPTION_NOTIFY_SHOW_POPUP_DEFAULT ).toBool() );
    Ui.chkNotifyMethod_NotificationIcon->setChecked( Settings->value( OPTION_NOTIFY_SHOW_NOTIFICATION_ICON, OPTION_NOTIFY_SHOW_NOTIFICATION_ICON_DEFAULT ).toBool() );
    Settings->endGroup();

    Settings->sync();
}

void UiOpt::SaveSettings()
{
    const auto Settings = GetSettings();

    Settings->beginGroup( OPTION_DETECT_GROUP );
    Settings->setValue( OPTION_DETECT_ATTACH_THREAD_INPUT, Ui.rdoDetectMethod_A->isChecked() );
    Settings->setValue( OPTION_DETECT_SENDMESSAGE, Ui.rdoDetectMethod_B->isChecked() );
    Settings->endGroup();

    Settings->beginGroup( OPTION_NOTIFY_GROUP );
    Settings->setValue( OPTION_NOTIFY_SHOW_CURSOR, Ui.chkNotifyMethod_Cursor->isChecked() );
    Settings->setValue( OPTION_NOTIFY_SHOW_CARET, Ui.chkNotifyMethod_Caret->isChecked() );
    Settings->setValue( OPTION_NOTIFY_SHOW_POPUP, Ui.chkNotifyMethod_Popup->isChecked() );
    Settings->setValue( OPTION_NOTIFY_SHOW_NOTIFICATION_ICON, Ui.chkNotifyMethod_NotificationIcon->isChecked() );
    Settings->endGroup();

    Q_EMIT settingsChanged();
}

void UiOpt::closeEvent( QCloseEvent* Event )
{
    SaveSettings();

    QDialog::closeEvent( Event );
}
