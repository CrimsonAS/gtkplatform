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

#include "qgtkwindow.h"
#include "qgtkhelpers.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>
#include <QtGui/private/qwindow_p.h>

#include "CSystrace.h"

Q_LOGGING_CATEGORY(lcWindowRender, "qt.qpa.gtk.window.render");

// debug QGtkWindow lock on surface content
#undef LOCK_DEBUG

void QGtkWindow::drawCallback(GtkWidget *, cairo_t *cr, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "drawCallback" << pw;
    pw->onDraw(cr);
}

void QGtkWindow::onDraw(cairo_t *cr)
{
    if (m_newGeometry != m_windowGeometry) {
        qWarning() << "special case, dereffing" << thatThing.load();
        thatThing.deref(); // special case; to let it swap for us
        bool needsExpose = m_newGeometry.size() != m_windowGeometry.size();
        QWindowSystemInterface::handleGeometryChange(window(), m_newGeometry, m_windowGeometry);
        m_windowGeometry = m_newGeometry;

        if (needsExpose) {
            QWindowSystemInterface::handleExposeEvent(window(), m_windowGeometry);

            // we must flush, otherwise the content we'll render might be out of date.
            // ### would be nice if we could compress these, somehow: at the least we'll
            // get a configure and a size-allocate independent of each other.
            QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
        }
        thatThing.ref(); // undo special case
        qWarning() << "special case, re-reffed" << thatThing.load();
    }
    TRACE_EVENT0("gfx", "QGtkWindow::onDraw");
    // Hold frameMutex during blit to cairo to prevent changes
    QMutexLocker lock(&m_frameMutex);
#if defined(LOCK_DEBUG)
    qWarning() << "rendering (LOCKED)" << this;
#endif
    if (m_frame.isNull())
        return;

#if 0
    QString clipString;
    cairo_rectangle_list_t *clip = cairo_copy_clip_rectangle_list(cr);
    for (int i = 0; i < clip->num_rectangles; i++) {
        auto r = clip->rectangles[i];
        clipString += QString("%1,%2@%3x%4  ").arg(r.x).arg(r.y).arg(r.width).arg(r.height);
    }
    qDebug(lcWindow) << "onDraw with clip:" << clipString;
#endif

    cairo_surface_t *surf = cairo_image_surface_create_for_data(
            const_cast<uchar*>(m_frame.constBits()),
            CAIRO_FORMAT_ARGB32,
            m_frame.width(),
            m_frame.height(),
            m_frame.bytesPerLine()
    );
    int sf = gtk_widget_get_scale_factor(m_window.get());
    cairo_surface_set_device_scale(surf, sf, sf);
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    // cairo_paint respects the current clip, which GTK sets based on
    // updated regions of the window, so we don't need to do anything
    // other than include the updated regions in queue_draw calls.
    cairo_paint(cr);
    cairo_surface_destroy(surf);
#if defined(LOCK_DEBUG)
    qWarning() << "rendering (UNLOCKING)" << this;
#endif
}


gboolean QGtkWindow::windowTickCallback(GtkWidget*, GdkFrameClock *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "windowTickCallback" << pw;
    pw->onWindowTickCallback();
    return G_SOURCE_CONTINUE;
}


void QGtkWindow::onWindowTickCallback()
{
    TRACE_EVENT0("gfx", "QGtkWindow::onWindowTickCallback");
    if (m_wantsUpdate) {
        TRACE_EVENT_ASYNC_END0("gfx", "QGtkWindow::requestUpdate", this);
        m_wantsUpdate = false;
        thatThing.deref();
        qWarning() << "draw unref" << thatThing.load();
        QWindowPrivate::get(window())->deliverUpdateRequest();
    }
}

void QGtkWindow::requestUpdate()
{
    if (!m_wantsUpdate) {
        TRACE_EVENT_ASYNC_BEGIN0("gfx", "QGtkWindow::requestUpdate", this);
        m_wantsUpdate = true;
        thatThing.ref();
        qWarning() << "draw ref" << thatThing.load();
    }
}

QGtkCourierObject::QGtkCourierObject(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QGtkWindow*>("QGtkWindow*");
}

void QGtkCourierObject::queueDraw(QGtkWindow *win)
{
    gtk_widget_queue_draw(win->gtkWindow().get());
}

QGtkCourierObject *QGtkCourierObject::instance;

void QGtkWindow::invalidateRegion(const QRegion &region)
{
    auto courier = QGtkCourierObject::instance;
    Q_ASSERT(courier);
    if (courier->thread() != QThread::currentThread()) {
        // In the multithreaded case, always signal a full screen update for now
        courier->metaObject()->invokeMethod(courier, "queueDraw", Qt::QueuedConnection, Q_ARG(QGtkWindow*, this));
        return;
    }

    QRegion realRegion = region.isNull() ? QRegion(m_frame.rect()) : region;
    cairo_region_t *cairoRegion = qt_convertToCairoRegion(realRegion);
    gtk_widget_queue_draw_region(m_content.get(), cairoRegion);
    cairo_region_destroy(cairoRegion);
}

QImage *QGtkWindow::beginUpdateFrame(const QString &reason)
{
    m_frameMutex.lock();
    Q_UNUSED(reason);
#if defined(LOCK_DEBUG)
    qWarning() << "beginUpdateFrame " << reason << "(LOCKED)" << this;
#endif
    return &m_frame;
}

void QGtkWindow::endUpdateFrame(const QString &reason)
{
    m_frameMutex.unlock();
    Q_UNUSED(reason);
#if defined(LOCK_DEBUG)
    qWarning() << "endUpdateFrame " << reason << "(UNLOCKED)" << this;
#endif
}

QImage QGtkWindow::currentFrameImage() const
{
    return m_frame;
}

