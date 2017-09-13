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

void activate_cb(GApplication *, gpointer)
{
    qDebug() << "Activate callback";
}

QGtkIntegration::QGtkIntegration(const QStringList &)
    : m_services(new QGenericUnixServices)
    , m_fontDatabase(new QGenericUnixFontDatabase)
{
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
}

QGtkIntegration::~QGtkIntegration()
{
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
    if (!context->nativeHandle().isNull()) {
        GdkGLContext *nativeContext = reinterpret_cast<GdkGLContext*>(context->nativeHandle().value<void*>());
        if (!nativeContext) {
            return nullptr;
        }

        return new QGtkOpenGLInternalContext(nativeContext);
    }
    return new QGtkOpenGLContext(context->format());
}

bool QGtkIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
    case MultipleWindows:
    case OpenGL:
        return true;
    case ThreadedOpenGL:
        return false;
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
    qWarning() << "Unimplemented request for " << resource << " on " << screen;
    return result;
}

void *QGtkIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    void *result = 0;
    qWarning() << "Unimplemented request for " << resource << " on " << window;
    return result;
}

void *QGtkIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    void *result = 0;
    qWarning() << "Unimplemented request for " << resource << " on " << context;
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
