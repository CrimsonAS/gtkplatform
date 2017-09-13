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

 #ifndef QGTKSYSTEMTRAYICON_P_H
 #define QGTKSYSTEMTRAYICON_P_H

 #include <QtCore/qglobal.h>

 #include "QtCore/qstring.h"
 #include "QtGui/qpa/qplatformsystemtrayicon.h"

#include <gtk/gtk.h>

#include <libnotify/notification.h>

 QT_BEGIN_NAMESPACE

 class Q_GUI_EXPORT QGtkSystemTrayIcon : public QPlatformSystemTrayIcon
 {
 public:
     QGtkSystemTrayIcon();
     ~QGtkSystemTrayIcon();

     void init() override;
     void cleanup() override;
     void updateIcon(const QIcon &icon) override;
     void updateToolTip(const QString &toolTip) override;
     void updateMenu(QPlatformMenu *menu) override;
     QRect geometry() const override;
     void showMessage(const QString &title, const QString &msg,
                      const QIcon& icon, MessageIcon iconType, int msecs) override;

     bool isSystemTrayAvailable() const override;
     bool supportsMessages() const override;

 private:
     GtkStatusIcon *m_icon;
     QGtkRefPtr<NotifyNotification> m_notification;
 };

 QT_END_NAMESPACE

 #endif // QGTKSYSTEMTRAYICON_P_H
