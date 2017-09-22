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
#include "qgtkmenu.h"
#include "qgtkmenuitem.h"
#include "qgtkhelpers.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcMenu, "qt.qpa.gtk.menu");

QGtkMenu::QGtkMenu()
    : m_tag((qintptr)this)
{
}

QGtkMenu::~QGtkMenu()
{
}

void QGtkMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    QGtkMenuItem *mi = static_cast<QGtkMenuItem*>(menuItem);
    QGtkMenuItem *bi = static_cast<QGtkMenuItem*>(before);

    int idx = m_items.indexOf(bi);
    if (idx < 0) {
        m_items.append(mi);
    } else {
        m_items.insert(idx, mi);
    }
    if (mi->menu()) {
        connect(mi->menu(), &QGtkMenu::updated, this, &QGtkMenu::updated, Qt::UniqueConnection);
    }
    qCDebug(lcMenu) << "Added menu item " << mi << " before " << bi << " at " << idx;
    Q_EMIT updated();
}

void QGtkMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    QGtkMenuItem *mi = static_cast<QGtkMenuItem*>(menuItem);
    int idx = m_items.indexOf(mi);
    m_items.removeAt(idx);
    m_items.removeAll(0); // if it was deleted, remove those too.
    if (mi->menu()) {
        disconnect(mi->menu(), &QGtkMenu::updated, this, &QGtkMenu::updated);
    }
    Q_EMIT updated();
}

void QGtkMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    QGtkMenu *m = static_cast<QGtkMenuItem*>(menuItem)->menu();
    if (m) {
        connect(m, &QGtkMenu::updated, this, &QGtkMenu::updated, Qt::UniqueConnection);
    }

    Q_EMIT updated();
}

void QGtkMenu::syncSeparatorsCollapsible(bool enable)
{
    Q_UNUSED(enable);
}

void QGtkMenu::setTag(quintptr tag)
{
    m_tag = tag;
    Q_EMIT updated();
}

quintptr QGtkMenu::tag()const
{
    return m_tag;
}

void QGtkMenu::setText(const QString &text)
{
    m_text = text;
    Q_EMIT updated();
}

void QGtkMenu::setIcon(const QIcon &icon)
{
    Q_UNUSED(icon);
}

void QGtkMenu::setEnabled(bool enabled)
{
    m_enabled = enabled;
    Q_EMIT updated();
}

bool QGtkMenu::isEnabled() const
{
    return m_enabled;
}

void QGtkMenu::setVisible(bool visible)
{
    //aboutToShow, aboutToHide signals
    Q_UNUSED(visible);
    m_visible = visible;
    Q_EMIT updated();
}

void QGtkMenu::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
{
    Q_UNUSED(item);

    if (m_popup) {
        qCWarning(lcMenu) << "Trying to showPopup while one is already around";
        dismiss();
    }

    Q_EMIT aboutToShow();

    m_popup = gtkMenu();
    GdkRectangle gRect { targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height() };
    gtk_menu_popup_at_rect(
        m_popup.get(),
        gtk_widget_get_window(static_cast<QGtkWindow*>(parentWindow->handle())->gtkWindow().get()),
        &gRect,
        GdkGravity(GDK_GRAVITY_NORTH_WEST),
        GdkGravity(GDK_GRAVITY_NORTH_WEST),
        NULL
    );
    connect(qGuiApp, &QGuiApplication::focusObjectChanged, this, &QGtkMenu::dismiss);
}

void QGtkMenu::dismiss()
{
    Q_EMIT aboutToHide();

    if (m_popup) {
        gtk_menu_popdown(m_popup.get());
        gtk_widget_destroy(GTK_WIDGET(m_popup.get()));
        m_popup = nullptr;
    }
    disconnect(qGuiApp, &QGuiApplication::focusObjectChanged, this, &QGtkMenu::dismiss);
}

QPlatformMenuItem *QGtkMenu::menuItemAt(int position) const
{
    int idx = 0;
    while (position >= 0 && idx < m_items.size()) {
        if (m_items.at(idx)) {
            position--;
        }
        idx++;
    }
    if (idx >= 0 && idx < m_items.size())
        return m_items.at(idx);
    return nullptr;
}

QPlatformMenuItem *QGtkMenu::menuItemForTag(quintptr tag) const
{
    for (QGtkMenuItem *item : qAsConst(m_items)) {
        if (item && item->tag() == tag) {
            return item;
        }
    }

    return nullptr;
}

QGtkRefPtr<GtkMenuItem> QGtkMenu::gtkMenuItem() const
{
    QGtkRefPtr<GtkMenuItem> mi = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(qt_convertToGtkMnemonics(m_text).toUtf8().constData()));
    gtk_menu_item_set_submenu(mi.get(), GTK_WIDGET(gtkMenu().get()));
    gtk_widget_set_sensitive(GTK_WIDGET(mi.get()), m_enabled);
    gtk_widget_set_visible(GTK_WIDGET(mi.get()), m_visible);
    return mi;
}

QGtkRefPtr<GtkMenu> QGtkMenu::gtkMenu() const
{
    QGtkRefPtr<GtkMenu> menu = GTK_MENU(gtk_menu_new());
    for (QGtkMenuItem *i : m_items) {
        if (i)
            gtk_menu_shell_append(GTK_MENU_SHELL(menu.get()), i->gtkMenuItem().get());
    }
    return menu;
}

QVector<QPointer<QGtkMenuItem>> QGtkMenu::items() const
{
    return m_items;
}

