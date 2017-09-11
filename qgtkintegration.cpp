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

#include "qgtkintegration.h"
#include "qgtkbackingstore.h"
#include "qgtkwindow.h"
#include "qgtkscreen.h"
#include "qgtktheme.h"
#include "qgtkopenglcontext.h"
#include "qgtkstyle.h"
//#include "qgtkclipboard.h"

#include <QtWidgets/qapplication.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>

#include <gtk/gtk.h>

QT_BEGIN_NAMESPACE

class QCoreTextFontEngine;

void monitor_added(GdkDisplay *display, GdkMonitor *monitor, gpointer integration)
{
    QGtkIntegration *ig = static_cast<QGtkIntegration*>(integration);
    ig->onMonitorAdded(monitor);
}

void monitor_removed(GdkDisplay *display, GdkMonitor *monitor, gpointer integration)
{
    QGtkIntegration *ig = static_cast<QGtkIntegration*>(integration);
    ig->onMonitorRemoved(monitor);
}

void device_added(GdkSeat *seat, GdkDevice *device, gpointer integration)
{
    QGtkIntegration *ig = static_cast<QGtkIntegration*>(integration);
    ig->onDeviceAdded(device);
}

void device_removed(GdkSeat *seat, GdkDevice *device, gpointer integration)
{
    QGtkIntegration *ig = static_cast<QGtkIntegration*>(integration);
    ig->onDeviceRemoved(device);
}

void activate_cb(GApplication *, gpointer)
{
    qDebug() << "Activate callback";
}

QGtkIntegration::QGtkIntegration(const QStringList &)
    : m_services(new QGenericUnixServices)
    , m_fontDatabase(new QGenericUnixFontDatabase)
{
    // ### we need this for notifications, but it isn't working right.
    // need to write an event dispatcher I think.
    //m_application = gtk_application_new("org.qt-project.app", G_APPLICATION_FLAGS_NONE);
    //g_application_register(G_APPLICATION(m_application), NULL, NULL);
    //g_signal_connect(m_application, "activate", G_CALLBACK(activate_cb), this);
    //g_signal_connect(m_application, "startup", G_CALLBACK(startup_cb), this);
    gtk_init(NULL, NULL);

    // Set up screens
    m_display = gdk_display_get_default();
    g_signal_connect(m_display, "monitor-added", G_CALLBACK(monitor_added), this);
    g_signal_connect(m_display, "monitor-removed", G_CALLBACK(monitor_removed), this);

    int num_monitors = gdk_display_get_n_monitors(m_display);

    for (int i = 0; i < num_monitors; i++) {
        GdkMonitor *monitor = gdk_display_get_monitor(m_display, i);
        monitor_added(m_display, monitor, this);
    }

    // And input devices
    GdkSeat *seat = gdk_display_get_default_seat(m_display);
    g_signal_connect(seat, "device-added", G_CALLBACK(device_added), this);
    g_signal_connect(seat, "device-removed", G_CALLBACK(device_added), this);

    GList *slaves = gdk_seat_get_slaves(seat, GDK_SEAT_CAPABILITY_ALL);
    GList *l;

    for (l = slaves; l != NULL; l = l->next)
    {
        device_added(seat, (GdkDevice*)l->data, this);
    }

    g_list_free(slaves);

    // ### not finished at all
    //QApplication::setStyle(new QGtkStyle);
}

QGtkIntegration::~QGtkIntegration()
{
    for (int i = 0; i < m_devices.count(); ++i) {
        delete m_devices.at(i).m_qTouchDevice;
    }
    m_devices.clear();

    //g_object_unref(m_application);
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

void QGtkIntegration::onDeviceAdded(GdkDevice *device)
{
    qDebug() << "Added " << device;

    // ### does any of this make sense?
    return;
#if 0
    GdkInputSource source = gdk_device_get_source(device);
    const gchar *name = gdk_device_get_name(device);
    const gchar *type = "";
    bool isTouch = false;
    QTouchDevice::DeviceType touchType = QTouchDevice::TouchScreen;

    switch (source) {
    case GDK_SOURCE_MOUSE:
        type = "mouse";
        break;
    case GDK_SOURCE_PEN:
        type = "pen";
        break;
    case GDK_SOURCE_ERASER:
        type = "eraser";
        break;
    case GDK_SOURCE_CURSOR:
        type = "cursor";
        break;
    case GDK_SOURCE_KEYBOARD:
        type = "keyboard";
        break;
    case GDK_SOURCE_TOUCHSCREEN:
        type = "touchscreen";
        isTouch = true;
        touchType = QTouchDevice::TouchScreen;
        break;
    case GDK_SOURCE_TOUCHPAD:
        type = "touchpad";
        isTouch = true;
        touchType = QTouchDevice::TouchPad;
        break;
    case GDK_SOURCE_TRACKPOINT:
        type = "trackpoint";
        break;
    case GDK_SOURCE_TABLET_PAD:
        type = "tablet pad";
        break;
    }

    qDebug() << "Device added: " << name << type;

    if (isTouch) {
        QTouchDevice::Capabilities caps = QTouchDevice::Position;
        GdkAxisFlags axes = gdk_device_get_axes(device);

        if (axes & GDK_AXIS_FLAG_PRESSURE)
            caps |= QTouchDevice::Pressure;

        QTouchDevice *d = new QTouchDevice;
        d->setType(touchType);
        d->setCapabilities(caps);
        QWindowSystemInterface::registerTouchDevice(d);
        m_devices.append(QGdkDevice{device, d});
    }
#endif
}

void QGtkIntegration::onDeviceRemoved(GdkDevice *device)
{
    qDebug() << "Removed " << device;

    for (int i = 0; i < m_devices.length(); ++i) {
        const QGdkDevice &d = m_devices.at(i);
        if (d.m_gdkDevice == device) {
            QWindowSystemInterface::unregisterTouchDevice(d.m_qTouchDevice);
            delete d.m_qTouchDevice;
            m_devices.removeAt(i);
            break;
        }
    }
}

GtkApplication *QGtkIntegration::application() const
{
    return m_application;
}

QPlatformOpenGLContext *QGtkIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QGtkOpenGLContext(context);
}

bool QGtkIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
    case MultipleWindows:
    case OpenGL:
    case ThreadedOpenGL:
    case WindowManagement:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

//QPlatformClipboard *QGtkIntegration::clipboard() const
//{
//    return new QGtkClipboard;
//}

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
    qWarning() << "Unimplemented request for " << resource;
    return result;
}

void *QGtkIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    void *result = 0;
    qWarning() << "Unimplemented request for " << resource;
    return result;
}

void *QGtkIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    void *result = 0;
    qWarning() << "Unimplemented request for " << resource;
    return result;
}

void *QGtkIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    void *result = 0;
    qWarning() << "Unimplemented request for " << resource;
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
    w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *QGtkIntegration::createPlatformBackingStore(QWindow *window) const
{
    qDebug() << "Creating backing store for window " << window;
    return new QGtkBackingStore(window);
}

QAbstractEventDispatcher *QGtkIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QGtkIntegration *QGtkIntegration::instance()
{
    return static_cast<QGtkIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QT_END_NAMESPACE
