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

#include <gtk/gtk.h>
#include <EGL/egl.h>
#include <dlfcn.h>

QGtkOpenGLContext::QGtkOpenGLContext(const QSurfaceFormat &desiredFormat)
    : m_fbo(0)
{
    QSurfaceFormat format(desiredFormat);
    format.setAlphaBufferSize(8);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setProfile(QSurfaceFormat::CoreProfile);
    m_format = format;
}

QGtkOpenGLContext::~QGtkOpenGLContext()
{

    qWarning() << "Stub";
}

void QGtkOpenGLContext::initialize()
{
    m_fbo = new QOpenGLFramebufferObject(400, 400, GL_TEXTURE_2D);
    qWarning() << "Stub";
}

QSurfaceFormat QGtkOpenGLContext::format() const
{
    return m_format;
}

GLuint QGtkOpenGLContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    return m_fbo->handle();
}

void QGtkOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    //usleep(40000);
    qWarning() << "Stub";

}
bool QGtkOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    QGtkWindow *win = static_cast<QGtkWindow*>(surface);
    win->setFramebuffer(m_fbo);
    GdkGLContext *ctx = win->gdkGLContext();
    qDebug() << "Making current";
    gdk_gl_context_make_current(ctx);
    qDebug() << "Done making current";
    return true;
}

void QGtkOpenGLContext::doneCurrent()
{
    qDebug() << "Done clearing current";
    gdk_gl_context_clear_current();
    qDebug() << "Done clearing current";
}

bool QGtkOpenGLContext::isSharing() const
{
    qWarning() << "Stub";
    return false;
}
bool QGtkOpenGLContext::isValid() const
{
    qWarning() << "Stub";
    return true;
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
