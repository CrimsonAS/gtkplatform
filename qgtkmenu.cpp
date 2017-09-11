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

QGtkMenu::QGtkMenu()
    : m_tag((qintptr)this)
{
}

QGtkMenu::~QGtkMenu()
{
}

void QGtkMenu::regenerate()
{
    // ### handle popup menu case too
    Q_EMIT changed();
}

void QGtkMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    QGtkMenuItem *mi = static_cast<QGtkMenuItem*>(menuItem);
    QGtkMenuItem *bi = static_cast<QGtkMenuItem*>(bi);

    int idx = m_items.indexOf(bi);
    if (idx < 0)
        m_items.append(mi);
    else
        m_items.insert(idx, mi);

    connect(mi, &QGtkMenuItem::changed, this, &QGtkMenu::regenerate);
    regenerate();
}

void QGtkMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    QGtkMenuItem *mi = static_cast<QGtkMenuItem*>(menuItem);

    m_items.removeAll(mi);

    disconnect(mi, &QGtkMenuItem::changed, this, &QGtkMenu::regenerate);
    regenerate();
}

void QGtkMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{

    qWarning() << "Stub";
}

void QGtkMenu::syncSeparatorsCollapsible(bool enable)
{
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
    m_text = qt_convertToGtkMnemonics(text);
    regenerate();
}

void QGtkMenu::setIcon(const QIcon &icon)
{

    qWarning() << "Stub";
}

void QGtkMenu::setEnabled(bool enabled)
{
    m_enabled = enabled;
    regenerate();
}

bool QGtkMenu::isEnabled() const
{
    return m_enabled;
}

void QGtkMenu::setVisible(bool visible)
{
    //aboutToShow, aboutToHide signals
    qWarning() << "Stub";
}

void QGtkMenu::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem  *item)
{
    // ### this isn't called because of a Q_OS check in qtbase?
    GtkMenuItem *mi = gtkMenuItem();
    GtkWidget *menu = gtk_menu_item_get_submenu(mi);
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
    qWarning() << "Stub";
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

static void show_cb(GtkWidget *, gboolean, gpointer qgtkMenu)
{
    QGtkMenu *gm = static_cast<QGtkMenu*>(qgtkMenu);
    gm->emitShow();
}

static void hide_cb(GtkWidget *, gboolean, gpointer qgtkMenu)
{
    QGtkMenu *gm = static_cast<QGtkMenu*>(qgtkMenu);
    gm->emitHide();
}

GtkMenuItem *QGtkMenu::gtkMenuItem() const
{
    GtkMenu *menu = GTK_MENU(gtk_menu_new());

    for (const QGtkMenuItem *item : m_items) {
        GtkWidget *w = item->gtkMenuItem();
        if (!w)
            continue;
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(w));
    }

    GtkMenuItem *mi = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(m_text.toUtf8().constData()));
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), GTK_WIDGET(menu));
    gtk_widget_set_sensitive(GTK_WIDGET(mi), m_enabled);

    // not what we want; menu appears to be visible when it isn't opened (as
    // it's a widget whose parent is visible, maybe?)
    //g_signal_connect(mi, "show", G_CALLBACK(show_cb), const_cast<QGtkMenu*>(this));
    //g_signal_connect(mi, "hide", G_CALLBACK(hide_cb), const_cast<QGtkMenu*>(this));

    return mi;
}

void QGtkMenu::emitShow()
{
    Q_EMIT aboutToShow();
}

void QGtkMenu::emitHide()
{
    Q_EMIT aboutToHide();
}

