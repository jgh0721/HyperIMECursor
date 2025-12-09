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
    void                                SetPollingMs( int Ms );
    void                                SetStyleSheet( const QString& StyleSheet );
    void                                SetCheckIME( bool IsCheck );
    void                                SetCheckNumlock( bool IsCheck );
    void                                SetOffset( const QPoint& Pt );

    void                                Show();
    void                                Hide();

private:
    Q_INVOKABLE void                    updateStatus();
    QString                             makeStatusText() const;
    QPoint                              retrieveCaretPosition();
    bool                                retrievePositionByW32API( QPoint& Pt, DWORD ThreadId );
    bool                                retrievePositionByAccessible( QPoint& Pt, DWORD ThreadId );
    bool                                retrievePositionByUIA( QPoint& Pt );
    bool                                retrievePositionByUIA2( QPoint& Pt );

    bool                                m_isCheckIME = false;
    bool                                m_isCheckNumlock = false;

    bool                                m_isChanged = true;         // 최초 한 번은 표시하기 위해 true 로 한다.
    bool                                m_isCurrentIMEMode = false;

    QLabel*                             m_label = nullptr;
    QTimer*                             m_timer = nullptr;
    QPoint                              m_offset;
    CComPtr<IUIAutomation>              m_pAutomation;
};

///////////////////////////////////////////////////////////////////////////////
///
///
