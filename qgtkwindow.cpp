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
#include "qgtkhelpers.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/private/qwindow_p.h>

#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcWindow, "qt.qpa.gtk.window");
Q_LOGGING_CATEGORY(lcWindowRender, "qt.qpa.gtk.window.render");

void draw_cb(GtkWidget *, cairo_t *cr, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "draw_cb" << pw;
    pw->onDraw(cr);
}

gboolean map_cb(GtkWidget *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "map_cb" << pw;
    pw->onMap();
    return FALSE;
}

gboolean unmap_cb(GtkWidget *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "unmap_cb" << pw;
    pw->onUnmap();
    return FALSE;
}

gboolean configure_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "configure_cb" << pw;
    pw->onConfigure(event);
    return FALSE;
}

gboolean delete_cb(GtkWidget *, GdkEvent *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "delete_cb" << pw;
    return pw->onDelete() ? TRUE : FALSE;
}

gboolean key_press_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "key_press_cb" << pw;
    return pw->onKeyPress(event) ? TRUE : FALSE;
}

gboolean key_release_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "key_release_cb" << pw;
    return pw->onKeyRelease(event) ? TRUE : FALSE;
}

gboolean button_press_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "button_press_cb" << pw;
    return pw->onButtonPress(event) ? TRUE : FALSE;
}

gboolean button_release_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "button_release_cb" << pw;
    return pw->onButtonRelease(event) ? TRUE : FALSE;
}

gboolean touch_event_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "touch_event_cb" << pw;
    return pw->onTouchEvent(event) ? TRUE : FALSE;
}

gboolean motion_notify_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "motion_notify_cb" << pw;
    return pw->onMotionNotify(event) ? TRUE : FALSE;
}

gboolean scroll_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "scroll_cb" << pw;
    return pw->onScrollEvent(event) ? TRUE : FALSE;
}

gboolean window_state_event_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "window_state_event_cb" << pw;
    pw->onWindowStateEvent(event);
    return FALSE;
}

gboolean window_tick_cb(GtkWidget*, GdkFrameClock *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "window_tick_cb" << pw;
    pw->onUpdateFrameClock();
    return G_SOURCE_CONTINUE;
}

void QGtkWindow::onUpdateFrameClock()
{
    if (m_wantsUpdate) {
        m_wantsUpdate = false;
        QWindowPrivate::get(window())->deliverUpdateRequest();
    }
}

// window
//  -> vbox
//      -> menubar
//      -> drawingarea
QGtkWindow::QGtkWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_buttons(Qt::NoButton)
{
    create(Qt::Window);
}

void QGtkWindow::create(Qt::WindowType windowType)
{
    if (m_window) {
        gtk_widget_destroy(m_window.get());
    }

    GtkWindowType gtkWindowType = GTK_WINDOW_TOPLEVEL;
    if (windowType == Qt::ToolTip) {
        gtkWindowType = GTK_WINDOW_POPUP;
    }

    m_window = gtk_window_new(gtkWindowType);
    g_signal_connect(m_window.get(), "map", G_CALLBACK(map_cb), this);
    g_signal_connect(m_window.get(), "unmap", G_CALLBACK(unmap_cb), this);
    g_signal_connect(m_window.get(), "configure-event", G_CALLBACK(configure_cb), this);
    g_signal_connect(m_window.get(), "delete-event", G_CALLBACK(delete_cb), this);
    g_signal_connect(m_window.get(), "key-press-event", G_CALLBACK(key_press_cb), this);
    g_signal_connect(m_window.get(), "key-release-event", G_CALLBACK(key_release_cb), this);
    g_signal_connect(m_window.get(), "scroll-event", G_CALLBACK(scroll_cb), this);
    m_tick_callback = gtk_widget_add_tick_callback(m_window.get(), window_tick_cb, this, NULL);
    gtk_window_resize(GTK_WINDOW(m_window.get()), window()->geometry().width(), window()->geometry().height());

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(m_window.get()), vbox);

    m_menubar = GTK_MENU_BAR(gtk_menu_bar_new());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(m_menubar.get()), FALSE, FALSE, 0);

    m_content = gtk_drawing_area_new();
    g_signal_connect(m_content.get(), "draw", G_CALLBACK(draw_cb), this);

    gtk_box_pack_end(GTK_BOX(vbox), m_content.get(), TRUE, TRUE, 0);

    // ### Proximity? Touchpad gesture? Tablet?
    gtk_widget_set_events(m_content.get(),
        GDK_POINTER_MOTION_MASK |
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK |
        GDK_SCROLL_MASK |
        GDK_SMOOTH_SCROLL_MASK |
        GDK_TOUCH_MASK
    );

    // Register event handlers that need coordinates on the content widget, not
    // the window.
    g_signal_connect(m_content.get(), "button-press-event", G_CALLBACK(button_press_cb), this);
    g_signal_connect(m_content.get(), "button-release-event", G_CALLBACK(button_release_cb), this);
    g_signal_connect(m_content.get(), "touch-event", G_CALLBACK(touch_event_cb), this);
    g_signal_connect(m_content.get(), "motion-notify-event", G_CALLBACK(motion_notify_cb), this);

    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen); // ### use GdkDevice or not?
    m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::MouseEmulation);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);

    setWindowState(window()->windowState());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());

    if (!qFuzzyCompare(QWindowPrivate::get(window())->opacity, qreal(1.0))) {
        setOpacity(QWindowPrivate::get(window())->opacity);
    }
    if (window()->isTopLevel()) {
        setWindowIcon(window()->icon());
    }
}

QGtkWindow::~QGtkWindow()
{
    gtk_widget_remove_tick_callback(m_window.get(), m_tick_callback);
    QWindowSystemInterface::unregisterTouchDevice(m_touchDevice);
    gtk_widget_destroy(m_window.get());
}

void QGtkWindow::onDraw(cairo_t *cr)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation(m_content.get(), &alloc);

    GtkStyleContext *ctx = gtk_widget_get_style_context(m_content.get());
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
    // ### highdpi
    //int sf = gtk_widget_get_scale_factor(m_window.get());
    //cairo_surface_set_device_scale(surf, sf, sf);
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(surf);
}

void QGtkWindow::onMap()
{
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
}

void QGtkWindow::onUnmap()
{
    QWindowSystemInterface::handleExposeEvent(window(), QRegion());
}

void QGtkWindow::onConfigure(GdkEvent *event)
{
    GdkEventConfigure *ev = (GdkEventConfigure*)event;
    QRect geom(ev->x, ev->y, ev->width, ev->height);
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
    gtk_window_resize(GTK_WINDOW(m_window.get()), rect.width(), rect.height());
}

QRect QGtkWindow::geometry() const
{
    int width;
    int height;
    gtk_window_get_size(GTK_WINDOW(m_window.get()), &width, &height);
    return QRect(0, 0, width, height);
}

QRect QGtkWindow::normalGeometry() const
{
    return geometry();
}

qreal QGtkWindow::devicePixelRatio() const
{
    // ### may change on configure event
    // ### highdpi
    return 1.0;
    //return gtk_widget_get_scale_factor(m_window.get());
}

QMargins QGtkWindow::frameMargins() const
{
    GdkWindow *gwindow = gtk_widget_get_window(m_window.get());
    if (!gwindow) {
        return QMargins();
    }
    GdkRectangle frame_rect;
    gdk_window_get_frame_extents(gtk_widget_get_window(m_window.get()), &frame_rect);
    GdkRectangle alloc_rect;
    gtk_widget_get_allocation(m_window.get(), &alloc_rect);
    return QMargins(
        alloc_rect.x,
        alloc_rect.y,
        frame_rect.width - alloc_rect.width - alloc_rect.x,
        frame_rect.height - alloc_rect.height - alloc_rect.y
    );
}

void QGtkWindow::setVisible(bool visible)
{
    if (visible) {
        gtk_widget_show_all(m_window.get());
    } else {
        gtk_widget_hide(m_window.get());
    }
}

void QGtkWindow::setWindowFlags(Qt::WindowFlags flags)
{
    // ### recreate the window if the type changes, but be careful, we may
    // recurse.
    gtk_window_set_decorated(GTK_WINDOW(m_window.get()), !(flags & Qt::FramelessWindowHint));
    gtk_window_set_deletable(GTK_WINDOW(m_window.get()), flags & Qt::WindowCloseButtonHint);
}

void QGtkWindow::setWindowState(Qt::WindowState state)
{
    if (state == m_state) {
        return;
    }

    switch (m_state) {
    case Qt::WindowMinimized:
        gtk_window_deiconify(GTK_WINDOW(m_window.get()));
        break;
    case Qt::WindowMaximized:
        gtk_window_unmaximize(GTK_WINDOW(m_window.get()));
        break;
    case Qt::WindowFullScreen:
        gtk_window_unfullscreen(GTK_WINDOW(m_window.get()));
        break;
    case Qt::WindowNoState:
    case Qt::WindowActive:
        break;
    }

    switch (state) {
    case Qt::WindowMinimized:
        gtk_window_iconify(GTK_WINDOW(m_window.get()));
        break;
    case Qt::WindowMaximized:
        gtk_window_maximize(GTK_WINDOW(m_window.get()));
        break;
    case Qt::WindowFullScreen:
        gtk_window_fullscreen(GTK_WINDOW(m_window.get()));
        break;
    case Qt::WindowNoState:
    case Qt::WindowActive:
        break;
    }

    m_state = state;
}

void QGtkWindow::onWindowStateEvent(GdkEvent *event)
{
    GdkEventWindowState *ev = (GdkEventWindowState*)event;
    Qt::WindowState newState = Qt::WindowNoState;

    if (ev->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
        newState = Qt::WindowMinimized;
    }
    if (ev->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        newState = Qt::WindowMaximized;
    }
    if (ev->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
        newState = Qt::WindowFullScreen;
    }

    if (newState != m_state) {
        m_state = newState;
        QWindowSystemInterface::handleWindowStateChanged(window(), newState);
    }

    if (ev->changed_mask & GDK_WINDOW_STATE_FOCUSED) {
        if (ev->new_window_state & GDK_WINDOW_STATE_FOCUSED) {
            QWindowSystemInterface::handleWindowActivated(window(), Qt::ActiveWindowFocusReason);
        } else {
            // ### it would be nicer if we could specify where the focus went
            // to, rather than setting it to nullptr. this may be a race in a
            // multiwindow application, too, depending on how event delivery is
            // ordered...
            QWindowSystemInterface::handleWindowActivated(nullptr, Qt::ActiveWindowFocusReason);
        }
    }

    // GDK_WINDOW_STATE_TILED not handled.
    // GDK_WINDOW_STATE_STICKY not handled.
    // GDK_WINDOW_STATE_ABOVE not handled.
    // GDK_WINDOW_STATE_BELOW not handled.
    // GDK_WINDOW_STATE_WITHDRAWN not handled.
}

WId QGtkWindow::winId() const
{
    return (WId)m_window.get();
}

void QGtkWindow::setParent(const QPlatformWindow *window)
{
    if (!window) {
        gtk_window_set_transient_for(GTK_WINDOW(m_window.get()), nullptr);
    } else {
        gtk_window_set_transient_for(GTK_WINDOW(m_window.get()), GTK_WINDOW(static_cast<const QGtkWindow*>(window)->gtkWindow().get()));
    }
}

void QGtkWindow::setWindowTitle(const QString &title)
{
    gtk_window_set_title(GTK_WINDOW(m_window.get()), title.toUtf8().constData());
}

void QGtkWindow::setWindowFilePath(const QString &title)
{
    // WTF is this?
    qWarning() << "setWindowFilePath: Not implemented: " << title;
}

void QGtkWindow::setWindowIcon(const QIcon &icon)
{
    if (icon.isNull()) {
        gtk_window_set_icon(GTK_WINDOW(m_window.get()), nullptr);
        return;
    }

    GdkPixbuf *pb = qt_iconToPixbuf(icon);

    // ### consider gtk_window_set_icon_list
    gtk_window_set_icon(GTK_WINDOW(m_window.get()), pb);
}

void QGtkWindow::raise()
{
    gtk_window_present(GTK_WINDOW(m_window.get()));
}

void QGtkWindow::lower()
{
    // ### not sure we can do this
    qWarning() << "lower: Not implemented";
}

bool QGtkWindow::isExposed() const
{
    return gtk_widget_get_visible(m_window.get());
}

bool QGtkWindow::isActive() const
{
    return gtk_widget_has_focus(m_window.get());
}

void QGtkWindow::propagateSizeHints()
{
    QSize minSize = windowMinimumSize();
    QSize maxSize = windowMaximumSize();
    QSize baseSize = windowBaseSize();
    QSize sizeIncrement = windowSizeIncrement();

    GdkGeometry hints;
    hints.min_width = minSize.width();
    hints.min_height = minSize.height();
    hints.max_width = maxSize.width();
    hints.max_height = maxSize.height();
    hints.base_width = baseSize.width();
    hints.base_height = baseSize.height();
    hints.width_inc = sizeIncrement.width();
    hints.height_inc = sizeIncrement.height();

    gtk_window_set_geometry_hints(
        GTK_WINDOW(m_window.get()),
        m_window.get(),
        &hints,
        GdkWindowHints(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_BASE_SIZE | GDK_HINT_RESIZE_INC)
    );
}

void QGtkWindow::setOpacity(qreal level)
{
    gtk_widget_set_opacity(m_window.get(), level);
}

/*
void QGtkWindow::setMask(const QRegion &region){}
void QGtkWindow::requestActivateWindow(){}

void QGtkWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation){}
*/
bool QGtkWindow::setKeyboardGrabEnabled(bool grab)
{
    if (grab) {
        gtk_window_present(GTK_WINDOW(m_window.get()));
    }
    return true;
}
bool QGtkWindow::setMouseGrabEnabled(bool grab)
{
    if (grab) {
        gtk_window_present(GTK_WINDOW(m_window.get()));
    }
    return true;
}
/*
bool QGtkWindow::setWindowModified(bool modified){}

void QGtkWindow::windowEvent(QEvent *event){}

bool QGtkWindow::startSystemResize(const QPoint &pos, Qt::Corner corner){}

void QGtkWindow::setFrameStrutEventsEnabled(bool enabled){}
bool QGtkWindow::frameStrutEventsEnabled() const{}
*/

void QGtkWindow::setAlertState(bool enabled)
{
    gtk_window_set_urgency_hint(GTK_WINDOW(m_window.get()), enabled);
}

bool QGtkWindow::isAlertState() const
{
    return gtk_window_get_urgency_hint(GTK_WINDOW(m_window.get()));
}

/*
void QGtkWindow::invalidateSurface(){}
*/
void QGtkWindow::requestUpdate()
{
    m_wantsUpdate = true;
    gtk_widget_queue_draw(m_content.get());
}

void QGtkWindow::setWindowContents(const QImage &image, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);
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
    gtk_widget_queue_draw_area(m_content.get(), x, y, dx, dy);
#endif
    gtk_widget_queue_draw(m_content.get());
}

QGtkRefPtr<GtkWidget> QGtkWindow::gtkWindow() const
{
    return m_window;
}

QGtkRefPtr<GtkMenuBar> QGtkWindow::gtkMenuBar() const
{
    return m_menubar;
}
