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

#include "qgtkscreen.h"

#include <QDebug>

QGtkScreen::QGtkScreen(GdkMonitor *monitor)
    : m_monitor(monitor)
{
}

QRect QGtkScreen::availableGeometry() const
{
    GdkRectangle geometry;
    gdk_monitor_get_workarea(m_monitor, &geometry);
    return QRect(geometry.x, geometry.y, geometry.width, geometry.height);
}

QRect QGtkScreen::geometry() const
{
    GdkRectangle geometry;
    gdk_monitor_get_geometry(m_monitor, &geometry);
    qDebug() << QRect(geometry.x, geometry.y, geometry.width, geometry.height);
    return QRect(geometry.x, geometry.y, geometry.width, geometry.height);
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
    return QPlatformScreen::physicalSize();

    // ### highdpi
    // for some reason, this makes fonts bizarrely huge ???
    //return QSizeF(gdk_monitor_get_width_mm(m_monitor), gdk_monitor_get_height_mm(m_monitor));
}

//QDpi QGtkScreen::logicalDpi() const
//{
//}

qreal QGtkScreen::devicePixelRatio() const
{
    return gdk_monitor_get_scale_factor(m_monitor);
}

qreal QGtkScreen::refreshRate() const
{
    // gdk gives us millihz..
    return gdk_monitor_get_refresh_rate(m_monitor) / 1000;
}

