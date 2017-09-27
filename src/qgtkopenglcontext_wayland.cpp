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

// This file is not compiled if GDK does not support Wayland.
#include "qgtkopenglcontext.h"
#ifdef GDK_WINDOWING_WAYLAND

#include "qgtkintegration.h"

#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>

#include <EGL/egl.h>
#include <gdk/gdkwayland.h>
#include <dlfcn.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,8,0)
#include <QtEglSupport/private/qeglconvenience_p.h>
#else
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#endif

#include <QLoggingCategory>
#include "CSystrace.h"

static QSurfaceFormat qgtk_wayland_update_format(const QSurfaceFormat &refFormat, EGLDisplay display, EGLContext context);

void QGtkWaylandContext::initialize()
{
    QGtkIntegration *integration = QGtkIntegration::instance();
    m_eglDisplay = integration->eglDisplay();
    // QGtkWaylandContext should not be created on non-wayland displays
    Q_ASSERT(m_eglDisplay);

    EGLContext shareEgl = nullptr;
    if (m_shareContext)
        shareEgl = static_cast<QGtkWaylandContext*>(m_shareContext)->eglContext();

    m_eglConfig = q_configFromGLFormat(m_eglDisplay, m_format);

    QVector<EGLint> eglContextAttrs;
    eglContextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
    eglContextAttrs.append(m_format.majorVersion());
    const bool hasKHRCreateContext = q_hasEglExtension(m_eglDisplay, "EGL_KHR_create_context");
    if (hasKHRCreateContext) {
        eglContextAttrs.append(EGL_CONTEXT_MINOR_VERSION_KHR);
        eglContextAttrs.append(m_format.minorVersion());
        int flags = 0;
        // The debug bit is supported both for OpenGL and OpenGL ES.
        if (m_format.testOption(QSurfaceFormat::DebugContext))
            flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
        // The fwdcompat bit is only for OpenGL 3.0+.
        if (m_format.renderableType() == QSurfaceFormat::OpenGL
            && m_format.majorVersion() >= 3
            && !m_format.testOption(QSurfaceFormat::DeprecatedFunctions))
            flags |= EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;
        if (flags) {
            eglContextAttrs.append(EGL_CONTEXT_FLAGS_KHR);
            eglContextAttrs.append(flags);
        }
        // Profiles are OpenGL only and mandatory in 3.2+. The value is silently ignored for < 3.2.
        if (m_format.renderableType() == QSurfaceFormat::OpenGL) {
            eglContextAttrs.append(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
            eglContextAttrs.append(m_format.profile() == QSurfaceFormat::CoreProfile
                                ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
                                : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR);
        }
    }
    eglContextAttrs.append(EGL_NONE);

    m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig, m_format);
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                    shareEgl, eglContextAttrs.data());

	if (m_eglContext)
        m_format = qgtk_wayland_update_format(m_format, m_eglDisplay, m_eglContext);
}

// Gently borrowed from the wayland QPA plugin; this code appears in several places,
// it should really be put into eglconvenience instead.
static QSurfaceFormat qgtk_wayland_update_format(const QSurfaceFormat &refFormat, EGLDisplay display, EGLContext context)
{
	QSurfaceFormat format = refFormat;

    // Have to save & restore to prevent QOpenGLContext::currentContext() from becoming
    // inconsistent after QOpenGLContext::create().
    EGLDisplay prevDisplay = eglGetCurrentDisplay();
    if (prevDisplay == EGL_NO_DISPLAY) // when no context is current
        prevDisplay = display;
    EGLContext prevContext = eglGetCurrentContext();
    EGLSurface prevSurfaceDraw = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface prevSurfaceRead = eglGetCurrentSurface(EGL_READ);

    if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
        if (format.renderableType() == QSurfaceFormat::OpenGL
            || format.renderableType() == QSurfaceFormat::OpenGLES) {
            const GLubyte *s = glGetString(GL_VERSION);
            if (s) {
                QByteArray version = QByteArray(reinterpret_cast<const char *>(s));
                int major, minor;
                if (QPlatformOpenGLContext::parseOpenGLVersion(version, major, minor)) {
                    format.setMajorVersion(major);
                    format.setMinorVersion(minor);
                }
            }
            format.setProfile(QSurfaceFormat::NoProfile);
            format.setOptions(QSurfaceFormat::FormatOptions());
            if (format.renderableType() == QSurfaceFormat::OpenGL) {
                // Check profile and options.
                if (format.majorVersion() < 3) {
                    format.setOption(QSurfaceFormat::DeprecatedFunctions);
                } else {
                    GLint value = 0;
                    glGetIntegerv(GL_CONTEXT_FLAGS, &value);
                    if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
                        format.setOption(QSurfaceFormat::DeprecatedFunctions);
                    if (value & GL_CONTEXT_FLAG_DEBUG_BIT)
                        format.setOption(QSurfaceFormat::DebugContext);
                    if (format.version() >= qMakePair(3, 2)) {
                        value = 0;
                        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);
                        if (value & GL_CONTEXT_CORE_PROFILE_BIT)
                            format.setProfile(QSurfaceFormat::CoreProfile);
                        else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
                            format.setProfile(QSurfaceFormat::CompatibilityProfile);
                    }
                }
            }
        }
        eglMakeCurrent(prevDisplay, prevSurfaceDraw, prevSurfaceRead, prevContext);
    }

    return format;
}

QGtkWaylandContext::~QGtkWaylandContext()
{
    if (m_eglContext) {
        eglDestroyContext(m_eglDisplay, m_eglContext);
    }
}

bool QGtkWaylandContext::makeCurrent(QPlatformSurface *surface)
{
    TRACE_EVENT0("gfx", "QGtkWaylandContext::makeCurrent");
    if (!m_eglContext) {
        qWarning("No context in QGtkOpenGLContext::makeCurrent");
        return false;
    }

    bool ok = eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext);
    if (!ok) {
        qWarning() << "eglMakeCurrent failed";
        return ok;
    }

    return QGtkOpenGLContext::makeCurrent(surface);
}

void QGtkWaylandContext::doneCurrent()
{
    TRACE_EVENT0("gfx", "QGtkWaylandContext::doneCurrent");
    if (m_eglContext) {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    QGtkOpenGLContext::doneCurrent();
}

bool QGtkWaylandContext::isValid() const
{
    return m_eglContext;
}

QFunctionPointer QGtkWaylandContext::getProcAddress(const char *procName)
{
    eglBindAPI(EGL_OPENGL_API); // ### EGL_OPENGL_ES_API?
    QFunctionPointer proc = (QFunctionPointer)eglGetProcAddress(procName);
    if (!proc) {
        proc = (QFunctionPointer)dlsym(RTLD_DEFAULT, procName);
    }
    return proc;
}

void *QGtkWaylandContext::nativeResource(const QByteArray &resource) const
{
    if (resource == "eglcontext") {
        return m_eglContext;
    } else if (resource == "eglconfig") {
        return m_eglConfig;
    } else if (resource == "egldisplay") {
        return m_eglDisplay;
    } else {
        return QGtkOpenGLContext::nativeResource(resource);
    }
}

EGLContext QGtkWaylandContext::eglContext() const
{
    return m_eglContext;
}

EGLDisplay QGtkWaylandContext::eglDisplay() const
{
    return m_eglDisplay;
}

EGLConfig QGtkWaylandContext::eglConfig() const
{
    return m_eglConfig;
}

#endif // GDK_WINDOWING_WAYLAND
