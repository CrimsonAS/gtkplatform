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

#include "qgtkwindow.h"
#include "qgtkhelpers.h"

#include <QtGui/qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/private/qwindow_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcWindow, "qt.qpa.gtk.window");
Q_LOGGING_CATEGORY(lcWindowEvents, "qt.qpa.gtk.window");

static gboolean map_cb(GtkWidget *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "map_cb" << pw;
    pw->onMap();
    return FALSE;
}

static gboolean unmap_cb(GtkWidget *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "unmap_cb" << pw;
    pw->onUnmap();
    return FALSE;
}

static gboolean configure_cb(GtkWidget *, GdkEvent *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "configure_cb" << pw;
    pw->onConfigure();
    return FALSE;
}

static gboolean size_allocate_cb(GtkWidget *, GdkRectangle *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "size_allocate_cb" << pw;
    pw->onConfigure();
    return FALSE;
}

static gboolean delete_cb(GtkWidget *, GdkEvent *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "delete_cb" << pw;
    return pw->onDelete() ? TRUE : FALSE;
}

static gboolean key_press_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "key_press_cb" << pw;
    return pw->onKeyPress(event) ? TRUE : FALSE;
}

static gboolean key_release_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "key_release_cb" << pw;
    return pw->onKeyRelease(event) ? TRUE : FALSE;
}

static gboolean button_press_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "button_press_cb" << pw;
    return pw->onButtonPress(event) ? TRUE : FALSE;
}

static gboolean button_release_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "button_release_cb" << pw;
    return pw->onButtonRelease(event) ? TRUE : FALSE;
}

static gboolean touch_event_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "touch_event_cb" << pw;
    return pw->onTouchEvent(event) ? TRUE : FALSE;
}

static gboolean motion_notify_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "motion_notify_cb" << pw;
    return pw->onMotionNotify(event) ? TRUE : FALSE;
}

static gboolean scroll_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "scroll_cb" << pw;
    return pw->onScrollEvent(event) ? TRUE : FALSE;
}

static gboolean window_state_event_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "window_state_event_cb" << pw;
    pw->onWindowStateEvent(event);
    return FALSE;
}

static gboolean enter_leave_window_notify_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    bool entering = event->type == GDK_ENTER_NOTIFY;
    qCDebug(lcWindowEvents) << "enter_leave_window_notify_cb" << pw << entering;
    pw->onEnterLeaveWindow(event, entering);
    return false;
}

static gboolean leave_content_notify_cb(GtkWidget *, GdkEvent *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowEvents) << "leave_content_notify_cb" << pw;
    pw->onLeaveContent();
    return false;
}

QGtkWindow::QGtkWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_buttons(Qt::NoButton)
    , m_windowGeometry(0, 0, 1, 1)
{
    create(window->type());

    if (!QGtkCourierObject::instance)
        QGtkCourierObject::instance = new QGtkCourierObject(QCoreApplication::instance());
}

void QGtkWindow::create(Qt::WindowType windowType)
{
    if (m_window) {
        gtk_widget_destroy(m_window.get());
    }

    // Determine the window type. GTK_WINDOW_TOPLEVEL is usually right.
    GtkWindowType gtkWindowType = GTK_WINDOW_TOPLEVEL;
    if (windowType == Qt::ToolTip ||
        windowType == Qt::Popup) {
        gtkWindowType = GTK_WINDOW_POPUP;
    }

    // Create the window.
    m_window = gtk_window_new(gtkWindowType);

    // First things first, set a proper type hint on the window.
    switch (windowType) {
    case Qt::Window:
        gtk_window_set_type_hint(GTK_WINDOW(m_window.get()), GDK_WINDOW_TYPE_HINT_NORMAL);
        break;
    case Qt::Dialog:
    case Qt::Sheet:
        gtk_window_set_type_hint(GTK_WINDOW(m_window.get()), GDK_WINDOW_TYPE_HINT_DIALOG);
        break;
    case Qt::Popup:
        gtk_window_set_type_hint(GTK_WINDOW(m_window.get()), GDK_WINDOW_TYPE_HINT_MENU);
        break;
    case Qt::Tool:
        gtk_window_set_type_hint(GTK_WINDOW(m_window.get()), GDK_WINDOW_TYPE_HINT_TOOLBAR);
        break;
    case Qt::SplashScreen:
        gtk_window_set_type_hint(GTK_WINDOW(m_window.get()), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
        break;
    case Qt::ToolTip:
        gtk_window_set_type_hint(GTK_WINDOW(m_window.get()), GDK_WINDOW_TYPE_HINT_TOOLTIP);
        break;
    default:
        break;
    }

    // Now set a transient parent (for things that ought to have one). This is
    // required otherwise things like positioning windows will not work, as
    // Wayland doesn't have any concept of a global window position.
    maybeForceTransientParent(windowType);

    g_signal_connect(m_window.get(), "map", G_CALLBACK(map_cb), this);
    g_signal_connect(m_window.get(), "unmap", G_CALLBACK(unmap_cb), this);
    g_signal_connect(m_window.get(), "configure-event", G_CALLBACK(configure_cb), this);
    g_signal_connect(m_window.get(), "enter-notify-event", G_CALLBACK(enter_leave_window_notify_cb), this);
    g_signal_connect(m_window.get(), "leave-notify-event", G_CALLBACK(enter_leave_window_notify_cb), this);

    // for whatever reason, configure-event is not enough. it doesn't seem to
    // get emitted for popup type windows. so also connect to size-allocate just
    // to be sure...
    g_signal_connect(m_window.get(), "size-allocate", G_CALLBACK(size_allocate_cb), this);
    g_signal_connect(m_window.get(), "delete-event", G_CALLBACK(delete_cb), this);
    g_signal_connect(m_window.get(), "window-state-event", G_CALLBACK(window_state_event_cb), this);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(m_window.get()), vbox);

    m_menubar = GTK_MENU_BAR(gtk_menu_bar_new());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(m_menubar.get()), FALSE, FALSE, 0);

    m_content = gtk_drawing_area_new();
    g_signal_connect(m_content.get(), "draw", G_CALLBACK(QGtkWindow::drawCallback), this);

    gtk_box_pack_end(GTK_BOX(vbox), m_content.get(), TRUE, TRUE, 0);

    // ### Proximity? Touchpad gesture? Tablet?
    gtk_widget_set_events(m_content.get(),
        GDK_POINTER_MOTION_MASK |
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK |
        GDK_SCROLL_MASK |
        GDK_SMOOTH_SCROLL_MASK |
        GDK_TOUCH_MASK |
        GDK_LEAVE_NOTIFY_MASK
    );

    // Register event handlers that need coordinates on the content widget, not
    // the window.
    g_signal_connect(m_content.get(), "button-press-event", G_CALLBACK(button_press_cb), this);
    g_signal_connect(m_content.get(), "button-release-event", G_CALLBACK(button_release_cb), this);
    g_signal_connect(m_content.get(), "touch-event", G_CALLBACK(touch_event_cb), this);
    g_signal_connect(m_content.get(), "motion-notify-event", G_CALLBACK(motion_notify_cb), this);
    g_signal_connect(m_content.get(), "key-press-event", G_CALLBACK(key_press_cb), this);
    g_signal_connect(m_content.get(), "key-release-event", G_CALLBACK(key_release_cb), this);
    g_signal_connect(m_content.get(), "scroll-event", G_CALLBACK(scroll_cb), this);
    g_signal_connect(m_content.get(), "leave-notify-event", G_CALLBACK(leave_content_notify_cb), this);
    gtk_widget_set_can_focus(m_content.get(), true);

    m_zoomGesture = gtk_gesture_zoom_new(m_content.get());
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(m_zoomGesture.get()), GTK_PHASE_CAPTURE);
    g_signal_connect(m_zoomGesture.get(), "scale-changed", G_CALLBACK(QGtkWindow::zoom_cb), this);
    g_signal_connect(m_zoomGesture.get(), "begin", G_CALLBACK(QGtkWindow::begin_zoom_cb), this);
    g_signal_connect(m_zoomGesture.get(), "cancel", G_CALLBACK(QGtkWindow::cancel_zoom_cb), this);
    g_signal_connect(m_zoomGesture.get(), "end", G_CALLBACK(QGtkWindow::end_zoom_cb), this);

    m_rotateGesture = gtk_gesture_rotate_new(m_content.get());
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(m_rotateGesture.get()), GTK_PHASE_CAPTURE);
    g_signal_connect(m_rotateGesture.get(), "angle-changed", G_CALLBACK(QGtkWindow::rotate_cb), this);
    g_signal_connect(m_rotateGesture.get(), "begin", G_CALLBACK(QGtkWindow::begin_rotate_cb), this);
    g_signal_connect(m_rotateGesture.get(), "cancel", G_CALLBACK(QGtkWindow::cancel_rotate_cb), this);
    g_signal_connect(m_rotateGesture.get(), "end", G_CALLBACK(QGtkWindow::end_rotate_cb), this);

    gtk_gesture_group(m_zoomGesture.get(), m_rotateGesture.get());

    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen); // ### use GdkDevice or not?
    m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::MouseEmulation);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);

    setWindowState(window()->windowState());
    propagateSizeHints();
    setWindowFlags(window()->flags());
    setGeometry(window()->geometry());
    gtk_window_set_modal(GTK_WINDOW(m_window.get()), window()->modality() != Qt::NonModal);
    if (!window()->title().isEmpty())
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

void QGtkWindow::maybeForceTransientParent(Qt::WindowType windowType)
{
    bool shouldTransient = window()->modality() != Qt::NonModal;

    switch (windowType) {
    case Qt::Dialog:
    case Qt::Sheet:
    case Qt::Tool:
    case Qt::SplashScreen:
    case Qt::ToolTip:
    case Qt::Drawer:
    case Qt::Popup:
        shouldTransient = true;
        break;
    default:
        break;
    }

    if (!shouldTransient) {
        return;
    }

    // Hope they specified one first.
    QWindow *transientParent = window()->transientParent();
    if (transientParent) {
        reallyForceTransientFor(transientParent);
        return;
    }

    // Try fall back to focus. We must have a top level window, though.
    if (qApp->focusWindow() && qApp->focusWindow()->type() == Qt::Window) {
        qWarning() << "Forcing transient parent to focus window " << qApp->focusWindow() << " for window " << window() << " -- this is bad, it ought to have a transientParent set, the window may end up incorrectly positioned";
        reallyForceTransientFor(qApp->focusWindow());
        return;
    }

    // Last ditch effort: try find a top level window.
    QWindowList wl = qApp->topLevelWindows();
    for (QWindow *win : wl) {
        if (win->type() == Qt::Window) {
            qWarning() << "Forcing transient parent to first available toplevel " << win << " for window " << window() << " -- this is bad, it ought to have a transientParent set, the window may end up incorrectly positioned.";
            reallyForceTransientFor(win);
            return;
        }
    }

    qWarning() << "Showing " << window() << " as a transient window without a transient parent, positioning will almost certainly be incorrect (if it works at all!)";
}

void QGtkWindow::reallyForceTransientFor(QWindow *transientParent)
{
    transientParent->create(); // force pwin creation
    QGtkWindow *transientParentPlatform = static_cast<QGtkWindow*>(transientParent->handle());
    gtk_window_set_transient_for(GTK_WINDOW(m_window.get()), GTK_WINDOW(transientParentPlatform->gtkWindow().get()));
}

void QGtkWindow::onMap()
{
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
}

void QGtkWindow::onUnmap()
{
    QWindowSystemInterface::handleExposeEvent(window(), QRegion());
}

void QGtkWindow::onConfigure()
{
    // windowX and windowY are the window system coordinates of the top-left of the client
    // portion of the window. They don't include server-side decorations but do include client-side.
    int windowX = 0, windowY = 0;
    GdkWindow *gwindow = gtk_widget_get_window(m_window.get());
    if (gwindow)
        gdk_window_get_position(gwindow, &windowX, &windowY);

    // contentRect is the drawn content area of the window in window coordinates. This excludes
    // client-side decorations and other frame elements (e.g. menubar).
    GdkRectangle contentRect;
    gtk_widget_get_allocated_size(m_content.get(), &contentRect, nullptr);

    // QWindow geometry is contentRect translated to window system coordinates with windowX/windowY
    m_newGeometry = QRect(windowX + contentRect.x, windowY + contentRect.y,
                          contentRect.width, contentRect.height);
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

void QGtkWindow::setGeometry(const QRect &crect)
{
    QRect rect(crect);
    Qt::WindowType type = static_cast<Qt::WindowType>(int(m_flags & Qt::WindowType_Mask));
    if (type != Qt::Window) {
        // Ensure that child windows are positioned somewhere that makes sense.
        // If we don't do this, then popup-type windows will end up off-screen,
        // which isn't very useful.
        //
        // It'd be quite nice if in Qt 6, we could consider doing away with
        // absolute positioning of menus and such, and rather, tie them to a
        // pointer device or a parent widget + offset inside that widget, or
        // something.
        const QSize screenSize = window()->screen()->availableGeometry().size();
        int deltaX = rect.x() - rect.width();
        int deltaY = rect.y() - rect.height();
        if (rect.y() + rect.height() > screenSize.height()) {
            rect.moveTop(deltaY);
        }
        if (rect.x() + rect.width() > screenSize.width()) {
            rect.moveLeft(deltaX);
        }
    }

    if (!window()->isVisible()) {
        // if we aren't visible, we won't get a configure event, so cache the
        // geometry for the time being.
        m_windowGeometry = QRect(QPoint(0, 0), rect.size());
    }
    gtk_window_move(GTK_WINDOW(m_window.get()), rect.x(), rect.y());
    gtk_window_resize(GTK_WINDOW(m_window.get()), qMax(rect.width(), 1), qMax(rect.height(), 1));
}

QRect QGtkWindow::geometry() const
{
    return m_windowGeometry;
}

QRect QGtkWindow::normalGeometry() const
{
    return geometry();
}

qreal QGtkWindow::devicePixelRatio() const
{
    // ### may change on configure event
    return gtk_widget_get_scale_factor(m_window.get());
}

QMargins QGtkWindow::frameMargins() const
{
    GdkWindow *gwindow = gtk_widget_get_window(m_window.get());
    if (!gwindow) {
        return QMargins();
    }

    // Bounding rectangle of the entire window, including server-side frame
    // x and y are in root window coordinates
    GdkRectangle frameRect;
    gdk_window_get_frame_extents(gwindow, &frameRect);

    // Position of the top-left of the window area, excluding server-side frames,
    // also in root window coordinates
    int originX, originY;
    gdk_window_get_origin(gwindow, &originX, &originY);

    // Rectangle in window coordinates of the content area, excluding client-side frames
    GdkRectangle contentRect;
    gtk_widget_get_allocated_size(m_content.get(), &contentRect, nullptr);

    // Size of the margin for top and left. This is the server-side margin (difference
    // between the frame and origin's X) plus the client-side margin (contentRect.x)
    int leftMargin = originX - frameRect.x + contentRect.x;
    int topMargin = originY - frameRect.y + contentRect.y;

    // Bottom and right margins are the remainder of frameRect's size after removing the
    // top/left margins and contentRect size.
    return QMargins(leftMargin, topMargin,
                    frameRect.width - leftMargin - contentRect.width,
                    frameRect.height - topMargin - contentRect.height);
}

void QGtkWindow::setVisible(bool visible)
{
    if (visible) {
        gtk_widget_show_all(m_window.get());
        gtk_widget_grab_focus(m_content.get());
    } else {
        gtk_widget_hide(m_window.get());
    }
}

void QGtkWindow::setWindowFlags(Qt::WindowFlags flags)
{
    if (flags == m_flags) {
        // probably means we're being called from create(), do our best to make
        // it harmless.
        return;
    }

    Qt::WindowType oldType = static_cast<Qt::WindowType>(int(m_flags & Qt::WindowType_Mask));
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));
    m_flags = flags;

    if (type == Qt::Popup) {
        flags |= Qt::FramelessWindowHint;
    }

    if (type != oldType) {
        if (type == Qt::Popup) {
            // ### do we really support changing to other types of windows at
            // runtime? this will probably break all sorts of stuff.
            //create(Qt::Popup);
        }
    }

    // ### recreate the window if the type changes, but be careful, we may
    // recurse.
    gtk_window_set_decorated(GTK_WINDOW(m_window.get()), !(flags & Qt::FramelessWindowHint));

    if ((flags & Qt::CustomizeWindowHint)) {
        gtk_window_set_deletable(GTK_WINDOW(m_window.get()), (flags & Qt::WindowCloseButtonHint));
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
void QGtkWindow::setWindowState(Qt::WindowStates requestedState)
{
    const Qt::WindowState state = QWindowPrivate::effectiveState(requestedState);
#else
void QGtkWindow::setWindowState(Qt::WindowState requestedState)
{
    const Qt::WindowState state = requestedState;
#endif

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

    Qt::WindowType type = static_cast<Qt::WindowType>(int(m_flags & Qt::WindowType_Mask));

    // We must not send window activation changes for tooltip windows, as they
    // will auto-hide on activation change.
    if (type != Qt::ToolTip && (ev->changed_mask & GDK_WINDOW_STATE_FOCUSED)) {
        static QPointer<QWindow> newActiveWindow = nullptr;
        if (ev->new_window_state & GDK_WINDOW_STATE_FOCUSED) {
            qCDebug(lcWindow) << window() << " focused";
            newActiveWindow = window();
        } else if (newActiveWindow == window()) {
            qCDebug(lcWindow) << window() << " unfocused";
            newActiveWindow = nullptr;
        }

        // We need a timer here to debounce the focus changes. Reason being that
        // one window appearing results in two GDK_WINDOW_STATE_FOCUSED changes:
        // one for the old window to remove it, one for the new window to add
        // it.
        //
        // Without a debounce, we set the active window like this:
        // old
        // nullptr
        // new
        //
        // ... which, in the case of say, popups, may result in their being
        // dismissed (since a combo box shouldn't be kept open if its parent
        // window loses focus to something other than the combo).
        QTimer::singleShot(0, [=]() {
            qCDebug(lcWindow) << "Active changed to " << newActiveWindow.data();
            QWindowSystemInterface::handleWindowActivated(newActiveWindow.data(), Qt::ActiveWindowFocusReason);
        });
    }

    // GDK_WINDOW_STATE_TILED not handled.
    // GDK_WINDOW_STATE_STICKY not handled.
    // GDK_WINDOW_STATE_ABOVE not handled.
    // GDK_WINDOW_STATE_BELOW not handled.
    // GDK_WINDOW_STATE_WITHDRAWN not handled.
}

void QGtkWindow::onEnterLeaveWindow(GdkEvent *event, bool entered)
{
    GdkEventCrossing *ev = (GdkEventCrossing*)event;
    static QPointer<QWindow> enterWindow;
    static QPoint enterPos;
    static QPoint globalEnterPos;
    static QPointer<QWindow> leaveWindow;
    static QPoint leavePos;
    static QPoint globalLeavePos;

    if (entered) {
        enterWindow = window();
        enterPos = QPoint(ev->x, ev->y);
        globalEnterPos = QPoint(ev->x_root, ev->y_root);
    } else {
        leaveWindow = window();
        leavePos = QPoint(ev->x, ev->y);
        globalLeavePos = QPoint(ev->x_root, ev->y_root);
    }

    QTimer::singleShot(0, [=]() {
        if (enterWindow && leaveWindow) {
            QWindowSystemInterface::handleEnterLeaveEvent(enterWindow.data(), leaveWindow.data(), leavePos, globalLeavePos);
        } else if (enterWindow) {
            QWindowSystemInterface::handleEnterEvent(enterWindow.data(), enterPos, globalEnterPos);
        } else if (leaveWindow) {
            QWindowSystemInterface::handleLeaveEvent(leaveWindow.data());
        }
    });
}

void QGtkWindow::onLeaveContent()
{
    // reset the mouse cursor.
    QGtkRefPtr<GdkCursor> c = gdk_cursor_new_from_name(gdk_display_get_default(), "default");
    gdk_window_set_cursor(gtk_widget_get_window(m_window.get()), c.get());
}

WId QGtkWindow::winId() const
{
    return (WId)m_window.get();
}

void QGtkWindow::setParent(const QPlatformWindow *)
{
}

void QGtkWindow::setWindowTitle(const QString &title)
{
    gtk_window_set_title(GTK_WINDOW(m_window.get()), title.toUtf8().constData());
}

void QGtkWindow::setWindowFilePath(const QString &title)
{
    // we can't do anything useful with this
    Q_UNUSED(title);
}

void QGtkWindow::setWindowIcon(const QIcon &icon)
{
    if (icon.isNull()) {
        gtk_window_set_icon(GTK_WINDOW(m_window.get()), nullptr);
        return;
    }

    QGtkRefPtr<GdkPixbuf> pb = qt_iconToPixbuf(icon);

    // ### consider gtk_window_set_icon_list
    gtk_window_set_icon(GTK_WINDOW(m_window.get()), pb.get());
}

void QGtkWindow::raise()
{
    // we cannot control the stacking order
}

void QGtkWindow::lower()
{
    // we cannot control the stacking order
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

    int activeHints = GdkWindowHints(0);
    GdkGeometry hints;
    if (!minSize.isNull()) {
        hints.min_width = minSize.width();
        hints.min_height = minSize.height();
        activeHints |= GDK_HINT_MIN_SIZE;
        gtk_widget_set_size_request(GTK_WIDGET(m_content.get()), hints.min_width, hints.min_height);
    }

    if (!maxSize.isNull()) {
        hints.max_width = maxSize.width();
        hints.max_height = maxSize.height();
        activeHints |= GDK_HINT_MAX_SIZE;
    }

    if (!baseSize.isNull()) {
        hints.base_width = baseSize.width();
        hints.base_height = baseSize.height();
        activeHints |= GDK_HINT_BASE_SIZE;
    }

    if (sizeIncrement.isNull()) {
        hints.width_inc = sizeIncrement.width();
        hints.height_inc = sizeIncrement.height();
        activeHints |= GDK_HINT_RESIZE_INC;
    }

    if ((activeHints & GDK_HINT_MIN_SIZE) && (activeHints & GDK_HINT_MAX_SIZE)) {
        if (minSize == maxSize) {
            gtk_window_set_resizable(GTK_WINDOW(m_window.get()), false);
        } else {
            gtk_window_set_resizable(GTK_WINDOW(m_window.get()), true);
        }
    } else {
        gtk_window_set_resizable(GTK_WINDOW(m_window.get()), true);
    }

    gtk_window_set_geometry_hints(
        GTK_WINDOW(m_window.get()),
        m_window.get(),
        &hints,
        GdkWindowHints(activeHints)
    );
}

void QGtkWindow::setOpacity(qreal level)
{
    gtk_widget_set_opacity(m_window.get(), level);
}

void QGtkWindow::requestActivateWindow()
{
    qCDebug(lcWindow) << "Request activate" << window();
    gtk_window_present(GTK_WINDOW(m_window.get()));
}

void QGtkWindow::setMask(const QRegion &region)
{
    // can't do anything useful with this
    Q_UNUSED(region);
}

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

QGtkRefPtr<GtkWidget> QGtkWindow::gtkWindow() const
{
    return m_window;
}

QGtkRefPtr<GtkMenuBar> QGtkWindow::gtkMenuBar() const
{
    return m_menubar;
}

