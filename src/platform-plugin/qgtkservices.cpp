/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
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

#include "qgtkservices.h"

#include <QtCore/qdebug.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qurl.h>

#include <gio/gio.h>

QGtkServices::QGtkServices()
{

}

QGtkServices::~QGtkServices()
{

}

bool QGtkServices::openUrl(const QUrl &url)
{
    GError *err = nullptr;
    g_app_info_launch_default_for_uri(url.toString(QUrl::FullyEncoded).toUtf8().constData(), NULL, &err);
    if (err) {
        qWarning() << "Open failed: " << err->message;
        g_error_free(err);
        return false;
    }
    return true;
}

bool QGtkServices::openDocument(const QUrl &url)
{
    GError *err = nullptr;
    g_app_info_launch_default_for_uri(url.toString(QUrl::FullyEncoded).toUtf8().constData(), NULL, &err);
    if (err) {
        qWarning() << "Open failed: " << err->message;
        g_error_free(err);
        return false;
    }
    return true;
}

QByteArray QGtkServices::desktopEnvironment() const
{
    return qgetenv("XDG_CURRENT_DESKTOP");
}
