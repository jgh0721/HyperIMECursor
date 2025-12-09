#pragma once

#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#include <string>
#include <memory>
#include <stdexcept>

namespace nsCmn
{
    namespace nsWin
    {
        class CComInitializer
        {
        public:
            explicit CComInitializer( DWORD CoInit = COINIT_APARTMENTTHREADED );
            ~CComInitializer();

            CComInitializer( const CComInitializer& )            = delete;
            CComInitializer& operator=( const CComInitializer& ) = delete;

            HRESULT IsInitialized() const;

        private:
            bool initialized_ = false;
            HRESULT Result_ = S_OK;
        };

        // 관리자 권한 관련 유틸리티
        class PrivilegeHelper
        {
        public:
            // 현재 프로세스가 관리자 권한으로 실행 중인지 확인
            static bool IsRunningAsAdmin();

            // 관리자 권한으로 프로세스 재시작
            static bool RestartAsAdmin( const std::wstring& FileFullPath, const std::wstring& CMDLine = L"" );

            // UAC 권한 상승 요청
            static bool RequestAdminPrivilege( const std::wstring& FileFullPath, const std::wstring& CMDLine = L"" );

            // 토큰 권한 활성화
            static bool EnablePrivilege( const std::wstring& PrivilegeName );
        };

        // 현재 사용자 이름 가져오기
        std::wstring GetCurrentUserName();

    }

    class CTaskSchedulerHelper
    {
    public:
        // 작업 트리거 타입
        enum class TriggerType
        {
            Logon,      // 사용자 로그온 시
            Boot,       // 시스템 부팅 시
            Daily,      // 매일
            Weekly,     // 매주
            Monthly     // 매월
        };

        // 작업 실행 레벨
        enum class RunLevel
        {
            Default,
            LUA,        // Least-privileged user account (일반 사용자)
            Highest     // 최고 권한 (관리자)
        };

        struct TaskSettings
        {
            std::wstring                TaskName;          // 작업 이름
            std::wstring                TaskPath;          // 작업 경로 (폴더)
            std::wstring                Description;       // 작업 설명

            std::wstring                FileFullPath;    // 실행할 프로그램 경로
            std::wstring                CMDLine;         // 프로그램 인자
            std::wstring                WorkingDirectory;  // 작업 디렉토리
            TriggerType                 Trigger;       // 트리거 타입
            RunLevel                    RunLevel;          // 실행 레벨
            bool                        RunOnlyIfLoggedOn; // 로그온된 경우에만 실행
            bool                        Enabled;           // 작업 활성화 여부

            TaskSettings()
                : TaskPath( L"\\" )
                , Trigger( TriggerType::Logon )
                , RunLevel( RunLevel::Highest )
                , RunOnlyIfLoggedOn( false )
                , Enabled( true )
            {
            }
        };

        CTaskSchedulerHelper();
        ~CTaskSchedulerHelper();

        HRESULT                         Initialize();

        HRESULT                         RegisterTask( const TaskSettings& TaskOpt );
        HRESULT                         UnregisterTask( const std::wstring& TaskName, const std::wstring& TaskPath = L"\\" );
        bool                            TaskExists( const std::wstring& TaskName, const std::wstring& TaskPath = L"\\", HRESULT* Result = nullptr );
        HRESULT                         EnableTask( const std::wstring& TaskName, bool Enable, const std::wstring& TaskPath = L"\\" );
        HRESULT                         RunTask( const std::wstring& TaskName, const std::wstring& TaskPath = L"\\" );

    private:
        ITaskFolder*                    retrieveTaskFolder( const std::wstring& Path, HRESULT* Result = nullptr );
        HRESULT                         setTaskTrigger( ITriggerCollection* Collection, TriggerType Type );
        HRESULT                         setTaskAction( IActionCollection* Collection, const std::wstring& FileFullPath, const std::wstring& CMDLine, const std::wstring& WorkingDirectory );

        bool                            isInitialized_ = false;
        ITaskService*                   taskService_ = nullptr;
        ITaskFolder*                    rootFolder_ = nullptr;
        std::wstring                    lastError_;
    };
}
