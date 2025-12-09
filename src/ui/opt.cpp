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

    Settings->sync();

    Ui.rdoIME_Attach->setChecked( GET_VALUE( OPTION_DETECT_ATTACH_THREAD_INPUT ).toBool() );
    Ui.rdoIME_SendMessage->setChecked( GET_VALUE( OPTION_DETECT_SENDMESSAGE ).toBool() );
    Ui.chkIME_KeyboardHook->setChecked( GET_VALUE( OPTION_DETECT_KEYBOARD_HOOK ).toBool() );
    Ui.spbIMEPollingMs->setValue( GET_VALUE( OPTION_DETECT_DETECT_POLLING_MS ).toInt() );

    Ui.chkIsRunOnStartup->setChecked( GET_VALUE( OPTION_STARTUP_START_ON_WINDOWS_BOOT ).toBool() );
    Ui.chkIsRunAsAdminOnStartup->setChecked( GET_VALUE( OPTION_STARTUP_START_AS_ADMIN_ON_WINDOWS_BOOT ).toBool() );

    Ui.chkIsCaret->setChecked( GET_VALUE( OPTION_ENGINE_CARET_IS_USE ).toBool() );
    Ui.chkIsCaretIME->setChecked( GET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_IME ).toBool() );
    Ui.chkIsCaretNumlock->setChecked( GET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_NUMLOCK ).toBool() );
    Ui.spbCaretPollingMs->setValue( GET_VALUE( OPTION_ENGINE_CARET_POLLING_MS ).toInt() );
    Ui.edtCaretStyleSheet->setPlainText( GET_VALUE( OPTION_ENGINE_CARET_STYLESHEET ).toString() );
    Ui.spbCaretOffsetX->setValue( GET_VALUE(OPTION_ENGINE_CARET_OFFSET_X ).toInt() );
    Ui.spbCaretOffsetY->setValue( GET_VALUE(OPTION_ENGINE_CARET_OFFSET_Y ).toInt() );
}

void UiOpt::SaveSettings()
{
    const auto Settings = GetSettings();

    SET_VALUE( OPTION_DETECT_ATTACH_THREAD_INPUT, Ui.rdoIME_Attach->isChecked() );
    SET_VALUE( OPTION_DETECT_SENDMESSAGE, Ui.rdoIME_Attach->isChecked() );
    SET_VALUE( OPTION_DETECT_KEYBOARD_HOOK, Ui.rdoIME_Attach->isChecked() );
    SET_VALUE( OPTION_DETECT_DETECT_POLLING_MS, Ui.spbIMEPollingMs->value() );

    SET_VALUE( OPTION_STARTUP_START_ON_WINDOWS_BOOT, Ui.chkIsRunOnStartup->isChecked() );
    SET_VALUE( OPTION_STARTUP_START_AS_ADMIN_ON_WINDOWS_BOOT, Ui.chkIsRunAsAdminOnStartup->isChecked() );

    SET_VALUE( OPTION_ENGINE_CARET_IS_USE, Ui.chkIsCaret->isChecked() );
    SET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_IME, Ui.chkIsCaretIME->isChecked() );
    SET_VALUE( OPTION_ENGINE_CARET_IS_CHECK_NUMLOCK, Ui.chkIsCaretNumlock->isChecked() );
    SET_VALUE( OPTION_ENGINE_CARET_POLLING_MS, Ui.spbCaretPollingMs->value() );
    SET_VALUE( OPTION_ENGINE_CARET_STYLESHEET, Ui.edtCaretStyleSheet->toPlainText() );
    SET_VALUE( OPTION_ENGINE_CARET_OFFSET_X, Ui.spbCaretOffsetX->value() );
    SET_VALUE( OPTION_ENGINE_CARET_OFFSET_Y, Ui.spbCaretOffsetY->value() );

    Settings->sync();
    Q_EMIT settingsChanged();
}

void UiOpt::on_btnOK_clicked( bool checked )
{
    SaveSettings();
    Q_EMIT settingsChanged();
    done( QDialog::Accepted );
}

void UiOpt::on_btnCancel_clicked( bool checked )
{
    done( QDialog::Rejected );
}
