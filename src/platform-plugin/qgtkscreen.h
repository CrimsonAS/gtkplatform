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

#ifndef QGTKSCREEN_H
#define QGTKSCREEN_H

#include <qpa/qplatformscreen.h>

#include <gdk/gdk.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QGtkCursor;

class QGtkScreen : public QPlatformScreen
{
public:
    QGtkScreen(GdkMonitor *monitor);

    QRect availableGeometry() const override;
    QRect geometry() const override;
    int depth() const override;
    QImage::Format format() const override;
    QSizeF physicalSize() const override;
    QDpi logicalDpi() const override;
    qreal devicePixelRatio() const override;
    qreal refreshRate() const override;
    QPlatformCursor *cursor() const override;

    GdkMonitor *monitor() const { return m_monitor; }
    bool isPrimary() const { return m_isPrimary; }
    void setPrimary(bool p) { m_isPrimary = p; }

public:
    GdkMonitor *m_monitor;
    std::unique_ptr<QGtkCursor> m_cursor;
    bool m_isPrimary = false;
};


QT_END_NAMESPACE

#endif // QGTKSCREEN_H
