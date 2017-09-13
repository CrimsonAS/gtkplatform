/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgtkhelpers.h"
#include "qgtksystemtrayicon.h"
#include "qgtkintegration.h"

#include <QtCore/qdebug.h>
#include <QtGui/qicon.h>
#include <QtGui/private/qguiapplication_p.h>

// Status icons are deprecated in GTK+.
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")

QGtkSystemTrayIcon::QGtkSystemTrayIcon()
{
    m_icon = gtk_status_icon_new();
    qDebug() << "ctor" << m_icon;
}

QGtkSystemTrayIcon::~QGtkSystemTrayIcon()
{
    qDebug() << "dtor" << m_icon;
    g_object_unref(m_icon);
    m_icon = 0;
}

void QGtkSystemTrayIcon::init()
{
    qDebug() << "init" << m_icon;
}

void QGtkSystemTrayIcon::cleanup()
{
    qDebug() << "cleanup" << m_icon;
    g_object_unref(m_icon);
    m_icon = 0;
    m_icon = gtk_status_icon_new();
    qDebug() << "cleanup 2" << m_icon;
}

void QGtkSystemTrayIcon::updateIcon(const QIcon &icon)
{
    qDebug() << "updateIcon" << m_icon << icon;
    GdkPixbuf *gpb = qt_iconToPixbuf(icon);
    if (gpb) {
        gtk_status_icon_set_from_pixbuf(m_icon, gpb);
        g_object_unref(gpb);
    }
}

void QGtkSystemTrayIcon::updateToolTip(const QString &toolTip)
{
    qDebug() << "updateToolTip" << m_icon << toolTip;
    gtk_status_icon_set_tooltip_text(m_icon, toolTip.toUtf8().constData());
}

void QGtkSystemTrayIcon::updateMenu(QPlatformMenu *menu)
{
    // ### todo
    Q_UNUSED(menu);
    return;
}

QRect QGtkSystemTrayIcon::geometry() const
{
    int sz = gtk_status_icon_get_size(m_icon);
    return QRect(0, 0, sz, sz);
}

void action_cb(NotifyNotification*, gchar *, gpointer)
{
    if (qApp->focusWindow()) {
        qApp->focusWindow()->raise();
    }
}

void QGtkSystemTrayIcon::showMessage(const QString &title, const QString &msg,
                 const QIcon& icon, MessageIcon iconType, int msecs)
{
    NotifyNotification *n = notify_notification_new(title.toUtf8().constData(), msg.toUtf8().constData(), nullptr);

    // ### technically, we could delete this after 'msecs'. we need to keep it
    // around to fire action_cb.
    m_notification.reset(n);

    if (!icon.isNull()) {
        GdkPixbuf *ico = qt_iconToPixbuf(icon);
        notify_notification_set_icon_from_pixbuf(n, ico);
        g_object_unref(ico);
    }

    switch (iconType) {
    case QPlatformSystemTrayIcon::NoIcon:
    case QPlatformSystemTrayIcon::Information:
        notify_notification_set_urgency(n, NOTIFY_URGENCY_LOW);
        break;
    case QPlatformSystemTrayIcon::Warning:
        notify_notification_set_urgency(n, NOTIFY_URGENCY_NORMAL);
        break;
    case QPlatformSystemTrayIcon::Critical:
        notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);
        break;
    }

    notify_notification_set_timeout(n, msecs);
    notify_notification_add_action(n, "default", "default", action_cb, NULL, NULL);

    notify_notification_show(n, NULL);
}

bool QGtkSystemTrayIcon::isSystemTrayAvailable() const
{
    return true;
}

bool QGtkSystemTrayIcon::supportsMessages() const
{
    return true;
}

QT_WARNING_POP
