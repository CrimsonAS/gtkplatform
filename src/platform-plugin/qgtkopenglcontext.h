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

#ifndef QGTKOPENGLCONTEXT_H
#define QGTKOPENGLCONTEXT_H

#include <QtCore/qelapsedtimer.h>
#include <QtGui/qopenglframebufferobject.h>
#include <qpa/qplatformopenglcontext.h>
#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_WAYLAND
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLConfig;
#endif

QT_BEGIN_NAMESPACE

class QGtkOpenGLContext : public QPlatformOpenGLContext
{
public:
    QGtkOpenGLContext(const QSurfaceFormat &format, QGtkOpenGLContext *shareContext);
    ~QGtkOpenGLContext();

    GLuint defaultFramebufferObject(QPlatformSurface *surface) const override;
    QSurfaceFormat format() const override;

    void swapBuffers(QPlatformSurface *surface) override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    bool isSharing() const override;
    bool isValid() const override;

    virtual void *nativeResource(const QByteArray &resource) const;

protected:
    QSurfaceFormat m_format;
    QGtkOpenGLContext *m_shareContext;
    QOpenGLFramebufferObject *m_fbo;
    QOpenGLFramebufferObject *m_fbo_mirrored;
    QElapsedTimer m_swapTimer;
};

#ifdef GDK_WINDOWING_WAYLAND
class QGtkWaylandContext : public QGtkOpenGLContext
{
public:
    using QGtkOpenGLContext::QGtkOpenGLContext;
    virtual ~QGtkWaylandContext();

    void initialize() override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    QFunctionPointer getProcAddress(const char *procName) override;

    bool isValid() const override;

    virtual void *nativeResource(const QByteArray &resource) const override;
    EGLContext eglContext() const;
    EGLDisplay eglDisplay() const;
    EGLConfig eglConfig() const;

protected:
    EGLContext m_eglContext;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
};
#endif

#ifdef GDK_WINDOWING_X11
class QGtkX11Context : public QGtkOpenGLContext
{
    using QGtkOpenGLContext::QGtkOpenGLContext;
    virtual ~QGtkX11Context();

    void initialize() override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    QFunctionPointer getProcAddress(const char *procName) override;

    bool isValid() const override;

protected:
    void *m_display;
    void *m_glxContext;
};
#endif

QT_END_NAMESPACE

#endif // QGTKOPENGLCONTEXT_H

