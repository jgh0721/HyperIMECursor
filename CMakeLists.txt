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

project(HyperIMEIndicator LANGUAGES C CXX VERSION 0.4.0.0)
 
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

configure_file( src/version.rc.in src/version.rc @ONLY )

set( FORMS
        src/ui/about.ui
        src/ui/opt.ui
        src/ui/opt.cpp
        src/ui/opt.hpp
        src/ui/indicator_popup.ui
        src/ui/indicator_popup.cpp
        src/ui/indicator_popup.hpp
        src/ui/indicator_caret.cpp
        src/ui/indicator_caret.hpp
        src/ui/about.cpp
        src/ui/about.hpp
)
set( SOURCES
        src/cmnUtils.cpp
        src/cmnUtils.hpp
        src/main.cpp
        src/main.hpp
        src/app.qrc
        src/stdafx.cpp
        src/imeborderindicator.cpp
        src/imeborderindicator.hpp
        src/imewindowindicator.cpp
        src/imewindowindicator.hpp
        src/settings.cpp
        src/settings.hpp
        src/app.cpp
        src/app.hpp
        src/CTaskSchedulerHelper.cpp
        src/CTaskSchedulerHelper.hpp
)
 set( RESOURCES
      src/version.rc
      src/resource.h
 )

# 실행 파일 생성
set(APP_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/src/res/app.manifest")
qt_standard_project_setup()
qt6_add_executable(HyperIMEIndicator ${SOURCES} ${FORMS} ${RESOURCES} ${APP_MANIFEST})

target_include_directories(HyperIMEIndicator PRIVATE AFTER ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_precompile_headers(HyperIMEIndicator PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h>")
target_link_libraries(HyperIMEIndicator PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
set_target_properties(HyperIMEIndicator PROPERTIES WIN32_EXECUTABLE ON)
# set_target_properties(HyperIMEIndicator PROPERTIES LINK_FLAGS "/MANIFEST:embed /MANIFESTINPUT:\"${APP_MANIFEST}\"")

if (MSVC)
    set_target_properties(HyperIMEIndicator PROPERTIES
            LINK_FLAGS "/MANIFEST:NO"
    )

    add_custom_command(TARGET HyperIMEIndicator POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/res/app.manifest"
            "$<TARGET_FILE_DIR:HyperIMEIndicator>/app.manifest"
            COMMAND mt.exe -manifest "$<TARGET_FILE_DIR:HyperIMEIndicator>/app.manifest"
            -outputresource:"$<TARGET_FILE:HyperIMEIndicator>";#1
    )
endif()

## 링크 라이브러리
#target_link_libraries(HyperIMEIndicator
#PUBLIC
#    gdiplus
#    imm32
#    comctl32
#)
#
#set_property(TARGET HyperIMEIndicator PROPERTY
#        MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
## set_property(TARGET HyperIMEIndicator PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#
## On Windows, copy the Slint DLL next to the application binary so that it's found.
#if (WIN32)
#    add_custom_command(TARGET HyperIMEIndicator POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:HyperIMEIndicator> $<TARGET_FILE_DIR:HyperIMEIndicator> COMMAND_EXPAND_LISTS)
#endif()
#
