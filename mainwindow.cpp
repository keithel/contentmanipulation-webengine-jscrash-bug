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
#include <QtWebEngineWidgets>
#include <QTimer>
#include "mainwindow.h"
#include "rendererkiller.h"

MainWindow::MainWindow(const QUrl& url)
    : rendererKiller(new RendererKiller(this))
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    progress = 0;

    QFile file;
    file.setFileName(":/jquery.min.js");
    file.open(QIODevice::ReadOnly);
    jQuery = file.readAll();
    jQuery.append("\nvar qt = { 'jQuery': jQuery.noConflict(true) };");
    file.close();

    view = new QWebEngineView(this);
    view->load(url);
    connect(view, &QWebEngineView::loadFinished, this, &MainWindow::adjustLocation);
    connect(view, &QWebEngineView::titleChanged, this, &MainWindow::adjustTitle);
    connect(view, &QWebEngineView::loadProgress, this, &MainWindow::setProgress);
    connect(view, &QWebEngineView::loadFinished, this, &MainWindow::finishLoading);
    connect(view, &QWebEngineView::renderProcessTerminated, this, &MainWindow::handleRenderProcessTerminated);

    locationEdit = new QLineEdit(this);
    locationEdit->setSizePolicy(QSizePolicy::Expanding, locationEdit->sizePolicy().verticalPolicy());
    connect(locationEdit, &QLineEdit::returnPressed, this, &MainWindow::changeLocation);

    QToolBar *toolBar = addToolBar(tr("Navigation"));
    toolBar->addAction(view->pageAction(QWebEnginePage::Back));
    toolBar->addAction(view->pageAction(QWebEnginePage::Forward));
    toolBar->addAction(view->pageAction(QWebEnginePage::Reload));
    toolBar->addAction(view->pageAction(QWebEnginePage::Stop));
    toolBar->addAction("😵", rendererKiller, &RendererKiller::killRenderer);
    toolBar->addWidget(locationEdit);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QAction *viewSourceAction = new QAction(tr("Page Source"), this);
    connect(viewSourceAction, &QAction::triggered, this, &MainWindow::viewSource);
    viewMenu->addAction(viewSourceAction);

    QMenu *effectMenu = menuBar()->addMenu(tr("&Effect"));
    effectMenu->addAction(tr("Highlight all links"), this, &MainWindow::highlightAllLinks);

    toggleHighlightAction = effectMenu->addAction(
        tr("Toggle highlight all links"), this, &MainWindow::toggleHighlightAllLinks);
    toggleHighlightAction->setCheckable(true);

    rotateAction = new QAction(this);
    rotateAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    rotateAction->setCheckable(true);
    rotateAction->setText(tr("Turn images upside down"));
    connect(rotateAction, &QAction::toggled, this, &MainWindow::rotateImages);
    effectMenu->addAction(rotateAction);

    reloadIfRendererTerminatedAction = new QAction("Reload if renderer terminated", this);
    reloadIfRendererTerminatedAction->setCheckable(true);
    effectMenu->addAction(reloadIfRendererTerminatedAction);

    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Remove GIF images"), this, &MainWindow::removeGifImages);
    toolsMenu->addAction(tr("Remove all inline frames"), this, &MainWindow::removeInlineFrames);
    toolsMenu->addAction(tr("Remove all object elements"), this, &MainWindow::removeObjectElements);
    toolsMenu->addAction(tr("Remove all embedded elements"), this, &MainWindow::removeEmbeddedElements);

    highlightTimer = new QTimer(this);
    connect(highlightTimer, &QTimer::timeout, toggleHighlightAction, [this](){
        toggleHighlightAction->trigger();
    });
    highlightTimer->start(1000);

    setCentralWidget(view);
}

void MainWindow::viewSource()
{
    QTextEdit *textEdit = new QTextEdit(nullptr);
    textEdit->setAttribute(Qt::WA_DeleteOnClose);
    textEdit->adjustSize();
    textEdit->move(this->geometry().center() - textEdit->rect().center());
    textEdit->show();

    view->page()->toHtml([textEdit](const QString &html){
        textEdit->setPlainText(html);
    });
}

void MainWindow::adjustLocation()
{
    locationEdit->setText(view->url().toString());
}

void MainWindow::changeLocation()
{
    QUrl url = QUrl::fromUserInput(locationEdit->text());
    view->load(url);
    view->setFocus();
}

void MainWindow::adjustTitle()
{
    if (progress <= 0 || progress >= 100)
        setWindowTitle(QString("%1 - %2").arg(view->title(), QString::number(QApplication::applicationPid())));
    else
        setWindowTitle(QStringLiteral("%1 (%2%)").arg(view->title()).arg(progress));
}

void MainWindow::setProgress(int p)
{
    progress = p;
    adjustTitle();
}

void MainWindow::finishLoading(bool ok)
{
    qDebug().noquote().nospace() << "loadFinished(" << ok << ")";
    if(ok) {
        renderProcessOk = true;
        progress = 100;
        adjustTitle();
        view->page()->runJavaScript(jQuery);

        rotateImages(rotateAction->isChecked());
    }
}

void MainWindow::handleRenderProcessTerminated(
    QWebEnginePage::RenderProcessTerminationStatus status, int exitCode)
{
    QStringList statusStrings { "terminated normally", "terminated abnormally", "crashed", "was killed", QString::number(status)};
    QString statusStr = status < 0 ? statusStrings.last() : status > QWebEnginePage::KilledTerminationStatus ? statusStrings.last() : statusStrings[status];

    qDebug() << "Render Process" << statusStr << "Exit code:" << exitCode;
    renderProcessOk = false;
}

void MainWindow::runJavaScript(const QString &code)
{
    if(!renderProcessOk)
    {
        qWarning() << "Render process is dead";

        if (reloadIfRendererTerminatedAction->isChecked()) {
            qWarning() << "Skipping runJavaScript, and reloading instead";
            view->reload();
        }
    }

    view->page()->runJavaScript(code);
}

void MainWindow::highlightAllLinks()
{
    QString code = QStringLiteral("qt.jQuery('a').each( function () { qt.jQuery(this).css('background-color', 'yellow') } )");
    runJavaScript(code);
}

void MainWindow::toggleHighlightAllLinks(bool checked)
{
    QString color = checked ? "yellow" : "";
    QString code = QStringLiteral("qt.jQuery('a').each( function () { qt.jQuery(this).css('background-color', '%1') } )").arg(color);
    runJavaScript(code);
}

void MainWindow::rotateImages(bool invert)
{
    QString code;

    if (invert)
        code = QStringLiteral("qt.jQuery('img').each( function () { qt.jQuery(this).css('transition', 'transform 2s'); qt.jQuery(this).css('transform', 'rotate(180deg)') } )");
    else
        code = QStringLiteral("qt.jQuery('img').each( function () { qt.jQuery(this).css('transition', 'transform 2s'); qt.jQuery(this).css('transform', 'rotate(0deg)') } )");
    runJavaScript(code);
}

void MainWindow::removeGifImages()
{
    QString code = QStringLiteral("qt.jQuery('[src*=gif]').remove()");
    runJavaScript(code);
}

void MainWindow::removeInlineFrames()
{
    QString code = QStringLiteral("qt.jQuery('iframe').remove()");
    runJavaScript(code);
}

void MainWindow::removeObjectElements()
{
    QString code = QStringLiteral("qt.jQuery('object').remove()");
    runJavaScript(code);
}

void MainWindow::removeEmbeddedElements()
{
    QString code = QStringLiteral("qt.jQuery('embed').remove()");
    runJavaScript(code);
}
