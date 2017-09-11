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

#include "qgtkwindow.h"

#include <qpa/qwindowsysteminterface.h>

#include <QDebug>

// GL
gboolean render_cb(GtkGLArea *, GdkGLContext *ctx, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    pw->onRender();
    return TRUE;
}

// raster
void draw_cb(GtkWidget *, cairo_t *cr, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    pw->onDraw(cr);
}

gboolean map_cb(GtkWidget *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    pw->onMap();
    return FALSE;
}

gboolean unmap_cb(GtkWidget *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    pw->onUnmap();
    return FALSE;
}

gboolean configure_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    pw->onConfigure(event);
    return FALSE;
}

gboolean delete_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    return pw->onDelete() ? TRUE : FALSE;
}

gboolean key_press_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    return pw->onKeyPress(event) ? TRUE : FALSE;
}

gboolean key_release_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    return pw->onKeyRelease(event) ? TRUE : FALSE;
}

gboolean button_press_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    return pw->onButtonPress(event) ? TRUE : FALSE;
}

gboolean button_release_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    return pw->onButtonRelease(event) ? TRUE : FALSE;
}

gboolean touch_event_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    return pw->onTouchEvent(event) ? TRUE : FALSE;
}

gboolean motion_notify_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    return pw->onMotionNotify(event) ? TRUE : FALSE;
}

gboolean scroll_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    return pw->onScrollEvent(event) ? TRUE : FALSE;
}

// window
//  -> vbox
//      -> menubar
//      -> drawingarea
QGtkWindow::QGtkWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_buttons(Qt::NoButton)
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(m_window, "map", G_CALLBACK(map_cb), this);
    g_signal_connect(m_window, "unmap", G_CALLBACK(unmap_cb), this);
    g_signal_connect(m_window, "configure-event", G_CALLBACK(configure_cb), this);
    g_signal_connect(m_window, "delete-event", G_CALLBACK(delete_cb), this);
    g_signal_connect(m_window, "key-press-event", G_CALLBACK(key_press_cb), this);
    g_signal_connect(m_window, "key-release-event", G_CALLBACK(key_release_cb), this);
    g_signal_connect(m_window, "scroll-event", G_CALLBACK(scroll_cb), this);

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(m_window), vbox);

    m_menubar = GTK_MENU_BAR(gtk_menu_bar_new());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(m_menubar), FALSE, FALSE, 0);

    if (window->supportsOpenGL()) {
        m_content = gtk_gl_area_new();
        g_signal_connect(m_content, "render", G_CALLBACK(render_cb), this);
    } else {
        m_content = gtk_drawing_area_new();
        g_signal_connect(m_content, "draw", G_CALLBACK(draw_cb), this);
    }

    gtk_box_pack_end(GTK_BOX(vbox), m_content, TRUE, TRUE, 0);

    // ### Proximity? Touchpad gesture? Tablet?
    gtk_widget_set_events(m_content,
        GDK_POINTER_MOTION_MASK |
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK |
        GDK_SCROLL_MASK |
        GDK_SMOOTH_SCROLL_MASK |
        GDK_TOUCH_MASK
    );

    // Register event handlers that need coordinates on the content widget, not
    // the window.
    g_signal_connect(m_content, "button-press-event", G_CALLBACK(button_press_cb), this);
    g_signal_connect(m_content, "button-release-event", G_CALLBACK(button_release_cb), this);
    g_signal_connect(m_content, "touch-event", G_CALLBACK(touch_event_cb), this);
    g_signal_connect(m_content, "motion-notify-event", G_CALLBACK(motion_notify_cb), this);

    if (window->supportsOpenGL()) {
        // this has to wait until everything is set up.
        gtk_widget_realize(m_content);
        m_gl_context = gtk_gl_area_get_context(GTK_GL_AREA(m_content));
    }

    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen); // ### use GdkDevice or not?
    m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::MouseEmulation);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);
}

QGtkWindow::~QGtkWindow()
{
    // ### destroy the window?

    QWindowSystemInterface::unregisterTouchDevice(m_touchDevice);
}

void QGtkWindow::onDraw(cairo_t *cr)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation(m_content, &alloc);

    GtkStyleContext *ctx = gtk_widget_get_style_context(m_content);
    gtk_render_background(ctx, cr, 0, 0, alloc.width, alloc.height);

    GdkRGBA color;
    gtk_style_context_get_color(ctx, GTK_STATE_FLAG_NORMAL, &color);

    gdk_cairo_set_source_rgba(cr, &color);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, alloc.width, alloc.height);
    cairo_stroke(cr);

    cairo_surface_t *surf = cairo_image_surface_create_for_data(
            const_cast<uchar*>(m_image.constBits()),
            CAIRO_FORMAT_ARGB32,
            m_image.width(),
            m_image.height(),
            m_image.bytesPerLine()
    );
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(surf);
}

#include <QtGui/QOpenGLExtraFunctions>

void QGtkWindow::onRender()
{
    QOpenGLExtraFunctions funcs;
    funcs.initializeOpenGLFunctions();

    // inside this function it's safe to use GL; the given
    // #GdkGLContext has been made current to the drawable
    // surface used by the #GtkGLArea and the viewport has
    // already been set to be the size of the allocation

    GLint fboId;
    funcs.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboId);

    funcs.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboId);
    funcs.glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo->handle());

    funcs.glBlitFramebuffer(0, 0, 400, 400, 0, 0, 400, 400, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // we completed our drawing; the draw commands will be
    // flushed at the end of the signal emission chain, and
    // the buffers will be drawn on the window
}

void QGtkWindow::onMap() 
{
    qDebug() << "map" << this;
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
}

void QGtkWindow::onUnmap()
{
    qDebug() << "unmap" << this;
    QWindowSystemInterface::handleExposeEvent(window(), QRegion());
}

void QGtkWindow::onConfigure(GdkEvent *event)
{
    GdkEventConfigure *ev = (GdkEventConfigure*)event;
    QRect geom(ev->x, ev->y, ev->width, ev->height);
    qDebug() << "Configure: " << geom;
    QWindowSystemInterface::handleGeometryChange(window(), geom);
}

bool QGtkWindow::onDelete()
{
    bool accepted = false;
    QWindowSystemInterface::handleCloseEvent(window(), &accepted);
    QWindowSystemInterface::flushWindowSystemEvents();
    return accepted;
}

QSurfaceFormat QGtkWindow::format() const
{
    return window()->requestedFormat();
}

void QGtkWindow::setGeometry(const QRect &rect)
{
    qDebug() << "setGeometry" << rect;
    gtk_window_resize(GTK_WINDOW(m_window), rect.width(), rect.height());
    qDebug() << "Done geometry";
}

QRect QGtkWindow::geometry() const
{
    int width;
    int height;
    gtk_window_get_size(GTK_WINDOW(m_window), &width, &height);
    return QRect(0, 0, width, height);
}

QRect QGtkWindow::normalGeometry() const
{
    return geometry();
}

qreal QGtkWindow::devicePixelRatio() const
{
    // ### may change on configure event
    return gtk_widget_get_scale_factor(m_window);
}

QMargins QGtkWindow::frameMargins() const{}

void QGtkWindow::setVisible(bool visible)
{
    if (visible) {
        qDebug() << "Showing" << m_window;
        gtk_widget_show_all(m_window);
        qDebug() << "Showed" << m_window;
    } else {
        qDebug() << "Showing" << m_window;
        gtk_widget_hide(m_window);
        qDebug() << "Showed" << m_window;
    }
}

void QGtkWindow::setWindowFlags(Qt::WindowFlags flags)
{
    qWarning() << "setWindowFlags: Not implemented";
}

void QGtkWindow::setWindowState(Qt::WindowState state)
{
    qWarning() << "setWindowState: Not implemented";
}

WId QGtkWindow::winId() const
{
    return (WId)m_window;
}

void QGtkWindow::setParent(const QPlatformWindow *window)
{
    qWarning() << "setParent: Not implemented";
}

void QGtkWindow::setWindowTitle(const QString &title)
{
    gtk_window_set_title(GTK_WINDOW(m_window), title.toUtf8().constData());
}

void QGtkWindow::setWindowFilePath(const QString &title)
{
    // WTF is this?
    qWarning() << "setWindowFilePath: Not implemented";
}

void QGtkWindow::setWindowIcon(const QIcon &icon)
{
    // icon -> GdkPixbuf, and send gtk_window_set_icon_list
    qWarning() << "setWindowIcon: Not implemented";
}

void QGtkWindow::raise()
{
    gtk_window_present(GTK_WINDOW(m_window));
}

void QGtkWindow::lower()
{
    // ### not sure we can do this
    qWarning() << "lower: Not implemented";
}

bool QGtkWindow::isExposed() const
{
    return gtk_widget_get_visible(m_window);
}

bool QGtkWindow::isActive() const
{
    //qWarning() << "isActive: Not implemented";
    return true;
}

/*
void QGtkWindow::propagateSizeHints(){}
*/
void QGtkWindow::setOpacity(qreal level)
{
    gtk_widget_set_opacity(m_window, level);
}

/*
void QGtkWindow::setMask(const QRegion &region){}
void QGtkWindow::requestActivateWindow(){}

void QGtkWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation){}

qreal QGtkWindow::devicePixelRatio() const{}

bool QGtkWindow::setKeyboardGrabEnabled(bool grab){}
bool QGtkWindow::setMouseGrabEnabled(bool grab){}

bool QGtkWindow::setWindowModified(bool modified){}

void QGtkWindow::windowEvent(QEvent *event){}

bool QGtkWindow::startSystemResize(const QPoint &pos, Qt::Corner corner){}

void QGtkWindow::setFrameStrutEventsEnabled(bool enabled){}
bool QGtkWindow::frameStrutEventsEnabled() const{}

void QGtkWindow::setAlertState(bool enabled){}
bool QGtkWindow::isAlertState() const{}

void QGtkWindow::invalidateSurface(){}
void QGtkWindow::requestUpdate()
{
    QPlatformWindow::requestUpdate();
    // ###
    // If you are displaying animated content and want to continually request
    // the GDK_FRAME_CLOCK_PHASE_UPDATE phase for a period of time, you should
    // use gdk_frame_clock_begin_updating() instead, since this allows GTK+ to
    // adjust system parameters to get maximally smooth animations.
    GdkFrameClock *clock = gdk_window_get_frame_clock(m_window);
    gdk_frame_clock_request_phase(clock, GDK_FRAME_CLOCK_PHASE_UPDATE);
}
*/

void QGtkWindow::setWindowContents(const QImage &image, const QRegion &region, const QPoint &offset)
{
    m_image = image;

#if 0
    // ### could use queue_draw_region instead, tho might not be worth it
    QRect br = region.boundingRect();

    int x = offset.x();
    int y = offset.y();
    int dx = br.width();
    int dy = br.height();
    qDebug() << "Before converting: " << x << y << dx << dy;

    // ### this is wrong somehow, maybe need a gtk_widget_translate_coordinates?
    gtk_widget_queue_draw_area(m_content, x, y, dx, dy);
#endif
    gtk_widget_queue_draw(m_content);
}

GtkWidget *QGtkWindow::gtkWindow() const
{
    return m_window;
}

GtkMenuBar *QGtkWindow::gtkMenuBar() const
{
    return m_menubar;
}

GdkGLContext *QGtkWindow::gdkGLContext() const
{
    return m_gl_context;
}
