#include "stdafx.h"
#include "indicator_caret.hpp"

#include <atlbase.h>
#include <atlsafe.h>

#include "settings.hpp"

#pragma comment( lib, "Oleacc.lib" )

UiIndicator_Caret::UiIndicator_Caret( QWidget* parent )
    : QDialog( parent,
               Qt::Window | Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
               Qt::WindowTransparentForInput )
{
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    setAttribute( Qt::WA_ShowWithoutActivating );
    setAttribute( Qt::WA_TranslucentBackground );
    setAttribute( Qt::WA_TransparentForMouseEvents );

    ( void )winId();
    auto layout = new QVBoxLayout( this );
    setLayout( layout );

    layout->setContentsMargins( 0, 0, 0, 0 );

    m_label = new QLabel( this );
    m_label->setStyleSheet( OPTION_ENGINE_CARET_STYLESHEET_DEFAULT );
    m_label->adjustSize();
    layout->addWidget( m_label );

    resize( m_label->size() );

    m_timer = new QTimer( this );
    connect( m_timer, &QTimer::timeout, this, &UiIndicator_Caret::updateStatus );

    HRESULT hr = CoCreateInstance( CLSID_CUIAutomation, NULL,
                                   CLSCTX_INPROC_SERVER, IID_IUIAutomation,
                                   reinterpret_cast< void** >( &m_pAutomation ) );
}

UiIndicator_Caret::~UiIndicator_Caret()
{
    m_pAutomation.Release();
    CoUninitialize();
}

void UiIndicator_Caret::SetIMEMode( bool IsKoreanMode )
{
    m_isChanged = m_isCurrentIMEMode != IsKoreanMode;
    m_isCurrentIMEMode = IsKoreanMode;
}

void UiIndicator_Caret::SetPollingMs( int Ms )
{
    m_timer->stop();
    m_timer->setInterval( Ms );
}

void UiIndicator_Caret::SetStyleSheet( const QString& StyleSheet )
{
    m_label->setStyleSheet( StyleSheet );
}

void UiIndicator_Caret::SetCheckIME( bool IsCheck )
{
    m_isCheckIME = IsCheck;
}

void UiIndicator_Caret::SetCheckNumlock( bool IsCheck )
{
    m_isCheckNumlock = IsCheck;
}

void UiIndicator_Caret::Show()
{
    if( m_timer->isActive() == false )
        m_timer->start( GET_VALUE( OPTION_ENGINE_CARET_POLLING_MS ).toInt() );
}

void UiIndicator_Caret::Hide()
{
    m_timer->stop();
    hide();
}

void UiIndicator_Caret::updateStatus()
{
    static QPoint PrevCaretPos = {};
    static bool PrevNumLockOn = false;

    auto       CaretPos    = retrieveCaretPosition();
    const bool IsNumlockOn = ((( SHORT )GetKeyState( VK_NUMLOCK )) & 0x0001) != 0;

    if( PrevNumLockOn != IsNumlockOn )
    {
        if( m_isCheckNumlock == true )
            m_isChanged = true;

        PrevNumLockOn = IsNumlockOn;
    }

    if( PrevCaretPos == CaretPos && m_isChanged == false )
        return;

    if( PrevCaretPos != CaretPos )
        nsCmn::PrintDebugString( nsCmn::Format( L"Caret Pos = %d|%d", CaretPos.x(), CaretPos.y() ) );

    PrevCaretPos = CaretPos;

    if( CaretPos.x() == 0 && CaretPos.y() == 0 )
    {
        hide();
        return;
    }

    QScreen* CurScreen = QGuiApplication::screenAt( CaretPos );

    if( !CurScreen )
        CurScreen = QGuiApplication::primaryScreen();

    if( !CurScreen )
    {
        hide();
        return;
    }

    if( CurScreen != screen() )
        setScreen( CurScreen );

    // 다중 모니터 간에 캐럿이 이동할 때의 보정 처리, 최초 위젯이 생성된 QScreen 과 다른 스크린에 캐럿이 있을 때의 처리가 필요함.
    POINT       pt   = { CaretPos.x(), CaretPos.y() };
    HMONITOR    hMon = MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );
    MONITORINFO mi   = { sizeof( MONITORINFO ) };

    QPoint      targetPos;
    const qreal dpr = screen()->devicePixelRatio();

    if( GetMonitorInfo( hMon, &mi ) )
    {
        // 1. 모니터 시작점으로부터의 물리적 오프셋(거리) 계산
        int physicalOffsetX = CaretPos.x() - mi.rcMonitor.left;
        int physicalOffsetY = CaretPos.y() - mi.rcMonitor.top;

        // 2. 오프셋만 DPR 적용 (논리 크기로 변환)
        int logicalOffsetX = static_cast< int >( physicalOffsetX / dpr );
        int logicalOffsetY = static_cast< int >( physicalOffsetY / dpr );

        // 3. Qt Screen의 논리적 시작점(Geometry TopLeft)에 더함
        // Qt의 CurScreen->geometry()는 이미 논리 좌표계입니다.
        targetPos = CurScreen->geometry().topLeft() + QPoint( logicalOffsetX, logicalOffsetY );
    }
    else
    {
        // Fallback: 정보 획득 실패 시 단순 변환 (단일 모니터 가정)
        targetPos = QPoint( static_cast< int >( CaretPos.x() / dpr ), static_cast< int >( CaretPos.y() / dpr ) );
    }

    move(targetPos);

    m_label->setText( makeStatusText() );
    m_label->adjustSize();
    show();
}

QString UiIndicator_Caret::makeStatusText() const
{
    const bool IsNumlockOn = ((( SHORT )GetKeyState( VK_NUMLOCK )) & 0x0001) != 0;

    return QString( "%1%2%3" ).arg( m_isCheckIME == true ? m_isCurrentIMEMode ? "한" : "Aa" : "" )
                              .arg( m_isCheckIME == true && m_isCheckNumlock == true ? "|" : "" )
                              .arg( m_isCheckNumlock == true ? IsNumlockOn ? "NumOn" : "NumOff" : "" );
}

QPoint UiIndicator_Caret::retrieveCaretPosition()
{
    QPoint CaretPos = {};

    do
    {
        const auto hWnd = GetForegroundWindow();
        if( hWnd == nullptr )
            break;

        const auto TargetThreadId = GetWindowThreadProcessId( hWnd, nullptr );
        if( retrievePositionByW32API( CaretPos, TargetThreadId ) == true )
            break;

        if( retrievePositionByAccessible( CaretPos, TargetThreadId ) == true )
            break;

        if( retrievePositionByUIA( CaretPos ) == true )
            break;

    } while( false );

    return CaretPos;
}

bool UiIndicator_Caret::retrievePositionByW32API( QPoint& Pt, DWORD ThreadId )
{
    bool          IsSuccess = false;
    GUITHREADINFO GuiInfo   = { 0 };
    GuiInfo.cbSize          = sizeof( GUITHREADINFO );

    do
    {
        if( GetGUIThreadInfo( ThreadId, &GuiInfo ) == FALSE )
            break;

        if( GuiInfo.rcCaret.right <= 0 && GuiInfo.rcCaret.bottom <= 0 )
            break;

        POINT caretPt = { GuiInfo.rcCaret.right, GuiInfo.rcCaret.bottom };
        ClientToScreen( GuiInfo.hwndCaret, &caretPt );

        Pt        = QPoint( caretPt.x, caretPt.y + 5 );
        IsSuccess = true;
    } while( false );

    return IsSuccess;
}

bool UiIndicator_Caret::retrievePositionByAccessible( QPoint& Pt, DWORD ThreadId )
{
    bool          IsSuccess = false;
    GUITHREADINFO GuiInfo   = { 0 };
    GuiInfo.cbSize          = sizeof( GUITHREADINFO );
    IAccessible* Accessible = nullptr;

    do
    {
        if( GetGUIThreadInfo( ThreadId, &GuiInfo ) == FALSE )
            break;

        auto Ret = AccessibleObjectFromWindow( GuiInfo.hwndFocus, OBJID_CARET, IID_IAccessible,
                                               ( void** )&Accessible );

        if( FAILED( Ret ) )
            break;

        typedef struct
        {
            long x;
            long y;
            long w;
            long h;
        } CRect;

        CRect   rect = {};
        VARIANT varCaret;

        varCaret.vt   = VT_I4;
        varCaret.lVal = CHILDID_SELF;

        Ret = Accessible->accLocation( &rect.x, &rect.y, &rect.w, &rect.h, varCaret );
        if( FAILED( Ret ) )
            break;

        // 이 8을 추가하는 것은 주로 VSCode의 커서 캡처가 vim 플러그인을 사용할 때 때때로 부정확할 수 있는 문제를 해결하기 위한 것이다
        if( rect.x != 0 && rect.y != 0 )
        {
            Pt.setX( rect.x + 8 );
            Pt.setY( rect.y + rect.h );

            IsSuccess = true;
        }
    } while( false );

    if( Accessible != nullptr )
        Accessible->Release();

    return IsSuccess;
}

bool UiIndicator_Caret::retrievePositionByUIA( QPoint& Pt )
{
    bool IsSuccess = false;

    do
    {
        if( m_pAutomation == nullptr )
            break;

        CComPtr< IUIAutomationElement > pFocused;
        // 현재 포커스된 요소 가져오기
        HRESULT hr = m_pAutomation->GetFocusedElement( &pFocused );
        if( FAILED( hr ) || !pFocused ) return false;

        // 해당 요소가 텍스트 패턴(TextPattern)을 지원하는지 확인
        CComPtr< IUIAutomationTextPattern >  pTextPattern;
        CComPtr< IUIAutomationValuePattern > pValuePattern;
        hr = pFocused->GetCurrentPatternAs( UIA_TextPatternId, IID_PPV_ARGS( &pTextPattern ) );

        if( FAILED( hr ) || !pTextPattern )
        {
            // 텍스트 패턴이 없으면 ValuePattern 등을 시도할 수도 있지만,
            // 캐럿 위치는 보통 TextPattern의 GetSelection으로 얻습니다.
            break;
        }

        // 현재 선택 영역(캐럿 포함) 가져오기
        CComPtr< IUIAutomationTextRangeArray > pRanges;
        hr = pTextPattern->GetSelection( &pRanges );
        if( FAILED( hr ) || !pRanges )
            break;

        int count = 0;
        pRanges->get_Length( &count );
        if( count <= 0 )
            break;

        // 첫 번째 범위(보통 캐럿 위치) 가져오기
        CComPtr< IUIAutomationTextRange > pRange;
        hr = pRanges->GetElement( 0, &pRange );
        if( FAILED( hr ) || !pRange )
            break;

        // 범위의 경계 사각형(Bounding Rect) 가져오기
        // 이 값은 화면(Screen) 좌표계입니다.
        CComSafeArray< double > rects;
        hr = pRange->GetBoundingRectangles( rects.GetSafeArrayPtr() );
        if( FAILED( hr ) || rects == nullptr )
            break;

        if( FAILED( SafeArrayLock( rects ) ) )
            break;

        if( rects.GetCount() < 4 )
        {
            hr = pRange->ExpandToEnclosingUnit( TextUnit_Character );
            if( FAILED( hr ) )
                break;

            rects.Destroy();
            hr = pRange->GetBoundingRectangles( rects.GetSafeArrayPtr() );
            if( FAILED( hr ) || rects == nullptr )
                break;
        }

        // rects 배열은 [L, T, W, H] 순서로 들어있음
        if( SUCCEEDED( SafeArrayLock( rects ) ) &&
            rects.GetCount() >= 4 )
        {
            double left   = rects.GetAt( 0 );
            double top    = rects.GetAt( 1 );
            double width  = rects.GetAt( 2 );
            double height = rects.GetAt( 3 );

            if( width == 0 && height == 0 )
            {
                pRange->ExpandToEnclosingUnit( TextUnit_Character );
                if( FAILED( hr ) )
                    break;

                rects.Destroy();
                hr = pRange->GetBoundingRectangles( rects.GetSafeArrayPtr() );
                if( FAILED( hr ) || rects == nullptr )
                    break;
            }

            // 값이 유효한지 체크 (화면 밖이거나 0인 경우 제외)
            if( width >= 0 && height >= 0 )
            {
                // 캐럿의 하단 중앙 혹은 좌하단 위치 계산
                Pt.setX( static_cast< LONG >( left ) );
                Pt.setY( static_cast< LONG >( top + height ) );
                IsSuccess = true;
            }
        }
    } while( false );

    return IsSuccess;
}

bool UiIndicator_Caret::retrievePositionByUIA2( QPoint& Pt )
{
    long  curX = 0, curY = 0, curW = 0, curH = 0;
    long* pX   = &curX;
    long* pY   = &curY;
    long* pW   = &curW;
    long* pH   = &curH;
    Pt.setX( *pX + *pW );
    Pt.setY( *pY + *pH );
    CComPtr< IUIAutomation >             uia;
    CComPtr< IUIAutomationElement >      eleFocus;
    CComPtr< IUIAutomationValuePattern > valuePattern;
    if( S_OK != uia.CoCreateInstance( CLSID_CUIAutomation ) || uia == nullptr )
    {
        Pt.setX( *pX + *pW );
        Pt.setY( *pY + *pH );
        return Pt.x() > 0 || Pt.y() > 0;
    }
    if( S_OK != uia->GetFocusedElement( &eleFocus ) || eleFocus == nullptr )
    {
        goto useAccLocation;
    }
    if( S_OK == eleFocus->GetCurrentPatternAs( UIA_ValuePatternId, IID_PPV_ARGS( &valuePattern ) ) &&
        valuePattern != nullptr )
    {
        BOOL isReadOnly;
        if( S_OK == valuePattern->get_CurrentIsReadOnly( &isReadOnly ) && isReadOnly )
        {
            Pt.setX( *pX + *pW );
            Pt.setY( *pY + *pH );
            return Pt.x() > 0 || Pt.y() > 0;
        }
    }
useAccLocation:
    // use IAccessible::accLocation
    GUITHREADINFO guiThreadInfo = { sizeof(guiThreadInfo) };
    HWND hwndFocus = GetForegroundWindow();
    GetGUIThreadInfo( GetWindowThreadProcessId( hwndFocus, nullptr ), &guiThreadInfo );
    hwndFocus = guiThreadInfo.hwndFocus ? guiThreadInfo.hwndFocus : hwndFocus;
    CComPtr< IAccessible > accCaret;
    if( S_OK == AccessibleObjectFromWindow( hwndFocus, OBJID_CARET, IID_PPV_ARGS( &accCaret ) ) && accCaret != nullptr )
    {
        CComVariant varChild = CComVariant( 0 );
        if( S_OK == accCaret->accLocation( pX, pY, pW, pH, varChild ) )
        {
            Pt.setX( *pX + *pW );
            Pt.setY( *pY + *pH );
            return Pt.x() > 0 || Pt.y() > 0;
        }
    }
    if( eleFocus == nullptr )
    {
        Pt.setX( *pX + *pW );
        Pt.setY( *pY + *pH );
        return Pt.x() > 0 || Pt.y() > 0;
    }
    // use IUIAutomationTextPattern2::GetCaretRange
    CComPtr< IUIAutomationTextPattern2 > textPattern2;
    CComPtr< IUIAutomationTextRange >    caretTextRange;
    CComSafeArray< double >              rects;
    void*                                pVal     = nullptr;
    BOOL                                 IsActive = FALSE;
    if( S_OK != eleFocus->GetCurrentPatternAs( UIA_TextPattern2Id, IID_PPV_ARGS( &textPattern2 ) ) ||
        textPattern2 == nullptr )
    {
        goto useGetSelection;
    }
    if( S_OK != textPattern2->GetCaretRange( &IsActive, &caretTextRange ) || caretTextRange == nullptr || !IsActive )
    {
        goto useGetSelection;
    }
    if( S_OK == caretTextRange->GetBoundingRectangles( rects.GetSafeArrayPtr() ) && rects != nullptr &&
        SUCCEEDED( SafeArrayLock(rects) ) && rects.GetCount() >= 4 )
    {
        *pX = long( rects[ 0 ] );
        *pY = long( rects[ 1 ] );
        *pW = long( rects[ 2 ] );
        *pH = long( rects[ 3 ] );
        Pt.setX( *pX + *pW );
        Pt.setY( *pY + *pH );
        return Pt.x() > 0 || Pt.y() > 0;
    }
useGetSelection:
    CComPtr< IUIAutomationTextPattern > textPattern;
    CComPtr< IUIAutomationTextRangeArray > selectionRangeArray;
    CComPtr< IUIAutomationTextRange >      selectionRange;
    if( textPattern2 == nullptr )
    {
        if( S_OK != eleFocus->GetCurrentPatternAs( UIA_TextPatternId, IID_PPV_ARGS( &textPattern ) ) ||
            textPattern == nullptr )
        {
            Pt.setX( *pX + *pW );
            Pt.setY( *pY + *pH );
            return Pt.x() > 0 || Pt.y() > 0;
        }
    }
    else
    {
        textPattern = textPattern2;
    }
    if( S_OK != textPattern->GetSelection( &selectionRangeArray ) || selectionRangeArray == nullptr )
    {
        Pt.setX( *pX + *pW );
        Pt.setY( *pY + *pH );
        return Pt.x() > 0 || Pt.y() > 0;
    }
    int length = 0;
    if( S_OK != selectionRangeArray->get_Length( &length ) || length <= 0 )
    {
        Pt.setX( *pX + *pW );
        Pt.setY( *pY + *pH );
        return Pt.x() > 0 || Pt.y() > 0;
    }
    if( S_OK != selectionRangeArray->GetElement( 0, &selectionRange ) || selectionRange == nullptr )
    {
        Pt.setX( *pX + *pW );
        Pt.setY( *pY + *pH );
        return Pt.x() > 0 || Pt.y() > 0;
    }
    if( S_OK != selectionRange->GetBoundingRectangles( rects.GetSafeArrayPtr() ) || rects == nullptr ||
        FAILED( SafeArrayLock(rects) ) )
    {
        Pt.setX( *pX + *pW );
        Pt.setY( *pY + *pH );
        return Pt.x() > 0 || Pt.y() > 0;
    }
    if( rects.GetCount() < 4 )
    {
        if( S_OK != selectionRange->ExpandToEnclosingUnit( TextUnit_Character ) )
        {
            Pt.setX( *pX + *pW );
            Pt.setY( *pY + *pH );
            return Pt.x() > 0 || Pt.y() > 0;
        }
        if( S_OK != selectionRange->GetBoundingRectangles( rects.GetSafeArrayPtr() ) || rects == nullptr ||
            FAILED( SafeArrayLock(rects) ) || rects.GetCount() < 4 )
        {
            Pt.setX( *pX + *pW );
            Pt.setY( *pY + *pH );
            return Pt.x() > 0 || Pt.y() > 0;
        }
    }
    *pX = long( rects[ 0 ] );
    *pY = long( rects[ 1 ] );
    *pW = long( rects[ 2 ] );
    *pH = long( rects[ 3 ] );

    /* caretPos.first = *pX + *pW; */
    Pt.setX( *pX );
    Pt.setY( *pY + *pH );
    return Pt.x() > 0 || Pt.y() > 0;
}
