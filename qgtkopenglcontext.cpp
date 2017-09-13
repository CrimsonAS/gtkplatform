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

#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <EGL/egl.h>
#include <dlfcn.h>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcContext, "qt.qpa.gtk.context");

// GDK creates an internal 'paint context' for each GDKWindow, and exposes
// an API to create additional contexts which share with the paint context.
// Unfortunately, that means it's not possible to create a context that can
// share with multiple GDKWindows, which is a requirement of how the
// QOpenGLContext API is structured.
//
// As a painful workaround, QGtkOpenGLContext uses a fake window to create
// an independent context for use by Qt rendering, and explicitly copies to
// the surface's context by reading back and re-uploading the framebuffer.
//
// It may be possible on some drivers to map a buffer from this context and
// use the mapped pointer in an upload to another context, and have that
// actually bypass the CPU for copying. I can't find many references to
// that technique, but it may be worth experimenting.
//
// Otherwise, we need GDK to provide enough API to guarantee that all paint
// contexts end up shared with eachother. A way to specify a shared context
// for the paint context would be sufficient.

static void updateFormatFromContext(QSurfaceFormat &format)
{
    // Update the version, profile, and context bit of the format
    int major = 0, minor = 0;
    QByteArray versionString(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    qWarning() << "GL_VERSION is" << versionString;
    if (QPlatformOpenGLContext::parseOpenGLVersion(versionString, major, minor)) {
        format.setMajorVersion(major);
        format.setMinorVersion(minor);
    }

    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setOptions(QSurfaceFormat::FormatOptions());

    if (format.renderableType() == QSurfaceFormat::OpenGL) {
        if (format.version() < qMakePair(3, 0)) {
            format.setOption(QSurfaceFormat::DeprecatedFunctions);
            return;
        }

        // Version 3.0 onwards - check if it includes deprecated functionality or is
        // a debug context
        GLint value = 0;
        glGetIntegerv(GL_CONTEXT_FLAGS, &value);
        if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
            format.setOption(QSurfaceFormat::DeprecatedFunctions);
        if (value & GL_CONTEXT_FLAG_DEBUG_BIT)
            format.setOption(QSurfaceFormat::DebugContext);
        if (format.version() < qMakePair(3, 2))
            return;

        // Version 3.2 and newer have a profile
        value = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);

        if (value & GL_CONTEXT_CORE_PROFILE_BIT)
            format.setProfile(QSurfaceFormat::CoreProfile);
        else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
            format.setProfile(QSurfaceFormat::CompatibilityProfile);
    }

#if 0
    // These are all returning 0, for some reason?
    GLint depth = 0, red = 0, green = 0, blue = 0, alpha = 0, stencil = 0;
    glGetIntegerv(GL_DEPTH_BITS, &value);
    glGetIntegerv(GL_RED_BITS, &value);
    glGetIntegerv(GL_GREEN_BITS, &value);
    glGetIntegerv(GL_BLUE_BITS, &value);
    glGetIntegerv(GL_ALPHA_BITS, &value);
    glGetIntegerv(GL_STENCIL_BITS, &value);
#else
    GLint depth = 24, red = 8, green = 8, blue = 8, alpha = 8, stencil = 8;
#endif
    format.setDepthBufferSize(depth);
    format.setRedBufferSize(red);
    format.setGreenBufferSize(green);
    format.setBlueBufferSize(blue);
    format.setAlphaBufferSize(alpha);
    format.setStencilBufferSize(stencil);

    qDebug() << "Updated format, is now" << format;
}

QGtkOpenGLContext::QGtkOpenGLContext(const QSurfaceFormat &format)
    : m_format(format)
    , m_gdkContext(nullptr)
    , m_fbo(nullptr)
{
    // The only GDK API for creating a GL context requires a window,
    // but the created context is explicitly not tied exclusively to that
    // window. Create a fake window to create an independent context here.
    //
    // The alternative is to figure out the platform and directly create
    // wayland or GLX contexts without GDK. That would give us more
    // flexibility, but might be more fragile.
    GtkWidget *fakeGtkWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_realize(fakeGtkWin);
    GdkWindow *fakeGdkWin = gtk_widget_get_window(fakeGtkWin);
    m_gdkContext = gdk_window_create_gl_context(fakeGdkWin, NULL);
    Q_ASSERT(m_gdkContext);
    bool context_realized = gdk_gl_context_realize(m_gdkContext, NULL);
    Q_ASSERT(context_realized);
    gdk_gl_context_make_current(m_gdkContext);
    gtk_widget_destroy(fakeGtkWin);

    updateFormatFromContext(m_format);
}

QGtkOpenGLContext::QGtkOpenGLContext()
    : m_gdkContext(nullptr)
    , m_fbo(nullptr)
{
}

QGtkOpenGLContext::~QGtkOpenGLContext()
{
    // XXX leaking context
    delete m_fbo;
    qWarning() << "Stub";
}

void QGtkOpenGLContext::initialize()
{
}

QSurfaceFormat QGtkOpenGLContext::format() const
{
    return m_format;
}

GLuint QGtkOpenGLContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    Q_UNUSED(surface);
    Q_ASSERT(m_fbo);
    return m_fbo->handle();
}

void QGtkOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    qCDebug(lcContext) << "Swapping";
    QGtkWindow *win = static_cast<QGtkWindow*>(surface);

    // Download rendered frame, slowly, so slowly.
    QImage image(m_fbo->width(), m_fbo->height(), QImage::Format_ARGB32);
    QOpenGLFunctions funcs(QOpenGLContext::currentContext());
    funcs.glReadPixels(0, 0, image.width(), image.height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image.bits());
    image = image.mirrored(false, true);
    win->setWindowContents(image, QRegion(), QPoint());

    qDebug(lcContext) << "Done swapping";
}

bool QGtkOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    QGtkWindow *win = static_cast<QGtkWindow*>(surface);
    gdk_gl_context_make_current(m_gdkContext);

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
    gdk_gl_context_clear_current();
}

bool QGtkOpenGLContext::isSharing() const
{
    // Sharing isn't supported, because GDK doesn't give us control over
    // shared contexts. If that ends up being a serious problem, the
    // alternative is to try to create EGL/GLX contexts directly and hope
    // they end up compatible.
    return false;
}

bool QGtkOpenGLContext::isValid() const
{
    return m_gdkContext;
}

QFunctionPointer QGtkOpenGLContext::getProcAddress(const char *procName)
{
    // XXX no
    eglBindAPI(EGL_OPENGL_API); // ### EGL_OPENGL_ES_API?
    QFunctionPointer proc = (QFunctionPointer)eglGetProcAddress(procName);
    if (!proc) {
        proc = (QFunctionPointer)dlsym(RTLD_DEFAULT, procName);
    }
    return proc;
}

// QGtkOpenGLInternalContext represents GTK-side contexts for internal use, allowing
// QOpenGLContexts to be created for them and the use of Qt's OpenGL functions.
//
// These contexts do not render to the user-provided surface. They are purely a
// wrapper around a GTK context.
QGtkOpenGLInternalContext::QGtkOpenGLInternalContext(GdkGLContext *nativeContext)
{
    m_gdkContext = nativeContext;
    gdk_gl_context_make_current(m_gdkContext);
    updateFormatFromContext(m_format);
}

QGtkOpenGLInternalContext::~QGtkOpenGLInternalContext()
{
}

GLuint QGtkOpenGLInternalContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    Q_UNUSED(surface);
    return 0;
}

bool QGtkOpenGLInternalContext::makeCurrent(QPlatformSurface *surface)
{
    if (surface) {
        gdk_gl_context_make_current(m_gdkContext);
    } else {
        gdk_gl_context_clear_current();
    }
    return true;
}

void QGtkOpenGLInternalContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
    qWarning() << "QGtkOpenGLInternalContext cannot swap buffers, it does not manage surfaces";
}
