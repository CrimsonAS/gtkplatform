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

#ifndef QGTKOPENGLCONTEXT_H
#define QGTKOPENGLCONTEXT_H

#include <QtGui/qopenglframebufferobject.h>
#include <qpa/qplatformopenglcontext.h>
#include <gdk/gdk.h>

QT_BEGIN_NAMESPACE

class QGtkOpenGLContext : public QPlatformOpenGLContext
{
public:
    QGtkOpenGLContext(const QSurfaceFormat &format);
    ~QGtkOpenGLContext();

    void initialize() override;

    GLuint defaultFramebufferObject(QPlatformSurface *surface) const override;
    QSurfaceFormat format() const override;

    void swapBuffers(QPlatformSurface *surface) override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    bool isSharing() const override;
    bool isValid() const override;

    QFunctionPointer getProcAddress(const char *procName) override;

protected:
    QSurfaceFormat m_format;
    GdkGLContext *m_gdkContext;
    QOpenGLFramebufferObject *m_fbo;

    QGtkOpenGLContext();
};

class QGtkOpenGLInternalContext : public QGtkOpenGLContext
{
public:
    QGtkOpenGLInternalContext(GdkGLContext *nativeContext);
    ~QGtkOpenGLInternalContext();

    GLuint defaultFramebufferObject(QPlatformSurface *surface) const override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void swapBuffers(QPlatformSurface *surface) override;
};

QT_END_NAMESPACE

#endif // QGTKOPENGLCONTEXT_H

