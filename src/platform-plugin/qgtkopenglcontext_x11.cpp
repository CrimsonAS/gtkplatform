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

// This file is not compiled if GDK does not support X11.
#include "qgtkopenglcontext.h"
#ifdef GDK_WINDOWING_X11

#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,8,0)
#include <QtGlxSupport/private/qglxconvenience_p.h>
#else
#include <QtPlatformSupport/private/qglxconvenience_p.h>
#endif

#include <gdk/gdkx.h>
#include <GL/glx.h>

#include <QLoggingCategory>
#include "CSystrace.h"

static void updateFormatFromContext(QSurfaceFormat &format)
{
    // Update the version, profile, and context bit of the format
    int major = 0, minor = 0;
    QByteArray versionString(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    if (QPlatformOpenGLContext::parseOpenGLVersion(versionString, major, minor)) {
        format.setMajorVersion(major);
        format.setMinorVersion(minor);
    }

    format.setProfile(QSurfaceFormat::NoProfile);
    format.setOptions(QSurfaceFormat::FormatOptions());

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

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

void QGtkX11Context::initialize()
{
    m_glxContext = nullptr;
    m_display = gdk_x11_get_default_xdisplay();

    if (!GDK_IS_X11_DISPLAY(gdk_display_get_default())) {
        qFatal("QGtkX11Context should not be used for non-X11 displays");
    }

    Display *display = reinterpret_cast<Display*>(m_display);
    int xScreen = gdk_x11_get_default_screen();

    GLXContext shareCtx = nullptr;
    if (m_shareContext)
        shareCtx = reinterpret_cast<GLXContext>(static_cast<QGtkX11Context*>(m_shareContext)->m_glxContext);

    // Requiring OpenGL 3.0+ for surfaceless contexts. It would be possible to add support for other configs.
    m_format.setRenderableType(QSurfaceFormat::OpenGL);
    if (m_format.version() < qMakePair(3, 0))
        m_format.setVersion(3, 0);

    GLXFBConfig glConfig = qglx_findConfig(display, xScreen, m_format);

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

    // Try to create an OpenGL context for each known OpenGL version in descending
    // order from the requested version.
    const int requestedVersion = m_format.majorVersion() * 10 + qMin(m_format.minorVersion(), 9);

    QVector<int> glVersions;
    if (requestedVersion > 45)
        glVersions << requestedVersion;

    // Don't bother with versions below 3.0
    glVersions << 45 << 44 << 43 << 42 << 41 << 40 << 33 << 32 << 31 << 30;

    for (int i = 0; !m_glxContext && i < glVersions.count(); i++) {
        const int version = glVersions[i];
        if (version > requestedVersion)
            continue;

        const int majorVersion = version / 10;
        const int minorVersion = version % 10;

        QVector<int> contextAttributes;
        contextAttributes << GLX_CONTEXT_MAJOR_VERSION_ARB << majorVersion
                          << GLX_CONTEXT_MINOR_VERSION_ARB << minorVersion;

        // If asking for OpenGL 3.2 or newer we should also specify a profile
        if (version >= 32) {
            if (m_format.profile() == QSurfaceFormat::CoreProfile)
                contextAttributes << GLX_CONTEXT_PROFILE_MASK_ARB << GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
            else
                contextAttributes << GLX_CONTEXT_PROFILE_MASK_ARB << GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        }

        int flags = 0;

        if (m_format.testOption(QSurfaceFormat::DebugContext))
            flags |= GLX_CONTEXT_DEBUG_BIT_ARB;

        // A forward-compatible context may be requested for 3.0 and later
        if (version >= 30 && !m_format.testOption(QSurfaceFormat::DeprecatedFunctions))
            flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

        if (flags != 0)
            contextAttributes << GLX_CONTEXT_FLAGS_ARB << flags;

        contextAttributes << None;

        m_glxContext = glXCreateContextAttribsARB(display, glConfig, shareCtx, true, contextAttributes.data());
        if (!m_glxContext && shareCtx) {
            // re-try without a shared glx context
            m_glxContext = glXCreateContextAttribsARB(display, glConfig, 0, true, contextAttributes.data());
            if (m_glxContext)
                m_shareContext = 0;
        }
    }

    if (m_glxContext) {
        qglx_surfaceFormatFromGLXFBConfig(&m_format, display, glConfig);
        // Query the OpenGL version and profile
        GLXContext prevContext = glXGetCurrentContext();
        GLXDrawable prevDrawable = glXGetCurrentDrawable();
        glXMakeContextCurrent(display, None, None, reinterpret_cast<GLXContext>(m_glxContext));
        updateFormatFromContext(m_format);
        // Make our context non-current
        glXMakeContextCurrent(display, prevDrawable, prevDrawable, prevContext);
    }
}

QGtkX11Context::~QGtkX11Context()
{
    if (m_glxContext) {
        glXDestroyContext(reinterpret_cast<Display*>(m_display), reinterpret_cast<GLXContext>(m_glxContext));
    }
}

bool QGtkX11Context::makeCurrent(QPlatformSurface *surface)
{
    TRACE_EVENT0("gfx", "QGtkX11Context::makeCurrent");
    if (!m_glxContext) {
        qWarning("No context in QGtkOpenGLContext::makeCurrent");
        return false;
    }

    // Assuming support for surfaceless contexts
    bool ok = glXMakeContextCurrent(reinterpret_cast<Display*>(m_display), None, None, reinterpret_cast<GLXContext>(m_glxContext));
    if (!ok)
        return ok;

    return QGtkOpenGLContext::makeCurrent(surface);
}

void QGtkX11Context::doneCurrent()
{
    TRACE_EVENT0("gfx", "QGtkX11Context::doneCurrent");
    glXMakeContextCurrent(reinterpret_cast<Display*>(m_display), None, None, NULL);
    QGtkOpenGLContext::doneCurrent();
}

bool QGtkX11Context::isValid() const
{
    return m_glxContext;
}

QFunctionPointer QGtkX11Context::getProcAddress(const char *procName)
{
    return (QFunctionPointer)glXGetProcAddressARB(reinterpret_cast<const GLubyte *>(procName));
}

#endif // GDK_WINDOWING_X11
