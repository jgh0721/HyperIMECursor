#pragma once

#include <string>

#include <QApplication>
#include <QSystemTrayIcon>

const auto KOREAN_MODE_ICON = ":/res/korean_icon.ico";
const auto ENGLISH_MODE_ICON = ":/res/english_icon.ico";

bool                                    IsProcessInAppContainor( HWND hWnd );

/*!
 * TSF 서비스를 이용하여 현재 입력기의 입력 모드 반환
 * @return 1 = Korean, 0 = English, -1 = Unknown
 * IsKoreanIMELayout
 */
int                                     IsKoreanIMEUsingIMM32( HWND hWnd );
int                                     IsKoreanIMEUsingKeyboardLayout( HWND hWnd );

/*!
 * 다양한 방법을 사용하여 IME 를 사용 중이고, 현재 한글 모드인지 확인
 * @return true = 한글 모드, false = 그외
 * IsKoreanIMEActive
 */
bool                                    IsKoreanModeInIME();
//
// void                                    UpdateIndicatorPosition();
// SIZE                                    GetCursorSize( HCURSOR Cursor );
// void                                    DrawIndicator( HDC hdc, bool IsKoreanMode );

