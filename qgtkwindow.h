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

#ifndef QGTKWINDOW_H
#define QGTKWINDOW_H

#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/private/qopengltextureblitter_p.h>

#include <gtk/gtk.h>

QT_BEGIN_NAMESPACE

class QTouchDevice;
class QOpenGLTexture;

class QGtkWindow : public QPlatformWindow
{
public:
    QGtkWindow(QWindow *window);
    ~QGtkWindow();

    QSurfaceFormat format() const override;

    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;
    QRect normalGeometry() const override;
    qreal devicePixelRatio() const;

    QMargins frameMargins() const override;

    void setVisible(bool visible) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    void setWindowState(Qt::WindowState state) override;

    WId winId() const override;
    void setParent(const QPlatformWindow *window) override;

    void setWindowTitle(const QString &title) override;
    void setWindowFilePath(const QString &title) override;
    void setWindowIcon(const QIcon &icon) override;
    void raise() override;
    void lower() override;

    bool isExposed() const override;
    bool isActive() const override;
    void propagateSizeHints() override;
    void setOpacity(qreal level) override;
/*
    void setMask(const QRegion &region) override;
    void requestActivateWindow() override;

    void handleContentOrientationChange(Qt::ScreenOrientation orientation) override;
*/
    bool setKeyboardGrabEnabled(bool grab) override;
    bool setMouseGrabEnabled(bool grab) override;
/*
    bool setWindowModified(bool modified) override;

    void windowEvent(QEvent *event) override;

    bool startSystemResize(const QPoint &pos, Qt::Corner corner) override;

    void setFrameStrutEventsEnabled(bool enabled) override;
    bool frameStrutEventsEnabled() const override;
*/
    void setAlertState(bool enabled) override;
    bool isAlertState() const override;
/*
    void invalidateSurface() override;
    void requestUpdate() override;
*/

    // End API, start implementation.
    void onDraw(cairo_t *cr);
    void onRender();
    void onMap();
    void onUnmap();
    void onConfigure(GdkEvent *event);
    bool onDelete();
    bool onKeyPress(GdkEvent *event);
    bool onKeyRelease(GdkEvent *event);
    bool onButtonPress(GdkEvent *event);
    bool onButtonRelease(GdkEvent *event);
    bool onMotionNotify(GdkEvent *event);
    bool onTouchEvent(GdkEvent *event);
    bool onScrollEvent(GdkEvent *event);
    void setWindowContents(const QImage &image, const QRegion &region, const QPoint &offset);

    GtkMenuBar *gtkMenuBar() const;
    GtkWidget *gtkWindow() const;
    GdkGLContext *gdkGLContext() const;

    void updateRenderBuffer(const QByteArray &buffer, const QSize &size);

private:
    static Qt::KeyboardModifiers convertGdkKeyboardModsToQtKeyboardMods(guint mask);

    GtkWidget *m_window = nullptr;
    GtkMenuBar *m_menubar = nullptr;
    GtkWidget *m_content = nullptr;
    QImage m_image;
    QTouchDevice *m_touchDevice = nullptr;
    QList<QWindowSystemInterface::TouchPoint> m_activeTouchPoints;
    Qt::MouseButtons m_buttons;
    GdkGLContext *m_gtkContext = nullptr;
    QOpenGLContext *m_gtkContextQt = nullptr;
    QOpenGLTexture *m_surfaceTexture = nullptr;
    QOpenGLTextureBlitter m_surfaceBlitter;
    QByteArray m_renderBuffer;
    QSize m_renderBufferSize;
};

QT_END_NAMESPACE

#endif // QGTKWINDOW_H
