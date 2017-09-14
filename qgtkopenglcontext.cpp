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

#include "qgtkopenglcontext.h"
#include "qgtkwindow.h"
#include "qgtkintegration.h"

#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <EGL/egl.h>
#include <dlfcn.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include <QLoggingCategory>

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
    : QGtkOpenGLContext()
{
    m_format = format;
    m_shareContext = shareContext;

    QGtkIntegration *integration = QGtkIntegration::instance();
#ifdef GDK_WINDOWING_WAYLAND
    if ((m_eglDisplay = integration->eglDisplay())) {
        m_eglConfig = q_configFromGLFormat(m_eglDisplay, format);
        m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig, m_format);
        m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                        shareContext ? shareContext->m_eglContext : NULL,
                                        NULL);
        Q_ASSERT(m_eglContext);
    }
    else
#endif
    {
        qWarning("GTK platform does not support this display backend for GL contexts");
    }
}

QGtkOpenGLContext::QGtkOpenGLContext()
    : m_eglContext(nullptr)
    , m_eglDisplay(nullptr)
    , m_eglConfig(nullptr)
    , m_shareContext(nullptr)
    , m_fbo(nullptr)
{
}

QGtkOpenGLContext::~QGtkOpenGLContext()
{
    delete m_fbo;
    if (m_eglContext) {
        eglDestroyContext(m_eglDisplay, m_eglContext);
    }
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
    qCDebug(lcContext) << "Swapping";
    QGtkWindow *win = static_cast<QGtkWindow*>(surface);

    // Download rendered frame, slowly, so slowly.
    QImage *image = win->beginUpdateFrame();
    if (image->size() != QSize(m_fbo->width(), m_fbo->height()) ||
        image->format() != QImage::Format_ARGB32)
    {
        *image = QImage(m_fbo->width(), m_fbo->height(), QImage::Format_ARGB32);
        image->setDevicePixelRatio(win->devicePixelRatio());
    }
    QOpenGLFunctions funcs(QOpenGLContext::currentContext());
    funcs.glReadPixels(0, 0, image->width(), image->height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image->bits());
    *image = std::move(*image).mirrored(false, true);

    // XXX In singlethreaded rendering, we can't really block swapBuffers because
    // this would also block the GTK loop, and any workaround to that seems insane.
    // This is probably okay in that case anyway, because a singlethreaded renderer
    // can't run on a swapBuffers loop.
    //
    // But in multithreaded rendering, having a nonblocking swapBuffers seems likely
    // to cause runaway rendering. This should probably block until the image has
    // been drawn onto the surface to properly throttle the rendering thread.
    win->endUpdateFrame();
    win->invalidateRegion(QRegion());

    qDebug(lcContext) << "Done swapping";
}

bool QGtkOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    if (m_eglContext) {
        bool ok = eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext);
        if (!ok) {
            qWarning() << "eglMakeCurrent failed";
            return ok;
        }
    } else {
        qWarning("No context in QGtkOpenGLContext::makeCurrent");
        return false;
    }

    QGtkWindow *win = static_cast<QGtkWindow*>(surface);
    QSize sz = win->geometry().size() * win->devicePixelRatio();
    if (m_fbo && m_fbo->size() != sz) {
        qCDebug(lcContext) << "clearing old context FBO of size" << m_fbo->size();
        delete m_fbo;
        m_fbo = nullptr;
    }
    if (!m_fbo) {
        m_fbo = new QOpenGLFramebufferObject(sz, QOpenGLFramebufferObject::CombinedDepthStencil);
        qCDebug(lcContext) << "created new context FBO of size" << m_fbo->size();
    }

    if (!m_fbo->isValid())
        return false;
    m_fbo->bind();
    return true;
}

void QGtkOpenGLContext::doneCurrent()
{
    if (m_eglContext) {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}

bool QGtkOpenGLContext::isSharing() const
{
    return m_shareContext;
}

bool QGtkOpenGLContext::isValid() const
{
    return m_eglContext;
}

QFunctionPointer QGtkOpenGLContext::getProcAddress(const char *procName)
{
    eglBindAPI(EGL_OPENGL_API); // ### EGL_OPENGL_ES_API?
    QFunctionPointer proc = (QFunctionPointer)eglGetProcAddress(procName);
    if (!proc) {
        proc = (QFunctionPointer)dlsym(RTLD_DEFAULT, procName);
    }
    return proc;
}

EGLContext QGtkOpenGLContext::eglContext() const
{
    return m_eglContext;
}

EGLDisplay QGtkOpenGLContext::eglDisplay() const
{
    return m_eglDisplay;
}

EGLConfig QGtkOpenGLContext::eglConfig() const
{
    return m_eglConfig;
}
