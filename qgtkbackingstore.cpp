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


#include "qgtkbackingstore.h"
#include "qgtkintegration.h"
#include "qgtkwindow.h"
#include "qscreen.h"
#include <QtCore/qdebug.h>
#include <qpa/qplatformscreen.h>
#include <private/qguiapplication_p.h>

// we have no_keywords, but this header uses one.
#define foreach Q_FOREACH
#include <private/qhighdpiscaling_p.h>
#undef foreach

QT_BEGIN_NAMESPACE

QGtkBackingStore::QGtkBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
    qDebug() << "QGtkBackingStore";
}

QGtkBackingStore::~QGtkBackingStore()
{
}

QPaintDevice *QGtkBackingStore::paintDevice()
{
    return &mImage;
}

void QGtkBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);

    //qDebug() << "flush: " << window << region << offset;
    // ### todo can we somehow use the cairo surface directly?
    static_cast<QGtkWindow*>(window->handle())->setWindowContents(mImage, region, offset);
}

void QGtkBackingStore::resize(const QSize &size, const QRegion &)
{
    qreal dpr = window()->devicePixelRatio() / QHighDpiScaling::factor(window());
    QSize realSize = size * dpr;
    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
    if (mImage.size() != realSize) {
        mImage = QImage(realSize, format);
        mImage.setDevicePixelRatio(dpr);
        mImage.fill(Qt::red);
    }
}

QT_END_NAMESPACE
