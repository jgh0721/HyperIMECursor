#include "stdafx.h"
#include "CProcessMonitorETW.hpp"

#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "tdh.lib")

///////////////////////////////////////////////////////////////////////////////
///

// Microsoft-Windows-Kernel-Process Provider GUID
// {22FB2CD6-0E7B-422B-A0C7-2FAD1FD0E716}
const GUID ETW_KERNEL_PROCESS_GUID = { 0x22FB2CD6, 0x0E7B, 0x422B, { 0xA0, 0xC7, 0x2F, 0xAD, 0x1F, 0xD0, 0xE7, 0x16 } };
constexpr USHORT EVENT_ID_PROCESS_START = 1;
constexpr USHORT EVENT_ID_PROCESS_STOP = 2;

namespace nsDetail
{
    class CProcessMonitorWorker : public QObject
    {
        Q_OBJECT
    public:
        explicit CProcessMonitorWorker( QObject* parent = nullptr ) : QObject( parent ) {}
        ~CProcessMonitorWorker() {}

    public Q_SLOTS:
        void Start()
        {
            if( m_running )
                return;

            if( !setupTraceSession() )
                return;

            m_running = true;
            emit sigSessionStarted();

            // ProcessTrace는 블로킹 호출 - 이벤트 처리 루프
            ULONG status = ProcessTrace( &m_traceHandle, 1, nullptr, nullptr );

            m_running = false;

            if( status != ERROR_SUCCESS && status != ERROR_CANCELLED )
            {
                emit sigErrorOccurred( QString( "ProcessTrace ended with error: %1" ).arg( status ) );
            }

            cleanupTraceSession();
            emit sigSessionEnded();
        }

        void Stop()
        {
            m_running = false;

            // ProcessTrace 루프 종료를 위해 트레이스 핸들 닫기
            if( m_traceHandle != INVALID_PROCESSTRACE_HANDLE )
            {
                CloseTrace( m_traceHandle );
                m_traceHandle = INVALID_PROCESSTRACE_HANDLE;
            }
        }

    Q_SIGNALS:
        void sigProcessEvent( const ProcessEventInfo& Info );
        void sigErrorOccurred( const QString& ErrorString );
        void sigSessionStarted();
        void sigSessionEnded();

    private:
        bool setupTraceSession()
        {
            // 기존 세션이 있으면 정리
            cleanupTraceSession();

            // 세션 속성 구조체 할당
            size_t sessionNameLen = (wcslen( SESSION_NAME ) + 1) * sizeof( wchar_t );
            size_t bufferSize     = sizeof( EVENT_TRACE_PROPERTIES ) + sessionNameLen;

            m_properties = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize ));

            if( !m_properties )
            {
                emit sigErrorOccurred( "Failed to allocate memory for session properties" );
                return false;
            }

            // 세션 속성 설정
            m_properties->Wnode.BufferSize    = static_cast<ULONG>(bufferSize);
            m_properties->Wnode.Flags         = WNODE_FLAG_TRACED_GUID;
            m_properties->Wnode.ClientContext = 1; // QPC clock resolution
            m_properties->LogFileMode         = EVENT_TRACE_REAL_TIME_MODE;
            m_properties->LoggerNameOffset    = sizeof( EVENT_TRACE_PROPERTIES );

            // 기존 세션 중지 시도 (있을 경우)
            ULONG status = ControlTraceW( 0, SESSION_NAME, m_properties, EVENT_TRACE_CONTROL_STOP );

            // 세션 속성 재설정 (ControlTrace가 구조체를 수정할 수 있음)
            ZeroMemory( m_properties, bufferSize );
            m_properties->Wnode.BufferSize    = static_cast<ULONG>(bufferSize);
            m_properties->Wnode.Flags         = WNODE_FLAG_TRACED_GUID;
            m_properties->Wnode.ClientContext = 1;
            m_properties->LogFileMode         = EVENT_TRACE_REAL_TIME_MODE;
            m_properties->LoggerNameOffset    = sizeof( EVENT_TRACE_PROPERTIES );

            // 새 세션 시작
            status = StartTraceW( &m_sessionHandle, SESSION_NAME, m_properties );

            if( status != ERROR_SUCCESS )
            {
                emit sigErrorOccurred( QString( "StartTrace failed with error: %1" ).arg( status ) );
                cleanupTraceSession();
                return false;
            }

            // Provider 활성화 (Microsoft-Windows-Kernel-Process)
            status = EnableTraceEx2(
                                    m_sessionHandle,
                                    &ETW_KERNEL_PROCESS_GUID,
                                    EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                                    TRACE_LEVEL_INFORMATION,
                                    0x10, // WINEVENT_KEYWORD_PROCESS
                                    0,
                                    0,
                                    nullptr
                                   );

            if( status != ERROR_SUCCESS )
            {
                emit sigErrorOccurred( QString( "EnableTraceEx2 failed with error: %1" ).arg( status ) );
                cleanupTraceSession();
                return false;
            }

            // Consumer 설정
            EVENT_TRACE_LOGFILE traceLogfile = {};
            traceLogfile.LoggerName          = const_cast<LPWSTR>(SESSION_NAME);
            traceLogfile.ProcessTraceMode    = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME;
            traceLogfile.EventRecordCallback = EventRecordCallback;
            traceLogfile.Context             = this;

            m_traceHandle = OpenTraceW( &traceLogfile );

            if( m_traceHandle == INVALID_PROCESSTRACE_HANDLE )
            {
                DWORD error = GetLastError();
                emit sigErrorOccurred( QString( "OpenTrace failed with error: %1" ).arg( error ) );
                cleanupTraceSession();
                return false;
            }

            return true;
        }

        void cleanupTraceSession()
        {
            if( m_traceHandle != INVALID_PROCESSTRACE_HANDLE )
            {
                CloseTrace( m_traceHandle );
                m_traceHandle = INVALID_PROCESSTRACE_HANDLE;
            }

            if( m_sessionHandle != INVALID_PROCESSTRACE_HANDLE && m_properties )
            {
                ControlTraceW( m_sessionHandle, SESSION_NAME, m_properties, EVENT_TRACE_CONTROL_STOP );
                m_sessionHandle = INVALID_PROCESSTRACE_HANDLE;
            }

            if( m_properties )
            {
                HeapFree( GetProcessHeap(), 0, m_properties );
                m_properties = nullptr;
            }
        }

        static void WINAPI EventRecordCallback( PEVENT_RECORD EventRecord )
        {
            if( EventRecord == nullptr )
                return;

            if( EventRecord->UserData == nullptr )
                return;

            const auto Self = reinterpret_cast< CProcessMonitorWorker* >( EventRecord->UserContext );
            if( Self == nullptr )
                return;

            if( !IsEqualGUID( EventRecord->EventHeader.ProviderId, ETW_KERNEL_PROCESS_GUID ) )
                return;

            const USHORT EventId = EventRecord->EventHeader.EventDescriptor.Id;

            if( EventId != EVENT_ID_PROCESS_START && EventId != EVENT_ID_PROCESS_STOP )
                return;

            // helper lambdas
            auto trimTrailingNullsW = [](const wchar_t* ptr, size_t wcharCount) -> size_t {
                if (wcharCount == 0) return 0;
                // trim trailing L'\0'
                while (wcharCount > 0 && ptr[wcharCount - 1] == L'\0') --wcharCount;
                return wcharCount;
            };
            auto trimTrailingNullsA = [](const char* ptr, size_t byteCount) -> size_t {
                if (byteCount == 0) return 0;
                while (byteCount > 0 && ptr[byteCount - 1] == '\0') --byteCount;
                return byteCount;
            };

            ProcessEventInfo Info;
            Info.Type = (EventId == EVENT_ID_PROCESS_START)
                            ? ProcessEventInfo::EventType::ProcessStart
                            : ProcessEventInfo::EventType::ProcessStop;
            Info.ProcessId       = 0;
            Info.ParentProcessId = 0;
            Info.ExitCode        = 0;
            Info.CreationTime    = {};

            // 타임스탬프 변환 (FILETIME -> QDateTime)
            FILETIME ft;
            ft.dwLowDateTime  = EventRecord->EventHeader.TimeStamp.LowPart;
            ft.dwHighDateTime = EventRecord->EventHeader.TimeStamp.HighPart;
            SYSTEMTIME st;
            FileTimeToSystemTime( &ft, &st );
            Info.EventTime = QDateTime( QDate( st.wYear, st.wMonth, st.wDay ),
                                        QTime( st.wHour, st.wMinute, st.wSecond, st.wMilliseconds ) );

            // TDH를 사용하여 이벤트 데이터 파싱
            DWORD     bufferSize = 0;
            TDHSTATUS Status     = TdhGetEventInformation( EventRecord, 0, nullptr, nullptr, &bufferSize );
            if( Status != ERROR_INSUFFICIENT_BUFFER || bufferSize == 0 )
                return;

            QByteArray        buffer( bufferSize, 0 );
            PTRACE_EVENT_INFO EventInfo = reinterpret_cast< PTRACE_EVENT_INFO >( buffer.data() );

            Status = TdhGetEventInformation( EventRecord, 0, nullptr, EventInfo, &bufferSize );
            if( Status != ERROR_SUCCESS )
                return;

            // 각 속성 파싱
            for( quint32 Idx = 0; Idx < EventInfo->TopLevelPropertyCount; Idx++ )
            {
                EVENT_PROPERTY_INFO& propInfo = EventInfo->EventPropertyInfoArray[ Idx ];

                // 속성 이름 가져오기
                LPCWSTR propName = reinterpret_cast< LPCWSTR >( reinterpret_cast< PBYTE >( EventInfo ) + propInfo.NameOffset );

                // 속성 데이터 가져오기
                PROPERTY_DATA_DESCRIPTOR dataDesc;
                dataDesc.PropertyName = reinterpret_cast< ULONGLONG >( propName );
                dataDesc.ArrayIndex   = ULONG_MAX;

                DWORD dataSize = 0;
                Status         = TdhGetPropertySize( EventRecord, 0, nullptr, 1, &dataDesc, &dataSize );

                if( Status == ERROR_SUCCESS && dataSize > 0 )
                {
                    QByteArray propData( dataSize, 0 );
                    Status = TdhGetProperty( EventRecord, 0, nullptr, 1, &dataDesc, dataSize,
                                             reinterpret_cast< PBYTE >( propData.data() ) );

                    if( Status == ERROR_SUCCESS )
                    {
                        QString propNameStr = QString::fromWCharArray( propName );

                        if( propNameStr == "ProcessID" || propNameStr == "ProcessId" )
                        {
                            Info.ProcessId = *reinterpret_cast< DWORD* >( propData.data() );
                        }
                        else if( propNameStr == "ParentProcessID" || propNameStr == "ParentProcessId" )
                        {
                            Info.ParentProcessId = *reinterpret_cast< DWORD* >( propData.data() );
                        }
                        else if( propNameStr == "ImageName" )
                        {
                            switch( propInfo.nonStructType.InType )
                            {
                                case TDH_INTYPE_UNICODESTRING: {
                                    const wchar_t* wptr = reinterpret_cast<const wchar_t*>( propData.constData() );
                                    size_t wcharCount = dataSize / sizeof(wchar_t);
                                    wcharCount = trimTrailingNullsW(wptr, wcharCount);
                                    if (wcharCount > 0)
                                        Info.ExecutableFileName = QString::fromWCharArray( wptr, static_cast<int>(wcharCount) );
                                    else
                                        Info.ExecutableFileName.clear();
                                } break;
                                case TDH_INTYPE_ANSISTRING: {
                                    const char* aptr = reinterpret_cast<const char*>( propData.constData() );
                                    size_t byteCount = dataSize;
                                    byteCount = trimTrailingNullsA(aptr, byteCount);
                                    if (byteCount > 0)
                                        Info.ExecutableFileName = QString::fromLocal8Bit( aptr, static_cast<int>(byteCount) );
                                    else
                                        Info.ExecutableFileName.clear();
                                } break;
                                default: {
                                    Info.ExecutableFileName = QString::fromLocal8Bit( reinterpret_cast< LPCSTR >( propData.data() ) );
                                } break;
                            }

                            if( propInfo.nonStructType.InType == TDH_INTYPE_UNICODESTRING )
                                Info.ExecutableFileName = QString::fromWCharArray( reinterpret_cast< LPCWSTR >( propData.data() ) );

                            if( propInfo.nonStructType.InType == TDH_INTYPE_ANSISTRING )
                                Info.ExecutableFileName = QString::fromLocal8Bit( reinterpret_cast< LPCSTR >( propData.data() ) );
                        }
                        else if( propNameStr == "CommandLine" )
                        {
                            switch( propInfo.nonStructType.InType )
                            {
                                case TDH_INTYPE_UNICODESTRING: {
                                    const wchar_t* wptr = reinterpret_cast<const wchar_t*>( propData.constData() );
                                    size_t wcharCount = dataSize / sizeof(wchar_t);
                                    wcharCount = trimTrailingNullsW(wptr, wcharCount);
                                    if (wcharCount > 0)
                                        Info.CommandLine = QString::fromWCharArray( wptr, static_cast<int>(wcharCount) );
                                    else
                                        Info.CommandLine.clear();
                                } break;
                                case TDH_INTYPE_ANSISTRING: {
                                    const char* aptr = reinterpret_cast<const char*>( propData.constData() );
                                    size_t byteCount = dataSize;
                                    byteCount = trimTrailingNullsA(aptr, byteCount);
                                    if (byteCount > 0)
                                        Info.CommandLine = QString::fromLocal8Bit( aptr, static_cast<int>(byteCount) );
                                    else
                                        Info.CommandLine.clear();
                                } break;
                                default: {
                                    Info.CommandLine = QString::fromLocal8Bit( reinterpret_cast< LPCSTR >( propData.data() ) );
                                } break;
                            }

                            if( propInfo.nonStructType.InType == TDH_INTYPE_UNICODESTRING )
                                Info.CommandLine = QString::fromWCharArray( reinterpret_cast< LPCWSTR >( propData.data() ) );

                            if( propInfo.nonStructType.InType == TDH_INTYPE_ANSISTRING )
                                Info.CommandLine = QString::fromLocal8Bit( reinterpret_cast< LPCSTR >( propData.data() ) );
                        }
                        else if( propNameStr == "ExitCode" )
                        {
                            Info.ExitCode = *reinterpret_cast< DWORD* >( propData.data() );
                        }
                        else if( propNameStr == "CreateTime" )
                        {
                            Info.CreateTime = *reinterpret_cast< LONGLONG* >( propData.data() );
                        }
                    }
                }
            } // for

            // ProcessID가 EventHeader에서도 얻을 수 있음 (fallback)
            if (Info.ProcessId == 0)
                Info.ProcessId = EventRecord->EventHeader.ProcessId;

            Q_EMIT Self->sigProcessEvent( Info );
        }

        volatile bool m_running = false;
        static constexpr wchar_t SESSION_NAME[] = L"QtProcessMonitorSession";
        EVENT_TRACE_PROPERTIES* m_properties    = nullptr;
        TRACEHANDLE             m_sessionHandle = INVALID_PROCESSTRACE_HANDLE;
        TRACEHANDLE             m_traceHandle   = INVALID_PROCESSTRACE_HANDLE;
    };
}

///////////////////////////////////////////////////////////////////////////////
///
///

CProcessMonitor::CProcessMonitor( QObject* Parent )
    : QObject( Parent )
{
    qRegisterMetaType<ProcessEventInfo>("ProcessEventInfo");
}

CProcessMonitor::~CProcessMonitor()
{
    Stop();
}

bool CProcessMonitor::Start()
{
    do
    {
        if( m_running )
            return true;

        m_workerThread = new QThread(this);
        m_worker = new nsDetail::CProcessMonitorWorker();
        m_worker->moveToThread(m_workerThread);

        // 시그널-슬롯 연결
        connect( m_workerThread, &QThread::started, m_worker, &nsDetail::CProcessMonitorWorker::Start );
        connect( m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater );

        connect(m_worker, &nsDetail::CProcessMonitorWorker::sigProcessEvent,
                this, &CProcessMonitor::sltProcessEvent, Qt::QueuedConnection);
        //     connect(m_worker, &EtwSessionWorker::sessionStarted,
        //             this, &EtwProcessMonitor::onSessionStarted, Qt::QueuedConnection);
        //     connect(m_worker, &EtwSessionWorker::sessionStopped,
        //             this, &EtwProcessMonitor::onSessionStopped, Qt::QueuedConnection);
        //     connect(m_worker, &EtwSessionWorker::errorOccurred,
        //             this, &EtwProcessMonitor::onError, Qt::QueuedConnection);

        m_workerThread->start();

        m_running = true;

    } while( false );

    return m_running;
}

void CProcessMonitor::Stop()
{
    if( m_running == false || m_worker == nullptr )
        return;

    m_worker->Stop();

    if( m_workerThread )
    {
        m_workerThread->quit();
        m_workerThread->wait( 5000 );
        m_workerThread = nullptr;
    }

    m_worker = nullptr;
    m_running = false;
}

bool CProcessMonitor::IsMonitoring() const
{
    return false;
}

void CProcessMonitor::sltProcessEvent( const ProcessEventInfo& Info )
{
    if( Info.Type == ProcessEventInfo::EventType::ProcessStart )
        emit sigProcessStarted( Info );
    else
        emit sigProcessTerminated( Info );
}

void CProcessMonitor::sltSessionStarted()
{
    m_running = true;
    emit sigMonitoringStarted();
}

void CProcessMonitor::sltSessionStopped()
{
    m_running = false;
    emit sigMonitoringStopped();
}

void CProcessMonitor::sltError(const QString& error)
{
    emit sigErrorOccurred(error);
}

///////////////////////////////////////////////////////////////////////////////

#include "CProcessMonitorETW.moc"
