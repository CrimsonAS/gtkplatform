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

#ifndef QGTKWINDOW_H
#define QGTKWINDOW_H

#include "qgtkrefptr.h"

#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>

#include <gtk/gtk.h>

QT_BEGIN_NAMESPACE

class QTouchDevice;

class QGtkWindow : public QObject, public QPlatformWindow
{
public:
    QGtkWindow(QWindow *window);
    ~QGtkWindow();

    void create(Qt::WindowType windowType);

    QSurfaceFormat format() const override;

    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;
    QRect normalGeometry() const override;
    qreal devicePixelRatio() const;

    QMargins frameMargins() const override;

    void setVisible(bool visible) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
    void setWindowState(Qt::WindowStates state) override;
#else
    void setWindowState(Qt::WindowState state) override;
#endif

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
    void requestActivateWindow() override;
    void setMask(const QRegion &region) override;
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
*/
    void requestUpdate() override;

    // End API, start implementation.
    void onDraw(cairo_t *cr);
    void onRender();
    void onMap();
    void onUnmap();
    void onConfigure();
    bool onDelete();
    bool onKeyPress(GdkEvent *event);
    bool onKeyRelease(GdkEvent *event);
    bool onButtonPress(GdkEvent *event);
    bool onButtonRelease(GdkEvent *event);
    bool onMotionNotify(GdkEvent *event);
    bool onTouchEvent(GdkEvent *event);
    bool onScrollEvent(GdkEvent *event);
    void onWindowStateEvent(GdkEvent *event);
    void onWindowTickCallback();
    void onEnterLeaveWindow(GdkEvent *event, bool entered);
    void onLeaveContent();
    QImage *beginUpdateFrame(const QString &reason);
    void endUpdateFrame(const QString &reason);
    void invalidateRegion(const QRegion &region);
    QImage currentFrameImage() const;

    QGtkRefPtr<GtkMenuBar> gtkMenuBar() const;
    QGtkRefPtr<GtkWidget> gtkWindow() const;

    void beginZoom(QPointF &contentPoint, guint32 ts);
    void zoom(QPointF &contentPoint, double scale, guint32 ts);
    void endZoom(QPointF &contentPoint, guint32 ts);

    void beginRotate(QPointF &contentPoint, guint32 ts);
    void rotate(QPointF &contentPoint, double angle, double angle_delta, guint32 ts);
    void endRotate(QPointF &contentPoint, guint32 ts);

private:
    void maybeForceTransientParent(Qt::WindowType windowType);
    void reallyForceTransientFor(QWindow *transientParent);

    QGtkRefPtr<GtkWidget> m_window;
    QGtkRefPtr<GtkMenuBar> m_menubar;
    QGtkRefPtr<GtkWidget> m_content;
    QMutex m_frameMutex;
    QImage m_frame;
    QTouchDevice *m_touchDevice = nullptr;
    QList<QWindowSystemInterface::TouchPoint> m_activeTouchPoints;
    Qt::MouseButtons m_buttons;
    Qt::WindowState m_state = Qt::WindowNoState;
    bool m_wantsUpdate = false;
    guint m_tick_callback = 0;
    Qt::WindowFlags m_flags = Qt::Widget;
    QRect m_windowGeometry; // must be cached as it's accessed from multiple threads
    QRect m_newGeometry;
    Qt::KeyboardModifiers m_scrollModifiers = Qt::NoModifier;
    bool m_scrollStarted = false;

    static void drawCallback(GtkWidget *, cairo_t *cr, gpointer platformWindow);
    static gboolean windowTickCallback(GtkWidget*, GdkFrameClock *, gpointer platformWindow);

    static void zoom_cb(GtkGestureZoom *pt, gdouble scale, gpointer platformWindow);
    static void begin_zoom_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow);
    static void end_zoom_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow);
    static void cancel_zoom_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow);

    static void rotate_cb(GtkGestureRotate *pt, gdouble angle, gdouble angle_delta, gpointer platformWindow);
    static void begin_rotate_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow);
    static void end_rotate_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow);
    static void cancel_rotate_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow);

    QGtkRefPtr<GtkGesture> m_zoomGesture;
    QGtkRefPtr<GtkGesture> m_rotateGesture;
    int m_activeNativeGestures = 0;
    bool m_initialZoomSet = false;
    double m_initialZoom = 0;
    bool m_initialRotateSet = false;
    double m_initialRotate = 0;
    bool m_hasTickCallback = false;
    QTimer *m_cancelTickTimer = nullptr;
};

class QGtkCourierObject : public QObject
{
    Q_OBJECT

public:
    static QGtkCourierObject *instance;

    QGtkCourierObject(QObject *parent = nullptr);
    Q_INVOKABLE void queueDraw(QGtkWindow *win);
};

QT_END_NAMESPACE

#endif // QGTKWINDOW_H
