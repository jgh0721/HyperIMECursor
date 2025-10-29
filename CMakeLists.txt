cmake_minimum_required(VERSION 4.0)

set(WINDOWS_SDK "10.0.19041.0")
set(CMAKE_SYSTEM_VERSION "${WINDOWS_SDK_VERSION}" CACHE STRING "Windows SDk Version" FORCE)
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "${WINDOWS_SDK_VERSION}" CACHE STRING "" FORCE)

set(ENV{WindowsSDKVersion} "${WINDOWS_SDK_VERSION}\\")
set(ENV{WindowsSDKLibVersion} "${WINDOWS_SDK_VERSION}\\")

project(HyperIMECursor LANGUAGES C CXX )

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions( OEMRESOURCE )
# add_compile_definitions( NOMINMAX )
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

set( SOURCES
        src/main.cpp
        src/main.hpp
        src/stdafx.cpp
)

# 실행 파일 생성
add_executable(HyperIMECursor WIN32 ${SOURCES})

target_precompile_headers( HyperIMECursor PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h>" )

# 링크 라이브러리
target_link_libraries(HyperIMECursor
    gdiplus
    imm32
    comctl32
)

# Windows 서브시스템 설정
set_target_properties(HyperIMECursor PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)

set_property(TARGET HyperIMECursor PROPERTY
        MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
set_property(TARGET HyperIMECursor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
