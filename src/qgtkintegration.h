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

#ifndef QPLATFORMINTEGRATION_GTK_H
#define QPLATFORMINTEGRATION_GTK_H

#include "qgtkservices.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>
#include <QtCore/qscopedpointer.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

typedef void *EGLDisplay;

QT_BEGIN_NAMESPACE

class QTouchDevice;
class QGtkScreen;
class QGtkClipboard;

class QGtkIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    explicit QGtkIntegration(const QStringList &parameters);
    ~QGtkIntegration();

    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformClipboard *clipboard() const override;
    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QPlatformServices *services() const override;
    QPlatformNativeInterface *nativeInterface() const override;

    // QPlatformNativeInterface
    void *nativeResourceForIntegration(const QByteArray &resource) override;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
    NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource) override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;

    static QGtkIntegration *instance();

    void onMonitorAdded(GdkMonitor *monitor);
    void onMonitorRemoved(GdkMonitor *monitor);

    GtkApplication *application() const;

    EGLDisplay eglDisplay() const;

private:
    QScopedPointer<QGtkServices> m_services;
    QScopedPointer<QPlatformFontDatabase> m_fontDatabase;
    GdkDisplay *m_display;
    QVector<const char*> m_arguments; /* must remain allocated for gdk's sake */
    QVector<QGtkScreen*> m_screens;
    QGtkClipboard *m_clipboard = nullptr;

    EGLDisplay m_eglDisplay; // non-null for wayland platforms
};

QT_END_NAMESPACE

#endif
