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

#include "qgtkmenubar.h"
#include "qgtkmenu.h"
#include "qgtkmenuitem.h"
#include "qgtkwindow.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcMenuBar, "qt.qpa.gtk.menubar");

QGtkMenuBar::QGtkMenuBar()
{
}

QGtkMenuBar::~QGtkMenuBar()
{
}

void QGtkMenuBar::insertMenu(QPlatformMenu *menu, QPlatformMenu *before)
{
    QGtkMenu *m = static_cast<QGtkMenu*>(menu);
    QGtkMenu *b = static_cast<QGtkMenu*>(before);

    if (m_items.indexOf(m) != -1) {
        removeMenu(menu);
    }

    int idx = m_items.indexOf(b);
    QGtkRefPtr<GtkMenuItem> mi = m->gtkMenuItem();
    qCDebug(lcMenuBar) << "Inserting menu " << m << mi << idx;
    if (idx < 0) {
        m_items.append(m);
        m_gtkItems.append(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(m_menubar.get()), GTK_WIDGET(mi.get()));
    } else {
        m_items.insert(idx, m);
        m_gtkItems.insert(idx, mi);
        gtk_menu_shell_insert(GTK_MENU_SHELL(m_menubar.get()), GTK_WIDGET(mi.get()), idx);
    }
}

void QGtkMenuBar::removeMenu(QPlatformMenu *menu)
{
    QGtkMenu *m = static_cast<QGtkMenu*>(menu);

    int idx = m_items.indexOf(m);
    qCDebug(lcMenuBar) << "Removing menu " << m_items.at(idx) << m_gtkItems.at(idx) << idx;
    m_items.removeAt(idx);

    gtk_container_remove(GTK_CONTAINER(m_menubar.get()), GTK_WIDGET(m_gtkItems.takeAt(idx).get()));
}

void QGtkMenuBar::syncMenu(QPlatformMenu *menuItem)
{
    Q_UNUSED(menuItem);
    qWarning() << "Stub";
}

void QGtkMenuBar::handleReparent(QWindow *newParentWindow)
{
    QGtkRefPtr<GtkMenuBar> oldMenuBar = m_menubar;

    if (!newParentWindow) {
        m_menubar.reset(nullptr);
    } else {
        QGtkWindow *w = static_cast<QGtkWindow*>(newParentWindow->handle());
        m_menubar = w->gtkMenuBar();
    }

    if (oldMenuBar) {
        GtkContainer *omb = GTK_CONTAINER(oldMenuBar.get());
        GtkContainer *nmb = GTK_CONTAINER(m_menubar.get());
        GList *children = gtk_container_get_children(omb);
        for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
            GtkWidget *menuChild = (GtkWidget*)iter->data;
            g_object_ref(menuChild); // temporaray ref, to save it past remove()
            gtk_container_remove(nmb, menuChild);
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
    Q_UNUSED(tag);
    qWarning() << "Stub";
    return 0;
}

QPlatformMenu *QGtkMenuBar::createMenu() const
{
    return new QGtkMenu;
}

