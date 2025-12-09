#include "stdafx.h"
#include "CTaskSchedulerHelper.hpp"

#include <comutil.h>
#include <sddl.h>
#include <lmcons.h>
#include <sstream>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

namespace nsCmn
{
    namespace nsWin
    {
        CComInitializer::CComInitializer( DWORD CoInit )
        {
            Result_ = CoInitializeEx( nullptr, CoInit );
            initialized_ = SUCCEEDED( Result_ );
        }

        CComInitializer::~CComInitializer()
        {
            if( initialized_ == false )
                return;

            CoUninitialize();
        }

        HRESULT CComInitializer::IsInitialized() const
        {
            return Result_;
        }

        bool PrivilegeHelper::IsRunningAsAdmin()
        {
            BOOL                     IsAdmin     = FALSE;
            PSID                     AdminGroup  = nullptr;
            SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

            // Administrators 그룹 SID 생성
            if( AllocateAndInitializeSid(
                                         &NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &AdminGroup ) )
            {
                // 현재 토큰이 Administrators 그룹에 속하는지 확인
                if( !CheckTokenMembership( nullptr, AdminGroup, &IsAdmin ) )
                {
                    IsAdmin = FALSE;
                }
                FreeSid( AdminGroup );
            }

            return IsAdmin == TRUE;
        }

        bool PrivilegeHelper::RestartAsAdmin( const std::wstring& FileFullPath, const std::wstring& CMDLine )
        {
            // 이미 관리자 권한이면 재시작 불필요
            if( IsRunningAsAdmin() )
                return true;

            return RequestAdminPrivilege( FileFullPath, CMDLine );
        }

        bool PrivilegeHelper::RequestAdminPrivilege( const std::wstring& FileFullPath, const std::wstring& CMDLine )
        {
            bool IsSuccess = false;
            SHELLEXECUTEINFOW sei = { 0, };

            do
            {
                sei.cbSize            = sizeof( sei );
                sei.lpVerb            = L"runas";  // 관리자 권한으로 실행
                sei.lpFile            = FileFullPath.c_str();
                sei.lpParameters      = CMDLine.empty() ? nullptr : CMDLine.c_str();
                sei.hwnd              = nullptr;
                sei.nShow             = SW_NORMAL;
                sei.fMask             = SEE_MASK_NOCLOSEPROCESS;

                // 사용자가 UAC 를 취소하면 GetLastError() 의 값이 ERROR_CANCELLED 로 반환된다
                IsSuccess = ShellExecuteExW( &sei ) != FALSE;

                if( sei.hProcess )
                    CloseHandle( sei.hProcess );

            } while( false );

            return IsSuccess;
        }

        bool PrivilegeHelper::EnablePrivilege( const std::wstring& PrivilegeName )
        {
            LUID Luid = {};
            HANDLE Token = nullptr;
            bool IsSuccess = false;

            do
            {
                if( !OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token ) )
                    break;

                if( !LookupPrivilegeValueW( nullptr, PrivilegeName.c_str(), &Luid ) )
                    break;

                TOKEN_PRIVILEGES tp;
                tp.PrivilegeCount             = 1;
                tp.Privileges[ 0 ].Luid       = Luid;
                tp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

                const BOOL  Result = AdjustTokenPrivileges( Token, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), nullptr, nullptr );
                const DWORD Error  = GetLastError();

                IsSuccess = Result != FALSE && Error == ERROR_SUCCESS;

            } while( false );

            CloseHandle( Token );

            return IsSuccess;
        }

        std::wstring GetCurrentUserName()
        {
            WCHAR UserName[ UNLEN + 1 ];
            DWORD UserNameLen = UNLEN + 1;

            if( GetUserNameW( UserName, &UserNameLen ) == FALSE )
                return L"";

            return std::wstring( UserName );
        }
    }

    CTaskSchedulerHelper::CTaskSchedulerHelper()
    {
    }

    CTaskSchedulerHelper::~CTaskSchedulerHelper()
    {
        if( rootFolder_ )
        {
            rootFolder_->Release();
            rootFolder_ = nullptr;
        }

        if( taskService_ )
        {
            taskService_->Release();
            taskService_ = nullptr;
        }
    }

    HRESULT CTaskSchedulerHelper::Initialize()
    {
        HRESULT Result = S_FALSE;

        do
        {
            if( isInitialized_ == true )
            {
                Result = S_OK;
                break;
            }

            Result = CoCreateInstance( CLSID_TaskScheduler,
                                       nullptr,
                                       CLSCTX_INPROC_SERVER,
                                       IID_ITaskService,
                                       reinterpret_cast< void** >( &taskService_ ) );

            if( FAILED( Result ) )
                break;

            // 로컬 컴퓨터의 Task Scheduler에 연결
            Result = taskService_->Connect( _variant_t(), _variant_t(), _variant_t(), _variant_t() );
            if( FAILED( Result ) )
                break;

            // 루트 폴더 가져오기
            Result = taskService_->GetFolder( _bstr_t( L"\\" ), &rootFolder_ );
            if( FAILED( Result ) )
                break;

            isInitialized_ = true;

        } while( false );

        if( FAILED( Result ) )
        {
            if( taskService_ )
            {
                taskService_->Release();
                taskService_ = nullptr;
            }
        }

        return Result;
    }

    HRESULT CTaskSchedulerHelper::RegisterTask( const TaskSettings& TaskOpt )
    {
        HRESULT Result = S_FALSE;
        ITaskDefinition* TaskDefinition = nullptr;
        IRegistrationInfo* RegistrationInfo = nullptr;
        IPrincipal* PrincipalInfo = nullptr;
        ITaskSettings* TaskSettings = nullptr;
        ITriggerCollection* TaskTriggerCollection = nullptr;
        IActionCollection* TaskActionCollection = nullptr;
        ITaskFolder* TaskFolder = nullptr;
        IRegisteredTask* RegisteredTask = nullptr;

        do
        {
            if( isInitialized_ == false )
            {
                Result = RPC_E_FAULT;
                break;
            }

            Result = taskService_->NewTask( 0, &TaskDefinition );
            if( FAILED( Result ) )
                break;

            // 등록 정보 설정
            Result = TaskDefinition->get_RegistrationInfo( &RegistrationInfo );
            if( SUCCEEDED( Result ) )
            {
                if( !TaskOpt.Description.empty() )
                    RegistrationInfo->put_Description( _bstr_t( TaskOpt.Description.c_str() ) );
                RegistrationInfo->put_Author( _bstr_t( nsCmn::nsWin::GetCurrentUserName().c_str() ) );
                RegistrationInfo->Release();
            }

            // Principal 설정 (실행 권한 레벨)
            Result = TaskDefinition->get_Principal( &PrincipalInfo );
            if( FAILED( Result ) )
                break;

            // 실행 레벨 설정
            if( TaskOpt.RunLevel == RunLevel::LUA )
                (void)PrincipalInfo->put_RunLevel( TASK_RUNLEVEL_LUA );
            if( TaskOpt.RunLevel == RunLevel::Highest )
                (void)PrincipalInfo->put_RunLevel( TASK_RUNLEVEL_HIGHEST );

            // 로그온 타입 설정
            if( TaskOpt.RunOnlyIfLoggedOn )
                (void)PrincipalInfo->put_LogonType( TASK_LOGON_INTERACTIVE_TOKEN );
            else
                (void)PrincipalInfo->put_LogonType( TASK_LOGON_SERVICE_ACCOUNT );

            PrincipalInfo->Release();

            // 작업 설정
            Result = TaskDefinition->get_Settings( &TaskSettings );
            if( FAILED( Result ) )
                break;

            TaskSettings->put_Enabled( TaskOpt.Enabled ? VARIANT_TRUE : VARIANT_FALSE );
            TaskSettings->put_StartWhenAvailable( VARIANT_TRUE );
            TaskSettings->put_DisallowStartIfOnBatteries( VARIANT_FALSE );
            TaskSettings->put_StopIfGoingOnBatteries( VARIANT_FALSE );
            TaskSettings->put_AllowDemandStart( VARIANT_TRUE );
            TaskSettings->put_MultipleInstances( TASK_INSTANCES_IGNORE_NEW );
            TaskSettings->Release();

            // 트리거 추가
            Result = TaskDefinition->get_Triggers( &TaskTriggerCollection );
            if( FAILED( Result ) )
                break;

            Result = setTaskTrigger( TaskTriggerCollection, TaskOpt.Trigger );
            if( FAILED( Result ) )
                break;

            // 실행할 작업 추가
            Result = TaskDefinition->get_Actions( &TaskActionCollection );
            if( FAILED( Result ) )
                break;

            Result = setTaskAction( TaskActionCollection, TaskOpt.FileFullPath, TaskOpt.CMDLine, TaskOpt.WorkingDirectory );
            if( FAILED( Result ) )
                break;

            // 작업 등록
            TaskFolder = retrieveTaskFolder( TaskOpt.TaskPath, &Result );
            if( TaskFolder == nullptr )
                break;

            Result = TaskFolder->RegisterTaskDefinition( _bstr_t( TaskOpt.TaskName.c_str() ),
                                                         TaskDefinition,
                                                         TASK_CREATE_OR_UPDATE,
                                                         _variant_t(),
                                                         _variant_t(),
                                                         TaskOpt.RunOnlyIfLoggedOn
                                                             ? TASK_LOGON_INTERACTIVE_TOKEN
                                                             : TASK_LOGON_SERVICE_ACCOUNT,
                                                         _variant_t( L"" ),
                                                         &RegisteredTask );

        } while( false );

        if( TaskActionCollection != nullptr )
            TaskActionCollection->Release();

        if( TaskTriggerCollection != nullptr )
            TaskTriggerCollection->Release();

        if( RegisteredTask != nullptr )
            RegisteredTask->Release();

        if( TaskFolder != nullptr &&
            TaskFolder != rootFolder_ )
            TaskFolder->Release();

        if( TaskDefinition != nullptr )
            TaskDefinition->Release();

        return Result;
    }

    HRESULT CTaskSchedulerHelper::UnregisterTask( const std::wstring& TaskName, const std::wstring& TaskPath )
    {
        HRESULT Result = S_FALSE;
        ITaskFolder* Folder = retrieveTaskFolder( TaskPath, &Result );

        do
        {
            if( Folder == nullptr )
                break;

            Result = Folder->DeleteTask( _bstr_t( TaskName.c_str() ), 0 );

        } while( false );

        if( Folder != nullptr &&
            Folder != rootFolder_ )
            Folder->Release();

        return Result;
    }

    bool CTaskSchedulerHelper::TaskExists( const std::wstring& TaskName, const std::wstring& TaskPath, HRESULT* Result )
    {
        bool IsSuccess = false; HRESULT Hr = S_FALSE;
        ITaskFolder* Folder = retrieveTaskFolder( TaskPath, &Hr );
        IRegisteredTask* Task = nullptr;

        do
        {
            if( Folder == nullptr )
                break;

            Hr = Folder->GetTask( _bstr_t( TaskName.c_str() ), &Task );

            if( SUCCEEDED( Hr ) && Task )
            {
                Task->Release();
                IsSuccess = true;
            }

        } while( false );

        if( Folder != nullptr &&
            Folder != rootFolder_ )
            Folder->Release();

        if( Result != nullptr )
            *Result = Hr;

        return IsSuccess;
    }

    HRESULT CTaskSchedulerHelper::EnableTask( const std::wstring& TaskName, bool Enable, const std::wstring& TaskPath )
    {
        HRESULT Result = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        ITaskFolder* Folder = retrieveTaskFolder( TaskPath, &Result );
        IRegisteredTask* Task = nullptr;

        do
        {
            if( Folder == nullptr )
                break;

            Result = Folder->GetTask( _bstr_t( TaskName.c_str() ), &Task );

            if( FAILED( Result ) || Task == nullptr )
                break;

            Result = Task->put_Enabled( Enable ? VARIANT_TRUE : VARIANT_FALSE );

        } while( false );

        if( Task != nullptr )
            Task->Release();

        if( Folder != nullptr &&
            Folder != rootFolder_ )
            Folder->Release();

        return Result;
    }

    HRESULT CTaskSchedulerHelper::RunTask( const std::wstring& TaskName, const std::wstring& TaskPath )
    {
        HRESULT Result = S_FALSE;

        ITaskFolder* Folder = retrieveTaskFolder( TaskPath, &Result );
        IRegisteredTask* Task = nullptr;
        IRunningTask* RunningTask = nullptr;

        do
        {
            if( Folder == nullptr )
                break;

            Result = Folder->GetTask( _bstr_t( TaskName.c_str() ), &Task );

            if( FAILED( Result ) || !Task )
                break;

            Result = Task->Run( _variant_t(), &RunningTask );

        } while( false );

        if( RunningTask != nullptr )
            RunningTask->Release();

        if( Task != nullptr )
            Task->Release();

        if( Folder != nullptr &&
            Folder != rootFolder_ )
            Folder->Release();

        return Result;
    }

    ITaskFolder* CTaskSchedulerHelper::retrieveTaskFolder( const std::wstring& Path, HRESULT* Result )
    {
        if( Path.empty() == true || Path == L"\\" )
            return rootFolder_;

        ITaskFolder* Folder = nullptr;

        do
        {
            const auto Hr = taskService_->GetFolder( _bstr_t( Path.c_str() ), &Folder );
            if( Result != nullptr )
                *Result = Hr;

        } while( false );

        return Folder;
    }

    HRESULT CTaskSchedulerHelper::setTaskTrigger( ITriggerCollection* Collection, TriggerType Type )
    {
        HRESULT   Result  = S_FALSE;
        ITrigger* Trigger = nullptr;

        if( Collection == nullptr )
            return RPC_E_FAULT;

        switch( Type )
        {
            case TriggerType::Logon: {
                Result = Collection->Create( TASK_TRIGGER_LOGON, &Trigger );
                if( FAILED( Result ) )
                    break;

                ILogonTrigger* logonTrigger = nullptr;
                Result = Trigger->QueryInterface( IID_ILogonTrigger, reinterpret_cast< void** >( &logonTrigger ) );
                if( FAILED( Result ) )
                    break;

                logonTrigger->put_Id( _bstr_t( L"LogonTriggerId" ) );
                logonTrigger->put_Enabled( VARIANT_TRUE );
                // 현재 사용자에 대해서만 트리거
                const std::wstring userId = nsWin::GetCurrentUserName();
                if( userId.empty() == false )
                    logonTrigger->put_UserId( _bstr_t( userId.c_str() ) );
                logonTrigger->Release();
            } break;
            case TriggerType::Boot: {
                Result = Collection->Create( TASK_TRIGGER_BOOT, &Trigger );
                if( FAILED( Result ) )
                    break;

                IBootTrigger* bootTrigger = nullptr;
                Result = Trigger->QueryInterface( IID_IBootTrigger, reinterpret_cast< void** >( &bootTrigger ) );
                if( FAILED( Result ) )
                    break;

                bootTrigger->put_Id( _bstr_t( L"BootTriggerId" ) );
                bootTrigger->put_Enabled( VARIANT_TRUE );
                // 시스템 시작 후 30초 지연
                bootTrigger->put_Delay( _bstr_t( L"PT30S" ) );
                bootTrigger->Release();
            } break;
            default: {
                Result = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            } break;
        }

        if( Trigger )
            Trigger->Release();

        return Result;
    }

    HRESULT CTaskSchedulerHelper::setTaskAction( IActionCollection* Collection, const std::wstring& FileFullPath,
            const std::wstring& CMDLine, const std::wstring& WorkingDirectory )
    {
        HRESULT Result = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        IAction* Action = nullptr;
        IExecAction* ExecAction = nullptr;

        do
        {
            if( Collection == nullptr )
                break;

            Result = Collection->Create( TASK_ACTION_EXEC, &Action );
            if( FAILED( Result ) )
                break;

            Result = Action->QueryInterface( IID_IExecAction, reinterpret_cast< void** >( &ExecAction ) );
            if( FAILED( Result ) )
                break;

            if( FileFullPath.empty() == true )
            {
                Result = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                break;
            }

            Result = ExecAction->put_Path( _bstr_t( FileFullPath.c_str() ) );
            if( FAILED( Result ) )
                break;

            if( CMDLine.empty() == false )
            {
                Result = ExecAction->put_Arguments( _bstr_t( CMDLine.c_str() ) );
                if( FAILED( Result ) )
                    break;
            }

            if( WorkingDirectory.empty() == false )
            {
                Result = ExecAction->put_WorkingDirectory( _bstr_t( WorkingDirectory.c_str() ) );
                if( FAILED( Result ) )
                    break;
            }

        } while( false );

        if( ExecAction != nullptr )
            ExecAction->Release();

        if( Action != nullptr )
            Action->Release();

        return Result;

    }
}
