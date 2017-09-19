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
#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include "CSystrace.h"

Q_LOGGING_CATEGORY(lcWindow, "qt.qpa.gtk.window");
Q_LOGGING_CATEGORY(lcWindowRender, "qt.qpa.gtk.window.render");

static void draw_cb(GtkWidget *, cairo_t *cr, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "draw_cb" << pw;
    pw->onDraw(cr);
}

static gboolean map_cb(GtkWidget *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "map_cb" << pw;
    pw->onMap();
    return FALSE;
}

static gboolean unmap_cb(GtkWidget *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "unmap_cb" << pw;
    pw->onUnmap();
    return FALSE;
}

static gboolean configure_cb(GtkWidget *, GdkEvent *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "configure_cb" << pw;
    pw->onConfigure();
    return FALSE;
}

static gboolean size_allocate_cb(GtkWidget *, GdkRectangle *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "size_allocate_cb" << pw;
    pw->onConfigure();
    return FALSE;
}

static gboolean delete_cb(GtkWidget *, GdkEvent *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindowRender) << "delete_cb" << pw;
    return pw->onDelete() ? TRUE : FALSE;
}

static gboolean key_press_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "key_press_cb" << pw;
    return pw->onKeyPress(event) ? TRUE : FALSE;
}

static gboolean key_release_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "key_release_cb" << pw;
    return pw->onKeyRelease(event) ? TRUE : FALSE;
}

static gboolean button_press_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "button_press_cb" << pw;
    return pw->onButtonPress(event) ? TRUE : FALSE;
}

static gboolean button_release_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "button_release_cb" << pw;
    return pw->onButtonRelease(event) ? TRUE : FALSE;
}

static gboolean touch_event_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "touch_event_cb" << pw;
    return pw->onTouchEvent(event) ? TRUE : FALSE;
}

static gboolean motion_notify_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "motion_notify_cb" << pw;
    return pw->onMotionNotify(event) ? TRUE : FALSE;
}

static gboolean scroll_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "scroll_cb" << pw;
    return pw->onScrollEvent(event) ? TRUE : FALSE;
}

static gboolean window_state_event_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "window_state_event_cb" << pw;
    pw->onWindowStateEvent(event);
    return FALSE;
}

static gboolean window_tick_cb(GtkWidget*, GdkFrameClock *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "window_tick_cb" << pw;
    pw->onUpdateFrameClock();
    return G_SOURCE_CONTINUE;
}

static gboolean enter_window_notify_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "enter_window_notify_cb" << pw;
    pw->onEnterLeaveWindow(event, true);
    return false;
}

static gboolean leave_window_notify_cb(GtkWidget *, GdkEvent *event, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "leave_window_notify_cb" << pw;
    pw->onEnterLeaveWindow(event, false);
    return false;
}

static gboolean leave_content_notify_cb(GtkWidget *, GdkEvent *, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    qCDebug(lcWindow) << "leave_content_notify_cb" << pw;
    pw->onLeaveContent();
    return false;
}

class QGtkCourierObject : public QObject
{
    Q_OBJECT

public:
    static QGtkCourierObject *instance;

    QGtkCourierObject(QObject *parent = nullptr)
        : QObject(parent)
    {
        qRegisterMetaType<QGtkWindow*>("QGtkWindow*");
    }

    Q_INVOKABLE void queueDraw(QGtkWindow *win)
    {
        gtk_widget_queue_draw(win->gtkWindow().get());
    }
};

QGtkCourierObject *QGtkCourierObject::instance;


void QGtkWindow::onUpdateFrameClock()
{
    TRACE_EVENT0("gfx", "QGtkWindow::onUpdateFrameClock");
    if (m_wantsUpdate) {
        m_wantsUpdate = false;
        TRACE_EVENT_ASYNC_END0("gfx", "QGtkWindow::requestUpdate", this);
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

    GtkWindowType gtkWindowType = GTK_WINDOW_TOPLEVEL;
    if (windowType == Qt::ToolTip ||
        windowType == Qt::Popup) {
        gtkWindowType = GTK_WINDOW_POPUP;
    }

    m_window = gtk_window_new(gtkWindowType);

    if (windowType == Qt::ToolTip) {
        gtk_window_set_type_hint(GTK_WINDOW(m_window.get()), GDK_WINDOW_TYPE_HINT_TOOLTIP);
    }

    g_signal_connect(m_window.get(), "map", G_CALLBACK(map_cb), this);
    g_signal_connect(m_window.get(), "unmap", G_CALLBACK(unmap_cb), this);
    g_signal_connect(m_window.get(), "configure-event", G_CALLBACK(configure_cb), this);
    g_signal_connect(m_window.get(), "enter-notify-event", G_CALLBACK(enter_window_notify_cb), this);
    g_signal_connect(m_window.get(), "leave-notify-event", G_CALLBACK(leave_window_notify_cb), this);

    // for whatever reason, configure-event is not enough. it doesn't seem to
    // get emitted for popup type windows. so also connect to size-allocate just
    // to be sure...
    g_signal_connect(m_window.get(), "size-allocate", G_CALLBACK(size_allocate_cb), this);
    g_signal_connect(m_window.get(), "delete-event", G_CALLBACK(delete_cb), this);
    g_signal_connect(m_window.get(), "window-state-event", G_CALLBACK(window_state_event_cb), this);
    m_tick_callback = gtk_widget_add_tick_callback(m_window.get(), window_tick_cb, this, NULL);
    setGeometry(window()->geometry());

    if (windowType == Qt::ToolTip ||
        windowType == Qt::Popup ||
        window()->modality() != Qt::NonModal) {
        const QWindow *transientParent = window()->transientParent();
        if (!transientParent)
            transientParent = qApp->focusWindow();
        if (transientParent && transientParent->handle()) {
            QGtkWindow *transientParentPlatform = static_cast<QGtkWindow*>(transientParent->handle());
            gtk_window_set_transient_for(GTK_WINDOW(m_window.get()), GTK_WINDOW(transientParentPlatform->gtkWindow().get()));
        }
    }

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
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

    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen); // ### use GdkDevice or not?
    m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::MouseEmulation);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);

    setWindowState(window()->windowState());
    propagateSizeHints();
    setWindowFlags(window()->flags());
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

#undef LOCK_DEBUG

void QGtkWindow::onDraw(cairo_t *cr)
{
    if (m_newGeometry != m_windowGeometry) {
        bool needsExpose = m_newGeometry.size() != m_windowGeometry.size();
        QWindowSystemInterface::handleGeometryChange(window(), m_newGeometry, m_windowGeometry);
        m_windowGeometry = m_newGeometry;

        if (needsExpose) {
            QWindowSystemInterface::handleExposeEvent(window(), m_windowGeometry);

            // we must flush, otherwise the content we'll render might be out of date.
            // ### would be nice if we could compress these, somehow: at the least we'll
            // get a configure and a size-allocate independent of each other.
            QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }
    TRACE_EVENT0("gfx", "QGtkWindow::onDraw");
    // Hold frameMutex during blit to cairo to prevent changes
    QMutexLocker lock(&m_frameMutex);
#if defined(LOCK_DEBUG)
    qWarning() << "rendering (LOCKED)" << this;
#endif
    if (m_frame.isNull())
        return;

#if 0
    QString clipString;
    cairo_rectangle_list_t *clip = cairo_copy_clip_rectangle_list(cr);
    for (int i = 0; i < clip->num_rectangles; i++) {
        auto r = clip->rectangles[i];
        clipString += QString("%1,%2@%3x%4  ").arg(r.x).arg(r.y).arg(r.width).arg(r.height);
    }
    qDebug(lcWindow) << "onDraw with clip:" << clipString;
#endif

    cairo_surface_t *surf = cairo_image_surface_create_for_data(
            const_cast<uchar*>(m_frame.constBits()),
            CAIRO_FORMAT_ARGB32,
            m_frame.width(),
            m_frame.height(),
            m_frame.bytesPerLine()
    );
    int sf = gtk_widget_get_scale_factor(m_window.get());
    cairo_surface_set_device_scale(surf, sf, sf);
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    // cairo_paint respects the current clip, which GTK sets based on
    // updated regions of the window, so we don't need to do anything
    // other than include the updated regions in queue_draw calls.
    cairo_paint(cr);
    cairo_surface_destroy(surf);
#if defined(LOCK_DEBUG)
    qWarning() << "rendering (UNLOCKING)" << this;
#endif
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
    int x;
    int y;
    gtk_window_get_position(GTK_WINDOW(m_window.get()), &x, &y);

    GdkRectangle r;
    gtk_widget_get_allocated_size(m_content.get(), &r, nullptr);
    m_newGeometry = QRect(x, y, r.width, r.height);
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

    Qt::WindowType type = static_cast<Qt::WindowType>(int(m_flags & Qt::WindowType_Mask));

    // We must not send window activation changes for tooltip windows, as they
    // will auto-hide on activation change.
    if (type != Qt::ToolTip && (ev->changed_mask & GDK_WINDOW_STATE_FOCUSED)) {
        static QPointer<QWindow> newActiveWindow = nullptr;
        if (ev->new_window_state & GDK_WINDOW_STATE_FOCUSED) {
            newActiveWindow = window();
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

void QGtkWindow::requestUpdate()
{
    TRACE_EVENT_ASYNC_BEGIN0("gfx", "QGtkWindow::requestUpdate", this);
    m_wantsUpdate = true;
}

QImage *QGtkWindow::beginUpdateFrame(const QString &reason)
{
    m_frameMutex.lock();
    Q_UNUSED(reason);
#if defined(LOCK_DEBUG)
    qWarning() << "beginUpdateFrame " << reason << "(LOCKED)" << this;
#endif
    return &m_frame;
}

static cairo_region_t *cairo_region_from_region(const QRegion &region)
{
    cairo_region_t *r = cairo_region_create();
    for (const QRect &qrect : region.rects()) {
        cairo_rectangle_int_t rect = { qrect.x(), qrect.y(), qrect.width(), qrect.height() };
        cairo_region_union_rectangle(r, &rect);
    }
    return r;
}

void QGtkWindow::endUpdateFrame(const QString &reason)
{
    m_frameMutex.unlock();
    Q_UNUSED(reason);
#if defined(LOCK_DEBUG)
    qWarning() << "endUpdateFrame " << reason << "(UNLOCKED)" << this;
#endif
}

void QGtkWindow::invalidateRegion(const QRegion &region)
{
    auto courier = QGtkCourierObject::instance;
    Q_ASSERT(courier);
    if (courier->thread() != QThread::currentThread()) {
        // In the multithreaded case, always signal a full screen update for now
        courier->metaObject()->invokeMethod(courier, "queueDraw", Qt::QueuedConnection, Q_ARG(QGtkWindow*, this));
        return;
    }

    QRegion realRegion = region.isNull() ? QRegion(m_frame.rect()) : region;
    cairo_region_t *cairoRegion = cairo_region_from_region(realRegion);
    gtk_widget_queue_draw_region(m_content.get(), cairoRegion);
    cairo_region_destroy(cairoRegion);
}

QImage QGtkWindow::currentFrameImage() const
{
    return m_frame;
}

QGtkRefPtr<GtkWidget> QGtkWindow::gtkWindow() const
{
    return m_window;
}

QGtkRefPtr<GtkMenuBar> QGtkWindow::gtkMenuBar() const
{
    return m_menubar;
}

#include "qgtkwindow.moc"
