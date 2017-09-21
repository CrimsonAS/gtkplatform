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
    m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig, m_format);
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                    shareEgl, NULL);
    Q_ASSERT(m_eglContext);
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
