cmake_minimum_required(VERSION 4.0)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(QT_FORCE_CMP0156_TO_VALUE NEW)

set(WINDOWS_SDK "10.0.19041.0")
set(CMAKE_SYSTEM_VERSION "${WINDOWS_SDK_VERSION}" CACHE STRING "Windows SDk Version" FORCE)
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "${WINDOWS_SDK_VERSION}" CACHE STRING "" FORCE)

set(ENV{WindowsSDKVersion} "${WINDOWS_SDK_VERSION}\\")
set(ENV{WindowsSDKLibVersion} "${WINDOWS_SDK_VERSION}\\")

project(HyperIMECursor LANGUAGES C CXX )
 
# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_compile_definitions( OEMRESOURCE )
#add_compile_definitions( NOMINMAX )
add_compile_definitions( WIN32_LEAN_AND_MEAN )
add_compile_definitions( _WIN32_WINNT=0x0601 )
add_compile_definitions( NTDDI_VERSION=0x06010000 )
add_compile_definitions( CRT_SECURE_NO_WARNINGS )
add_compile_definitions( _CRT_SECURE_NO_WARNINGS )
add_compile_definitions( _CRT_RAND_S )
add_compile_definitions( _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING )
add_compile_definitions( _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING )
add_compile_definitions( UNICODE )
add_compile_definitions( _UNICODE )

IF(DEFINED ENV{QT_PATH})
    SET(QT_SDK_DIR $ENV{QT_PATH})
ELSE()
    SET(QT_SDK_DIR D:/Qt/6.10.0/msvc2022_64 CACHE PATH "QT SDK DIR" FORCE)
ENDIF()

list(APPEND CMAKE_PREFIX_PATH ${QT_SDK_DIR})
SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/Install CACHE PATH "Installation path" FORCE)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set( FORMS
        src/ui/about.ui
        src/ui/opt.ui
)
set( SOURCES
        src/main.cpp
        src/main.hpp
        src/app.qrc
        src/stdafx.cpp
        src/imeborderindicator.cpp
        src/imeborderindicator.hpp
        src/imewindowindicator.cpp
        src/imewindowindicator.hpp
)
 set( RESOURCES
#    src/resource.h
 )

# 실행 파일 생성
qt_standard_project_setup()
qt6_add_executable(HyperIMECursor ${SOURCES} ${FORMS} ${RESOURCES} )

target_precompile_headers(HyperIMECursor PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h>")
target_link_libraries(HyperIMECursor PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
set_target_properties(HyperIMECursor PROPERTIES WIN32_EXECUTABLE ON)

## 링크 라이브러리
#target_link_libraries(HyperIMECursor
#PUBLIC
#    gdiplus
#    imm32
#    comctl32
#)
#
#set_property(TARGET HyperIMECursor PROPERTY
#        MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
## set_property(TARGET HyperIMECursor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#
## On Windows, copy the Slint DLL next to the application binary so that it's found.
#if (WIN32)
#    add_custom_command(TARGET HyperIMECursor POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:HyperIMECursor> $<TARGET_FILE_DIR:HyperIMECursor> COMMAND_EXPAND_LISTS)
#endif()
#
