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
    , m_paintImage(nullptr)
{
    qDebug() << "QGtkBackingStore";
}

QGtkBackingStore::~QGtkBackingStore()
{
}

QPaintDevice *QGtkBackingStore::paintDevice()
{
    return m_paintImage;
}

// beginPaint locks QGtkWindow::m_frame and exposes it for painting. The surface
// will remain locked until flushed -- it is _not_ released at endPaint. It's
// unclear if this is safe, but it allows us to safely update the changed region
// as well.

void QGtkBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);
    if (!m_paintImage)
        m_paintImage = static_cast<QGtkWindow*>(window()->handle())->beginUpdateFrame();
}

void QGtkBackingStore::endPaint()
{
    Q_ASSERT(m_paintImage);
}

void QGtkBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_ASSERT(m_paintImage);
    static_cast<QGtkWindow*>(window->handle())->endUpdateFrame(region.translated(offset));
    m_paintImage = nullptr;
}

void QGtkBackingStore::resize(const QSize &size, const QRegion &)
{
    QGtkWindow *qgwin = static_cast<QGtkWindow*>(window()->handle());

    QImage *image = qgwin->beginUpdateFrame();
    qreal dpr = window()->devicePixelRatio() / QHighDpiScaling::factor(window());
    QSize realSize = size * dpr;
    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
    if (image->size() != realSize) {
        *image = QImage(realSize, format);
        image->setDevicePixelRatio(dpr);
    }
    qgwin->endUpdateFrame(QRegion());
}

QT_END_NAMESPACE
