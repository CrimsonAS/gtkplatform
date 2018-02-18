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

#include "qgtkmenubar.h"
#include "qgtkmenu.h"
#include "qgtkmenuitem.h"
#include "qgtkwindow.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcMenuBar, "qt.qpa.gtk.menubar");

QGtkMenuBar::QGtkMenuBar()
{
    connect(this, &QGtkMenuBar::updated, this, &QGtkMenuBar::queueRegenerate);
}

QGtkMenuBar::~QGtkMenuBar()
{
}

void QGtkMenuBar::insertMenu(QPlatformMenu *menu, QPlatformMenu *before)
{
    QGtkMenu *m = static_cast<QGtkMenu*>(menu);
    QGtkMenu *b = static_cast<QGtkMenu*>(before);

    Q_ASSERT(m && !m_items.contains(m));
    Q_ASSERT(!b || m_items.contains(b));

    int idx = m_items.indexOf(b);
    qCDebug(lcMenuBar) << "Inserting menu " << m << idx;
    if (idx < 0) {
        m_items.append(m);
    } else {
        m_items.insert(idx, m);
    }

    connect(m, &QGtkMenu::updated, this, &QGtkMenuBar::queueRegenerate);
    syncMenu(menu);
    Q_EMIT updated();
}

void QGtkMenuBar::removeMenu(QPlatformMenu *menu)
{
    QGtkMenu *m = static_cast<QGtkMenu*>(menu);

    int idx = m_items.indexOf(m);
    Q_ASSERT(idx >= 0);
    qCDebug(lcMenuBar) << "Removing menu " << m_items.at(idx) << idx;
    m_items.removeAt(idx);
    m_items.removeAll(0); // if it was deleted, remove nulls too.

    disconnect(m, &QGtkMenu::updated, this, &QGtkMenuBar::queueRegenerate);
    Q_EMIT updated();
}

void QGtkMenuBar::syncMenu(QPlatformMenu *menuItem)
{
    QGtkMenu *menu = static_cast<QGtkMenu*>(menuItem);
    for (QGtkMenuItem *item : menu->items()) {
        if (item)
            menu->syncMenuItem(item);
    }
}

void QGtkMenuBar::queueRegenerate()
{
    if (m_regenerateQueued) {
        return;
    }

    QMetaObject::invokeMethod(this, "regenerate", Qt::QueuedConnection);
    m_regenerateQueued = true;
}

void QGtkMenuBar::regenerate()
{
    m_regenerateQueued = false;
    GtkContainer *omb = GTK_CONTAINER(m_menubar.get());
    GList *children = gtk_container_get_children(omb);
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        GtkWidget *menuChild = (GtkWidget*)iter->data;
        gtk_container_remove(omb, menuChild);
    }
    g_list_free(children);

    for (QGtkMenu *menu : m_items) {
        if (menu)
            gtk_menu_shell_append(GTK_MENU_SHELL(m_menubar.get()), GTK_WIDGET(menu->gtkMenuItem().get()));
    }
}

void QGtkMenuBar::handleReparent(QWindow *newParentWindow)
{
    QGtkRefPtr<GtkMenuBar> oldMenuBar = m_menubar;

    if (!newParentWindow) {
        m_menubar.reset(nullptr);
    } else {
        QGtkWindow *w = static_cast<QGtkWindow*>(newParentWindow->handle());
        if (!w) {
            // force creation of pwin
            newParentWindow->create();
        }
        w = static_cast<QGtkWindow*>(newParentWindow->handle());
        m_menubar = w->gtkMenuBar();
    }

    if (oldMenuBar) {
        GtkContainer *omb = GTK_CONTAINER(oldMenuBar.get());
        GtkContainer *nmb = GTK_CONTAINER(m_menubar.get());
        GList *children = gtk_container_get_children(omb);
        for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
            GtkWidget *menuChild = (GtkWidget*)iter->data;
            g_object_ref(menuChild); // temporaray ref, to save it past remove()
            gtk_container_remove(omb, menuChild);
            if (m_menubar.get()) {
                gtk_container_add(nmb, menuChild);
            }
            g_object_unref(menuChild);
        }
        g_list_free(children);
    }
}

QPlatformMenu *QGtkMenuBar::menuForTag(quintptr tag) const
{
    for (QGtkMenu *menu : qAsConst(m_items)) {
        if (menu && menu->tag() == tag) {
            return menu;
        }
    }

    return nullptr;
}

QPlatformMenu *QGtkMenuBar::createMenu() const
{
    return new QGtkMenu;
}

