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

#include "qgtkbackingstore.h"
#include "qgtkintegration.h"
#include "qgtkwindow.h"
#include "qscreen.h"
#include <QtCore/qdebug.h>
#include <qpa/qplatformscreen.h>
#include <private/qguiapplication_p.h>

#include "CSystrace.h"

// we have no_keywords, but this header uses one.
#define foreach Q_FOREACH
#include <private/qhighdpiscaling_p.h>
#undef foreach

QGtkBackingStore::QGtkBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_paintImage(nullptr)
{
}

QGtkBackingStore::~QGtkBackingStore()
{
}

QPaintDevice *QGtkBackingStore::paintDevice()
{
    return m_paintImage;
}

QImage QGtkBackingStore::toImage() const
{
    return static_cast<QGtkWindow*>(window()->handle())->currentFrameImage();
}

void QGtkBackingStore::beginPaint(const QRegion &region)
{
    TRACE_EVENT_ASYNC_BEGIN0("gfx", "QGtkBackingStore::paint", this);
    Q_UNUSED(region);
    if (!m_paintImage)
        m_paintImage = static_cast<QGtkWindow*>(window()->handle())->beginUpdateFrame("beginPaint");
}

void QGtkBackingStore::endPaint()
{
    Q_ASSERT(m_paintImage);
    TRACE_EVENT_ASYNC_END0("gfx", "QGtkBackingStore::paint", this);
}

void QGtkBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                       QPlatformTextureList *textures, QOpenGLContext *context, bool translucentBackground)
{
    TRACE_EVENT0("gfx", "QGtkBackingStore::composeAndFlush");
    static_cast<QGtkWindow*>(window->handle())->endUpdateFrame("composeAndFlush");
    m_paintImage = nullptr;
    QPlatformBackingStore::composeAndFlush(window, region, offset, textures, context, translucentBackground);
}

void QGtkBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    TRACE_EVENT0("gfx", "QGtkBackingStore::flush");
    static_cast<QGtkWindow*>(window->handle())->endUpdateFrame("composeAndFlush");
    static_cast<QGtkWindow*>(window->handle())->invalidateRegion(region.translated(offset));
    m_paintImage = nullptr;
}

void QGtkBackingStore::resize(const QSize &size, const QRegion &)
{
    TRACE_EVENT0("gfx", "QGtkBackingStore::resize");
    QGtkWindow *qgwin = static_cast<QGtkWindow*>(window()->handle());

    QImage *image = qgwin->beginUpdateFrame("resize");
    qreal dpr = window()->devicePixelRatio() / QHighDpiScaling::factor(window());
    QSize realSize = size * dpr;
    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
    if (image->size() != realSize) {
        *image = QImage(realSize, format);
        image->setDevicePixelRatio(dpr);
    }
    qgwin->endUpdateFrame("resize");
}

bool QGtkBackingStore::scroll(const QRegion &region, int dx, int dy)
{
    // ### temporarily disabled
    //
    // this helps to accelerate widget scrolling by copying a region of the
    // backing store rather than re-rendering them from scratch. however, it's
    // currently not working quite right because of bad event ordering.
    //
    // the way this is supposed to work:
    // QWidget::scroll()
    //      -> backingStore->scroll()
    //          -> QPABackingStore->scroll()
    //          -> QWidget::update()
    // QPABackingStore::beginPaint
    // QWidget::paintEvent
    // ...
    // gtk_render_stuff
    //
    // But events are arriving out of order at the moment (perhaps due to event
    // dispatcher? perhaps due to something else?) and as a result we get:
    // QWidget::scroll
    //      -> backingStore->scroll
    //          -> QPABackingStore->scroll
    //          -> QWidget::update
    // gtk_render_stuff <--- OOPS! didn't render the area that we couldn't // scroll yet!
    // QPABackingStore::beginPaint
    // QWidget::paintEvent
    // gtk_render_stuff <- eventually consistent, but looks bad.
    return false;

    TRACE_EVENT0("gfx", "QGtkBackingStore::scroll");
    extern void qt_scrollRectInImage(QImage &, const QRect &, const QPoint &);
    QGtkWindow *qgwin = static_cast<QGtkWindow*>(window()->handle());
    const qreal dpr = qgwin->devicePixelRatio();
    const QPoint delta = QPoint(dx * dpr, dy * dpr);
    QImage *image = qgwin->beginUpdateFrame("scroll");

    for (const QRect &rect : region.rects()) {
        qt_scrollRectInImage(*image, QRect(rect.topLeft() * dpr, rect.size() * dpr), delta);
    }

    qgwin->endUpdateFrame("scroll");
    QRegion uregion = region.united(region.translated(delta));
    qgwin->invalidateRegion(uregion);

    return true;
}

