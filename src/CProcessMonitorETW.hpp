#pragma once

#include <QtCore>

namespace nsDetail
{
    class CProcessMonitorWorker;
}

// 프로세스 이벤트 정보
struct ProcessEventInfo
{
    enum class EventType { ProcessStart, ProcessStop };

    EventType                           Type;
    QDateTime                           EventTime;

    DWORD                               ProcessId = 0;
    DWORD                               ParentProcessId = 0;
    QString                             ExecutableFileName;
    // QString exePath;
    // DWORD threadCount = 0;
    // LONG priority = 0;
    // QDateTime firstSeen;
    //
    // // 확장 정보 (OpenProcess 성공 시)
    QString                             CommandLine;
    // QString userName;
    FILETIME                            CreationTime = {};
    LONGLONG                            CreateTime = {};
    // SIZE_T workingSetSize = 0;

    DWORD                               ExitCode = 0;  // 종료 시에만 유효
};

Q_DECLARE_METATYPE(ProcessEventInfo)

class CProcessMonitor : public QObject
{
    Q_OBJECT
public:
    explicit CProcessMonitor( QObject* Parent = nullptr );
    ~CProcessMonitor();

public Q_SLOTS:
    bool                                Start();
    void                                Stop();
    bool                                IsMonitoring() const;

Q_SIGNALS:
    void                                sigProcessStarted( const ProcessEventInfo& info );
    void                                sigProcessTerminated( const ProcessEventInfo& info );
    void                                sigMonitoringStarted();
    void                                sigMonitoringStopped();
    void                                sigErrorOccurred( const QString& error );

private slots:
    void                                sltProcessEvent( const ProcessEventInfo& info );
    void                                sltSessionStarted();
    void                                sltSessionStopped();
    void                                sltError( const QString& error );

private:
    bool        m_running = false;
    QThread*    m_workerThread = nullptr;
    QPointer< nsDetail::CProcessMonitorWorker > m_worker;
};

///////////////////////////////////////////////////////////////////////////////
