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

#include "qgtkopenglcontext.h"
#include "qgtkwindow.h"
#include "qgtkintegration.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>
#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <EGL/egl.h>
#include <dlfcn.h>

#ifdef GDK_WINDOWING_WAYLAND

#if QT_VERSION >= QT_VERSION_CHECK(5,8,0)
#include <QtEglSupport/private/qeglconvenience_p.h>
#else
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#endif

#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include <QLoggingCategory>

#include "CSystrace.h"

Q_LOGGING_CATEGORY(lcContext, "qt.qpa.gtk.context");

// GDK creates an internal 'paint context' for each GDKWindow, and exposes
// only an API to create additional contexts which share with the paint
// context. Unfortunately, that means it's not possible to create a GDK
// context that can share with multiple GDKWindows, which is a requirement
// of how the QOpenGLContext API is structured. QOpenGLContext is created
// independently of surfaces and can be attached to different services at
// makeCurrent() time.
//
// Since the GDK APIs aren't really useful here, QGtkOpenGLContext creates
// contexts directly with EGL/GLX.
//
// Even more unfortunately, GDK as of now doesn't actually have opengl
// windows; all GL rendering happens into a framebuffer that is downloaded
// and composited by cairo. Since this is what GDK would be doing anyway,
// QGtkOpenGLContext downloads the framebuffer on swapBuffers and passes
// the raster image back to QGtkWindow to composite.

QGtkOpenGLContext::QGtkOpenGLContext(const QSurfaceFormat &format, QGtkOpenGLContext *shareContext)
    : m_shareContext(nullptr)
    , m_fbo(nullptr)
    , m_fbo_mirrored(nullptr)
{
    m_format = format;
    m_shareContext = shareContext;
}

QGtkOpenGLContext::~QGtkOpenGLContext()
{
    delete m_fbo;
    delete m_fbo_mirrored;
}

QSurfaceFormat QGtkOpenGLContext::format() const
{
    return m_format;
}

GLuint QGtkOpenGLContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    // XXX This will result in recreating FBOs if a context renders to differently
    // sized surfaces. It would be smarter to store the FBO with the surface, and
    // recreate it only if the context changes.
    Q_UNUSED(surface);
    Q_ASSERT(m_fbo);
    return m_fbo->handle();
}

void QGtkOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    TRACE_EVENT0("gfx", "QGtkOpenGLContext::swapBuffers");
    QGtkWindow *win = static_cast<QGtkWindow*>(surface);

    QImage *image = win->beginUpdateFrame("swapBuffers");

    // ### perhaps this should be done in one place (inside QGtkBackingStore)?
    if (image->size() != QSize(m_fbo_mirrored->width(), m_fbo_mirrored->height()) ||
        image->format() != QImage::Format_ARGB32)
    {
        *image = QImage(m_fbo_mirrored->width(), m_fbo_mirrored->height(), QImage::Format_ARGB32);
        image->setDevicePixelRatio(win->devicePixelRatio());
    }

    // Download rendered frame, slowly, so slowly.
    // First we need to invert y, otherwise we'll draw upside down. To do that,
    // blit to a framebuffer with inverted coordinate.
    QRect srcRect(0, image->size().height(), image->size().width(), -image->size().height());
    QRect destRect(0, 0, image->size().width(), image->size().height());
    QOpenGLFramebufferObject::blitFramebuffer(m_fbo_mirrored, destRect,
                                              m_fbo, srcRect,
                                              GL_COLOR_BUFFER_BIT, GL_LINEAR, 0, 0, QOpenGLFramebufferObject::DontRestoreFramebufferBinding);

    // Now read back the flipped data into the backing store image.
    QOpenGLFunctions funcs(QOpenGLContext::currentContext());
    m_fbo_mirrored->bind();
    funcs.glReadPixels(0, 0, image->width(), image->height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image->bits());

    win->endUpdateFrame("swapBuffers");
    win->invalidateRegion(QRegion());

    // If swap is called on the main thread, then assume that the application
    // knows what it is doing, and can self-throttle (either via requestUpdate,
    // or QWidget). We can't really do more than this because if we do block we
    // may lose the chance to process some events.
    //
    // If the application requested a swap interval, though, that means they
    // want us to block, so let's try so they don't render as fast as our little
    // bits can take us.
    if (m_format.swapInterval() > 0) {
        // So yeah, this isn't exactly an ideal throttling mechanism, but it
        // should be quite deadlock-proof, and works well enough for the time
        // being. We take the allowed frame time, and if we're finishing ahead
        // of the allowed time, we sleep. This isn't perfect in that it doesn't
        // include the time taken by gtk+ to get the frame onto the display but
        // it's better than nothing.
        qint64 timeBudget = 1000 / win->window()->screen()->refreshRate();
        qint64 delta = timeBudget - m_swapTimer.elapsed();
        if (m_swapTimer.isValid() && m_swapTimer.elapsed() < timeBudget) {
            TRACE_EVENT0("gfx", "QGtkOpenGLContext::swapBuffers::usleep");
            usleep(delta * 1000);
        }
        m_swapTimer.restart();
    }
}

bool QGtkOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    QGtkWindow *win = static_cast<QGtkWindow*>(surface);
    QSize sz = win->geometry().size() * win->devicePixelRatio();
    if (sz.isEmpty())
        sz = QSize(1, 1);

    if (m_fbo && m_fbo->size() != sz) {
        qCDebug(lcContext) << "clearing old context FBO of size" << m_fbo->size();
        delete m_fbo_mirrored;
        m_fbo_mirrored = nullptr;
        // XXX ###: I've seen defaultFramebufferObject getting called when a
        // QOpenGLFramebufferObject is deleted. That seems scary given this. To
        // reproduce, delete m_fbo_mirrored after m_fbo is already set to
        // nullptr, and watch it assert.
        delete m_fbo;
        m_fbo = nullptr;
    }
    if (!m_fbo) {
        m_fbo = new QOpenGLFramebufferObject(sz, QOpenGLFramebufferObject::CombinedDepthStencil);
        m_fbo_mirrored = new QOpenGLFramebufferObject(sz, QOpenGLFramebufferObject::CombinedDepthStencil);
        qCDebug(lcContext) << "created new context FBO of size" << m_fbo->size();
    }

    if (!m_fbo->isValid())
        return false;
    m_fbo->bind();
    return true;
}

void QGtkOpenGLContext::doneCurrent()
{
}

bool QGtkOpenGLContext::isSharing() const
{
    return m_shareContext;
}

bool QGtkOpenGLContext::isValid() const
{
    return false;
}

void *QGtkOpenGLContext::nativeResource(const QByteArray &resource) const
{
    Q_UNUSED(resource);
    return nullptr;
}
