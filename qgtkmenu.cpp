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
#include "qgtkmenu.h"
#include "qgtkmenuitem.h"
#include "qgtkhelpers.h"

#include <QtGui/qwindow.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcMenu, "qt.qpa.gtk.menu");

QGtkMenu::QGtkMenu()
    : m_tag((qintptr)this)
{
    m_menu = GTK_MENU(gtk_menu_new());
    m_menuItem = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(""));
    gtk_menu_item_set_submenu(m_menuItem.get(), GTK_WIDGET(m_menu.get()));
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
        m_gtkItems.append(mi->sync());
        m_items.append(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(m_menu.get()), m_gtkItems.last().get());
    } else {
        m_gtkItems.insert(idx, mi->sync());
        m_items.insert(idx, mi);
        gtk_menu_shell_insert(GTK_MENU_SHELL(m_menu.get()), m_gtkItems.at(idx).get(), idx);
    }
    qCDebug(lcMenu) << "Added menu item " << mi << " before " << bi << " at " << idx;
}

void QGtkMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    QGtkMenuItem *mi = static_cast<QGtkMenuItem*>(menuItem);

    int idx = m_items.indexOf(mi);
    m_items.removeAt(idx);
    gtk_container_remove(GTK_CONTAINER(m_menu.get()), m_gtkItems.takeAt(idx).get());
}

void QGtkMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    Q_UNUSED(menuItem);
    QGtkMenuItem *mi = static_cast<QGtkMenuItem*>(menuItem);
    QGtkRefPtr<GtkWidget> oldItem = mi->gtkMenuItem();
    QGtkRefPtr<GtkWidget> newItem = mi->sync();

    if (oldItem != newItem) {
        Q_ASSERT(oldItem);
        Q_ASSERT(newItem);
        int oldIdx = m_gtkItems.indexOf(oldItem);
        qCDebug(lcMenu) << "Sync removing item " << oldItem << " for new " << newItem << " at pos " << oldIdx;
        Q_ASSERT(oldIdx != -1);
        gtk_container_remove(GTK_CONTAINER(m_menu.get()), m_gtkItems.at(oldIdx).get());
        gtk_menu_shell_insert(GTK_MENU_SHELL(m_menu.get()), newItem.get(), oldIdx);
        m_gtkItems[oldIdx] = newItem;
    }
}

void QGtkMenu::syncSeparatorsCollapsible(bool enable)
{
    Q_UNUSED(enable);
}

void QGtkMenu::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QGtkMenu::tag()const
{
    return m_tag;
}

void QGtkMenu::setText(const QString &text)
{
    QString gtkText = qt_convertToGtkMnemonics(text);

    GtkWidget *child = gtk_bin_get_child(GTK_BIN(m_menuItem.get()));
    gtk_label_set_markup_with_mnemonic(GTK_LABEL(child), gtkText.toUtf8().constData());
}

void QGtkMenu::setIcon(const QIcon &icon)
{
    Q_UNUSED(icon);
}

void QGtkMenu::setEnabled(bool enabled)
{
    m_enabled = enabled;
    gtk_widget_set_sensitive(GTK_WIDGET(m_menuItem.get()), m_enabled);
}

bool QGtkMenu::isEnabled() const
{
    return m_enabled;
}

void QGtkMenu::setVisible(bool visible)
{
    //aboutToShow, aboutToHide signals
    Q_UNUSED(visible);
    gtk_widget_set_visible(GTK_WIDGET(m_menuItem.get()), visible);
}

void QGtkMenu::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
{
    Q_UNUSED(item);

    // ### this isn't called because of a Q_OS check in qtbase?
    QGtkRefPtr<GtkMenuItem> mi = gtkMenuItem();
    GtkWidget *menu = gtk_menu_item_get_submenu(mi.get());
    GdkRectangle gRect { targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height() };
    gtk_menu_popup_at_rect(
        GTK_MENU(menu),
        GDK_WINDOW(static_cast<QGtkWindow*>(parentWindow->handle())->gtkWindow()),
        &gRect,
        GdkGravity(GDK_GRAVITY_NORTH_WEST),
        GdkGravity(GDK_GRAVITY_NORTH_WEST),
        NULL
    );
}

void QGtkMenu::dismiss()
{
    qCWarning(lcMenu) << "Stub";
}

QPlatformMenuItem *QGtkMenu::menuItemAt(int position) const
{
    return m_items.at(position);
}

QPlatformMenuItem *QGtkMenu::menuItemForTag(quintptr tag) const
{
    for (QGtkMenuItem *item : qAsConst(m_items)) {
        if (item->tag() == tag) {
            return item;
        }
    }

    return nullptr;
}

QGtkRefPtr<GtkMenuItem> QGtkMenu::gtkMenuItem() const
{
    return m_menuItem;
}

