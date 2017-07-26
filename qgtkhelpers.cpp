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

// Returns the largest image for this icon, in RGB32 format.
static QImage qt_getBiggestImageForIcon(const QIcon &icon)
{
    QList<QSize> sizes = icon.availableSizes(QIcon::Normal, QIcon::On);
    if (!sizes.length()) {
        qWarning() << "No available icons for icon?" << icon;
        return QImage();
    }

    // Find the largest size, hopefully it's the best looking.
    QSize sz;
    for (int i = 0; i < sizes.length(); ++i) {
        const QSize &nsz = sizes.at(i);

        if (nsz.width() * nsz.height() >= sz.width() * sz.height()) {
            sz = nsz;
        }
    }

    QPixmap p = icon.pixmap(sz, QIcon::Normal, QIcon::On);
    QImage i = p.toImage().convertToFormat(QImage::Format_RGB32);
    return i;
}

// Convert a QIcon to a GdkPixbuf for use elsewhere.
// The GdkPixbuf is started with an initial refcount, so it must be
// unreffed by the caller.
GdkPixbuf *qt_iconToPixbuf(const QIcon &icon)
{
    QImage i = qt_getBiggestImageForIcon(icon);
    if (i.isNull())
        return 0;
    GdkPixbuf *gpb = gdk_pixbuf_new_from_data(
        i.constBits(),
        GDK_COLORSPACE_RGB,
        false,
        8,
        i.width(),
        i.height(),
        i.bytesPerLine(),
        NULL,
        NULL
    );

    return gpb;
}

// Convert a QIcon to a GIcon for use elsewhere.
// The GIcon is started with an initial refcount, so it must be unreffed by the
// caller.
GIcon *qt_iconToIcon(const QIcon &icon)
{
    QImage i = qt_getBiggestImageForIcon(icon);
    GBytes *bytes = g_bytes_new_take(const_cast<uchar*>(i.constBits()), i.byteCount());
    GIcon *ico = g_bytes_icon_new(bytes);
    g_bytes_unref(bytes);
    return ico;
}

// Qt uses &, gtk uses _.
QString qt_convertToGtkMnemonics(const QString &text)
{
    QString cpy = text;
    return cpy.replace("&", "_"); // ### too simple! need to leave &&.
}

