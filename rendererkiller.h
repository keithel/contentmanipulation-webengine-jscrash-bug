#ifndef RENDERERKILLER_H
#define RENDERERKILLER_H

#include <QTimer>
#include <QString>
#include <QSet>

struct ProcessInfo {
    qint64 pid;
    qint64 ppid;
    QString commandLine;

    ProcessInfo(qint64 pid_, qint64 ppid_, QString commandLine_);
};

bool operator==(const ProcessInfo &a, const ProcessInfo &b);
uint qHash(const ProcessInfo &key, uint seed);

class RendererKiller : public QTimer
{
    Q_OBJECT
public:
    explicit RendererKiller(QObject *parent = nullptr);

public slots:
    void killRenderer();

protected:
    void getDescendantProcInfo(qint64 pid, QSet<ProcessInfo> &childProcInfos, const QString& commandLineContains);
    QSet<ProcessInfo> getDescendantProcInfo(const QString &commandLineContains);
    void killPid(const qint64 pid);
};

#endif // RENDERERKILLER_H
