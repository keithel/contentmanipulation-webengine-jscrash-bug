/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include "mainwindow.h"
#include <qtwebenginewidgetsglobal.h>

struct ProcessInfo {
    qint64 pid;
    qint64 ppid;
    QString commandLine;

    ProcessInfo(qint64 pid_, qint64 ppid_, QString commandLine_)
        : pid(pid_), ppid(ppid_), commandLine(commandLine_)
    {}

};

inline bool operator==(const ProcessInfo &a, const ProcessInfo &b)
{
    return (a.pid == b.pid) && (a.ppid == b.ppid) && (a.commandLine == b.commandLine);
}

inline uint qHash(const ProcessInfo &key, uint seed)
{
    return qHash(key.pid, seed) ^ key.ppid;
}


void _getDescendantProcInfo(qint64 pid, QSet<ProcessInfo> &childProcInfos, const QString& commandLineContains)
{
    QProcess ps;
    ps.start("ps", QStringList() << "-e" << "-o" << "pid,ppid,args");
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
                            _getDescendantProcInfo(cpid, childProcInfos, commandLineContains);
                    }
                }
            }
        }
    }
}

QSet<ProcessInfo> getDescendantProcInfo(const QString &commandLineContains)
{
    QSet<ProcessInfo> childProcInfos;
    _getDescendantProcInfo(QCoreApplication::applicationPid(), childProcInfos, commandLineContains);
    return childProcInfos;
}

bool killPid(const qint64 pid)
{
    QProcess killProcess;
    killProcess.start("kill", QStringList() << "-9" << QString::number(pid));
    killProcess.waitForFinished();

    if (killProcess.exitStatus() != QProcess::NormalExit || killProcess.exitCode() != 0)
    {
        qDebug() << "Failed to kill process with PID:" << pid;
        return false;
    }

    return true; // Return true if the process was successfully killed.
}

void setupRendererKiller(QObject *parent)
{
    QTimer* rendererKiller = new QTimer(parent);
    rendererKiller->setSingleShot(false);
    QObject::connect(rendererKiller, &QTimer::timeout, parent, [](){
        QSet<ProcessInfo> descendantProcInfos = getDescendantProcInfo("QtWebEngineProc");
        for(auto& procInfo : qAsConst(descendantProcInfos)) {
            if (procInfo.commandLine.contains("renderer"))
                killPid(procInfo.pid);
        }
    });
    rendererKiller->start(5000);
}

int main(int argc, char * argv[])
{
    QCoreApplication::setOrganizationName("QtExamples");
    QApplication app(argc, argv);

    QUrl url;
    if (argc > 1)
        url = QUrl::fromUserInput(argv[1]);
    else
        url = QUrl("http://www.google.com/ncr");
    MainWindow *browser = new MainWindow(url);
    browser->resize(1024, 768);
    browser->show();

    setupRendererKiller(browser);
    return app.exec();
}
