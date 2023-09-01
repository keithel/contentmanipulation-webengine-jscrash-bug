#ifndef RENDERERKILLERTIMER_H
#define RENDERERKILLERTIMER_H

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

class RendererKillerTimer : public QTimer
{
    Q_OBJECT
public:
    explicit RendererKillerTimer(QObject *parent = nullptr);

protected:
    void getDescendantProcInfo(qint64 pid, QSet<ProcessInfo> &childProcInfos, const QString& commandLineContains);
    QSet<ProcessInfo> getDescendantProcInfo(const QString &commandLineContains);
    bool killPid(const qint64 pid);
};

#endif // RENDERERKILLERTIMER_H
