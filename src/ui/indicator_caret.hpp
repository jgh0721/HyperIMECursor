#pragma once

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// https://github.com/fanlumaster/FullIME

class UiIndicator_Caret : public QDialog
{
    Q_OBJECT

public:
    explicit UiIndicator_Caret( QWidget* parent = nullptr );
    ~UiIndicator_Caret() override;

    void                                SetIMEMode( bool IsKoreanMode );
    void                                Show();
    void                                Hide();

private:
    Q_INVOKABLE void                    updateStatus();
    QPoint                              retrieveCaretPosition();
    bool                                retrievePositionByW32API( QPoint& Pt, DWORD ThreadId );
    bool                                retrievePositionByAccessible( QPoint& Pt, DWORD ThreadId );
    bool                                retrievePositionByUIA( QPoint& Pt );
    bool                                retrievePositionByUIA2( QPoint& Pt );

    QLabel*                             m_label = nullptr;
    QTimer*                             m_timer = nullptr;
    CComPtr<IUIAutomation>              m_pAutomation;
};

///////////////////////////////////////////////////////////////////////////////
///
///
