/****************************************************************************
**
** Copyright (C) 2018 Crimson AS <info@crimson.no>
** Contact: https://www.crimson.no
**
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <QWindow>
#include <QWidget>
#include <QGuiApplication>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>

#include "qgtkheaderbar.h"
#include "qgtkrefptr.h"

class QGtkHeaderBar::QGtkHeaderBarPrivate
{
public:
    QGtkRefPtr<GtkWidget> headerBar;
};

QGtkHeaderBar::QGtkHeaderBar(QObject *parent)
    : d(new QGtkHeaderBarPrivate)
{
    QWindow *win = 0;

    win = qobject_cast<QWindow*>(parent);
    if (!win) {
        QWidget *w = qobject_cast<QWidget*>(parent);

        if (!w->isVisible()) {
            w->show(); // ensure pwin creation
            w->hide();
        }

        qWarning() << "widget w" << w;
        if (w) {
            // ### this stuff all feels super sketchy
            //w = w->window();
            //if (!w) {
            //    qFatal("Created a QGtkHeaderBar on a widget with no parent window...");
            //}
            win = w->windowHandle();
            qWarning() << "widget win" << win;
            if (!win) {
                qWarning() << "widget npw" << w->nativeParentWidget();
                qWarning() << "widget npw win" << w->nativeParentWidget()->windowHandle();
                win = w->nativeParentWidget()->windowHandle();
                if (!win) {
                    qFatal("QGtkHeaderBar couldn't get window handle of widget...");
                }
            }
        }
    }

    // ensure that QPA resources are created
    win->create();

    d->headerBar = gtk_header_bar_new();

    QPlatformNativeInterface *platformNativeInterface = QGuiApplication::platformNativeInterface();
    GtkWidget *w = static_cast<GtkWidget*>(platformNativeInterface->nativeResourceForWindow("gtkwindow", win));
    if (!w) {
        qFatal("no GtkWidget! (not using the right QPA?)");
    }
    gtk_window_set_titlebar(GTK_WINDOW(w), d->headerBar.get());
}

QGtkHeaderBar::~QGtkHeaderBar()
{
    delete d;
}

GtkWidget *QGtkHeaderBar::headerBarWidget() const
{
    return d->headerBar.get();
}

