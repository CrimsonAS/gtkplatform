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

#include "qgtkscreen.h"
#include "qgtkcursor.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include <gtk/gtk.h>

Q_LOGGING_CATEGORY(lcScreen, "qt.qpa.gtk.screen");

QGtkScreen::QGtkScreen(GdkMonitor *monitor)
    : m_monitor(monitor)
    , m_cursor(new QGtkCursor())
{
}

QRect QGtkScreen::availableGeometry() const
{
    qreal dpr = devicePixelRatio();
    GdkRectangle geometry;
    gdk_monitor_get_workarea(m_monitor, &geometry);
    return QRect(geometry.x / dpr, geometry.y / dpr, geometry.width / dpr, geometry.height / dpr);
}

QRect QGtkScreen::geometry() const
{
    qreal dpr = devicePixelRatio();
    GdkRectangle geometry;
    gdk_monitor_get_geometry(m_monitor, &geometry);
    return QRect(geometry.x / dpr, geometry.y / dpr, geometry.width / dpr, geometry.height / dpr);
}

int QGtkScreen::depth() const
{
    return 32;
}

QImage::Format QGtkScreen::format() const
{
    return QImage::Format_ARGB32_Premultiplied;
}

QSizeF QGtkScreen::physicalSize() const
{
    return QSizeF(gdk_monitor_get_width_mm(m_monitor), gdk_monitor_get_height_mm(m_monitor));
}

QDpi QGtkScreen::logicalDpi() const
{
    // ### notify on change
    int dpi = -1;
    g_object_get(gtk_settings_get_default(), "gtk-xft-dpi", &dpi, NULL);
    if (dpi == -1) {
        dpi = 96;
    } else {
        dpi /= 1024;
    }
    return QDpi(dpi, dpi);
}

qreal QGtkScreen::devicePixelRatio() const
{
    return gdk_monitor_get_scale_factor(m_monitor);
}

qreal QGtkScreen::refreshRate() const
{
    // gdk gives us millihz..
    return gdk_monitor_get_refresh_rate(m_monitor) / 1000;
}

QPlatformCursor *QGtkScreen::cursor() const
{
    return m_cursor.get();
}
