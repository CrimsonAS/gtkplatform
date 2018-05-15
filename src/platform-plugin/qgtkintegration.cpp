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

#include "qgtkintegration.h"
#include "qgtkbackingstore.h"
#include "qgtkwindow.h"
#include "qgtkscreen.h"
#include "qgtktheme.h"
#include "qgtkopenglcontext.h"
#include "qgtkeventdispatcher.h"
#include "qgtkclipboard.h"

#include <QtWidgets/qapplication.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,8,0)
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#include <QtServiceSupport/private/qgenericunixservices_p.h>
#else
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>
#endif

#include <gtk/gtk.h>

#include <libnotify/notify.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <EGL/egl.h>
#include <gdk/gdkwayland.h>
static EGLDisplay createWaylandEGLDisplay(wl_display *display);
#endif
#ifdef GDK_WINDOWING_X11
# include <gdk/gdkx.h>
# include <X11/Xlib-xcb.h>
#endif

#include "CSystrace.h"

QT_BEGIN_NAMESPACE

class QCoreTextFontEngine;

void monitor_added(GdkDisplay *, GdkMonitor *monitor, gpointer integration)
{
    QGtkIntegration *ig = static_cast<QGtkIntegration*>(integration);
    ig->onMonitorAdded(monitor);
}

void monitor_removed(GdkDisplay *, GdkMonitor *monitor, gpointer integration)
{
    QGtkIntegration *ig = static_cast<QGtkIntegration*>(integration);
    ig->onMonitorRemoved(monitor);
}

QGtkIntegration::QGtkIntegration(const QStringList &)
    : m_services(new QGtkServices)
    , m_fontDatabase(new QGenericUnixFontDatabase)
    , m_eglDisplay(nullptr)
{
    systrace_init();
    gtk_init(NULL, NULL);
    notify_init(qApp->applicationName().toUtf8().constData());

    // Set up screens
    m_display = gdk_display_get_default();
    g_signal_connect(m_display, "monitor-added", G_CALLBACK(monitor_added), this);
    g_signal_connect(m_display, "monitor-removed", G_CALLBACK(monitor_removed), this);

    int num_monitors = gdk_display_get_n_monitors(m_display);

    for (int i = 0; i < num_monitors; i++) {
        GdkMonitor *monitor = gdk_display_get_monitor(m_display, i);
        monitor_added(m_display, monitor, this);
    }

#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(m_display)) {
        wl_display *wldisplay = gdk_wayland_display_get_wl_display(GDK_WAYLAND_DISPLAY(m_display));
        m_eglDisplay = createWaylandEGLDisplay(wldisplay);
        Q_ASSERT(m_eglDisplay);
    }
    else
#endif
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(m_display)) {
        qWarning() << "Application is running under X11. While this may work, it is experimental.";
        qWarning() << "Run under Wayland for best results.";
    }
    else
#endif
        qWarning("GTK platform does not support this display backend; GL contexts will fail");
}

QGtkIntegration::~QGtkIntegration()
{
    notify_uninit();
#ifdef GDK_WINDOWING_WAYLAND
    if (m_eglDisplay) {
        eglTerminate(m_eglDisplay);
    }
#endif
    systrace_deinit();
}

void QGtkIntegration::onMonitorAdded(GdkMonitor *monitor)
{
    qDebug() << "Added " << monitor;
    m_screens.append(new QGtkScreen(monitor));
    screenAdded(m_screens.at(m_screens.count() - 1));
}

void QGtkIntegration::onMonitorRemoved(GdkMonitor *monitor)
{
    qDebug() << "Removed " << monitor;
    for (int i = 0; i < m_screens.count(); ++i) {
        if (m_screens.at(i)->monitor() == monitor) {
            removeScreen(m_screens.at(i)->screen());
            m_screens.removeAt(i);
            return;
        }
    }

}

QPlatformOpenGLContext *QGtkIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(m_display)) {
        return new QGtkWaylandContext(context->format(), static_cast<QGtkOpenGLContext*>(context->shareHandle()));
    }
#endif
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(m_display)) {
        return new QGtkX11Context(context->format(), static_cast<QGtkOpenGLContext*>(context->shareHandle()));
    }
#endif
    return nullptr;
}

bool QGtkIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
    case MultipleWindows:
    case OpenGL:
    case ThreadedOpenGL:
    case RasterGLSurface:
    case WindowManagement:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformClipboard *QGtkIntegration::clipboard() const
{
    if (m_clipboard == nullptr) {
        QGtkIntegration *that = const_cast<QGtkIntegration*>(this);
        that->m_clipboard = new QGtkClipboard(that);
    }
    return this->m_clipboard;
}

QPlatformFontDatabase *QGtkIntegration::fontDatabase() const
{
    return m_fontDatabase.data();
}

QStringList QGtkIntegration::themeNames() const
{
    return QStringList(QLatin1String(QGtkTheme::name));
}

QPlatformTheme *QGtkIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String(QGtkTheme::name))
        return new QGtkTheme;
    return QPlatformIntegration::createPlatformTheme(name);
}

QPlatformServices *QGtkIntegration::services() const
{
    return m_services.data();
}

QPlatformNativeInterface *QGtkIntegration::nativeInterface() const
{
    return const_cast<QGtkIntegration*>(this);
}

void *QGtkIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
    void *result = 0;

    if (resource == "egldisplay") {
        result = reinterpret_cast<void*>(m_eglDisplay);
    } else if (resource == "connection") {
#ifdef GDK_WINDOWING_X11
        static bool xcb_warned = false;
        if (!xcb_warned) {
            qWarning() << "XCB connection requested; this is experimental, and may not work well.";
            xcb_warned = true;
        }
        Display *dpy = nullptr;
        if (GDK_IS_X11_DISPLAY(m_display)) {
            dpy = gdk_x11_display_get_xdisplay(m_display);
        } else {
            qWarning() << "Can't get XCB connection, GDK_BACKEND is not X11.";
        }
        xcb_connection_t *conn = XGetXCBConnection(dpy);
        result = reinterpret_cast<void*>(conn);
#endif
    } else if (resource == "display") {
#ifdef GDK_WINDOWING_X11
        static bool x11_warned = false;
        if (!x11_warned) {
            qWarning() << "X11 display handle; this is experimental, and may not work well.";
            x11_warned = true;
        }
        Display *dpy = nullptr;
        if (GDK_IS_X11_DISPLAY(m_display)) {
            dpy = gdk_x11_display_get_xdisplay(m_display);
        } else {
            qWarning() << "Can't get XCB connection, GDK_BACKEND is not X11.";
        }
        result = reinterpret_cast<void*>(dpy);
#endif
    } else {
        qWarning() << "Unimplemented request for " << resource;
    }

    return result;
}

void *QGtkIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    void *result = 0;
    QByteArray res = resource.toLower();
    // ### notify on change
    if (res == "antialiasingenabled") {
        int aa = -1;
        g_object_get(gtk_settings_get_default(), "gtk-xft-antialias", &aa, NULL);
        result = reinterpret_cast<void*>(aa + 1);
    } else if (res == "subpixeltype") {
        GtkSettings *s = gtk_settings_get_default();
        gchararray value;
        g_object_get(s, "gtk-xft-rgba", &value, NULL);
        QString qtVal = QString::fromUtf8(value);
        g_free(value);

        QFontEngine::SubpixelAntialiasingType type = QFontEngine::SubpixelAntialiasingType(-1);
        if (qtVal == "none") {
            type = QFontEngine::Subpixel_None;
        } else if (qtVal == "rgb") {
            type = QFontEngine::Subpixel_RGB;
        } else if (qtVal == "bgr") {
            type = QFontEngine::Subpixel_BGR;
        } else if (qtVal == "vrgb") {
            type = QFontEngine::Subpixel_VRGB;
        } else if (qtVal == "vbgr") {
            type = QFontEngine::Subpixel_VBGR;
        }

        result = reinterpret_cast<void*>(type + 1);
    } else if (resource == "rootwindow") {
#ifdef GDK_WINDOWING_X11
        static bool rootwin_warned = false;
        if (!rootwin_warned) {
            qWarning() << "X root window requested; this is experimental, and may not work well.";
            rootwin_warned = true;
        }
        Display *dpy = nullptr;
        xcb_screen_t *screen = nullptr;
        if (GDK_IS_X11_DISPLAY(m_display)) {
            dpy = gdk_x11_display_get_xdisplay(m_display);
            xcb_connection_t *conn = XGetXCBConnection(dpy);

            // use the first screen... hopefully this is okay? sigh...
            screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
        } else {
            qWarning() << "Can't get root X window, GDK_BACKEND is not X11.";
        }

        result = reinterpret_cast<void*>(screen ? screen->root : nullptr);
#endif
    } else {
        qWarning() << "Unimplemented request for " << resource << " on " << screen;
    }
    return result;
}

void *QGtkIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    void *result = 0;
    if (resource == "gtkwindow") {
        return static_cast<QGtkWindow*>(window->handle())->gtkWindow().get();
    }
    qWarning() << "Unimplemented request for " << resource << " on " << window;
    return result;
}

void *QGtkIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    void *result = 0;

    if (!context->handle()) {
        return result;
    }

    result = static_cast<QGtkOpenGLContext*>(context->handle())->nativeResource(resource);
    if (!result) {
        qWarning() << "Unimplemented request for " << resource << " on " << context;
    }
    return result;
}

QPlatformNativeInterface::NativeResourceForContextFunction QGtkIntegration::nativeResourceFunctionForContext(const QByteArray &resource)
{
    qWarning() << "Unimplemented request for " << resource;
    return 0;
}

QPlatformWindow *QGtkIntegration::createPlatformWindow(QWindow *window) const
{
    Q_UNUSED(window);
    QGtkWindow *w = new QGtkWindow(window);
    return w;
}

QPlatformBackingStore *QGtkIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QGtkBackingStore(window);
}

QAbstractEventDispatcher *QGtkIntegration::createEventDispatcher() const
{
    return new QGtkEventDispatcher;
}

QGtkIntegration *QGtkIntegration::instance()
{
    return static_cast<QGtkIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

#ifdef GDK_WINDOWING_WAYLAND
static EGLDisplay createWaylandEGLDisplay(wl_display *display)
{
    eglBindAPI(EGL_OPENGL_API);

    EGLDisplay dpy = eglGetDisplay((EGLNativeDisplayType)display);
    if (dpy == EGL_NO_DISPLAY) {
        qWarning() << "eglGetDisplay failed";
        return dpy;
    }

    if (!eglInitialize(dpy, NULL, NULL)) {
        qWarning() << "eglInitialize failed";
        return EGL_NO_DISPLAY;
    }

    return dpy;
}
#endif

EGLDisplay QGtkIntegration::eglDisplay() const
{
    return m_eglDisplay;
}

QT_END_NAMESPACE
