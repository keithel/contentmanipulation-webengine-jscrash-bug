#include "rendererkiller.h"
#include <QProcess>
#include <QCoreApplication>

ProcessInfo::ProcessInfo(qint64 pid_, qint64 ppid_, QString commandLine_)
    : pid(pid_), ppid(ppid_), commandLine(commandLine_)
{}

bool operator==(const ProcessInfo &a, const ProcessInfo &b)
{
    return (a.pid == b.pid) && (a.ppid == b.ppid) && (a.commandLine == b.commandLine);
}

uint qHash(const ProcessInfo &key, uint seed)
{
    return qHash(key.pid, seed) ^ key.ppid;
}

RendererKiller::RendererKiller(QObject *parent)
    : QTimer{parent}
{
    setSingleShot(false);
    connect(this, &QTimer::timeout, this, &RendererKiller::killRenderer);
}

void RendererKiller::getDescendantProcInfo(qint64 pid, QSet<ProcessInfo> &childProcInfos, const QString& commandLineContains)
{
    QProcess ps;
    ps.start("ps", QStringList() << "-e" << "--cols" << "300" << "-o" << "pid,ppid,args");
    ps.waitForFinished();

    if (ps.exitStatus() == QProcess::NormalExit)
    {
        QString psOutput = ps.readAllStandardOutput();
        QStringList lines = psOutput.split('\n', Qt::SkipEmptyParts);

        for (const QString& line : lines)
        {
            QStringList fields = line.trimmed().split(' ', Qt::SkipEmptyParts);

            if (fields.size() >= 3)
            {
                bool conversionOK;
                qint64 cpid = fields.at(0).toLongLong(&conversionOK);
                QString commandLine = fields.mid(2).join(" ");

                if (conversionOK)
                {
                    qint64 ppid = fields.at(1).toLongLong(&conversionOK);

                    if (conversionOK && ppid == pid && cpid != pid &&
                        commandLine.contains(commandLineContains, Qt::CaseInsensitive))
                    {
                        ProcessInfo cpinfo(cpid, ppid, commandLine);
                        bool recurse = ! childProcInfos.contains(cpinfo);
                        childProcInfos.insert(ProcessInfo(cpid, ppid, commandLine));
                        if (recurse)
                            getDescendantProcInfo(cpid, childProcInfos, commandLineContains);
                    }
                }
            }
        }
    }
}

QSet<ProcessInfo> RendererKiller::getDescendantProcInfo(const QString &commandLineContains)
{
    QSet<ProcessInfo> childProcInfos;
    getDescendantProcInfo(QCoreApplication::applicationPid(), childProcInfos, commandLineContains);
    return childProcInfos;
}

void RendererKiller::killPid(const qint64 pid)
{
    QProcess killProcess;
    killProcess.start("kill", QStringList() << "-9" << QString::number(pid));
    killProcess.waitForFinished();

    if (killProcess.exitStatus() != QProcess::NormalExit || killProcess.exitCode() != 0)
    {
        qDebug() << "Failed to kill process with PID:" << pid;
    }
}

void RendererKiller::killRenderer()
{
    size_t numRenderersKilled = 0;
    QSet<ProcessInfo> descendantProcInfos = getDescendantProcInfo("QtWebEngineProc");
    for(auto& procInfo : qAsConst(descendantProcInfos)) {
        if (procInfo.commandLine.contains("renderer")) {
            killPid(procInfo.pid);
            numRenderersKilled++;
        }
    }

    qDebug().noquote() << "Found and killed" << numRenderersKilled << "render processes";
}
