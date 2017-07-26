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

QGtkMenuBar::QGtkMenuBar()
    : m_menubar(0)
{
}

QGtkMenuBar::~QGtkMenuBar()
{
}

void QGtkMenuBar::regenerate()
{
    if (!m_menubar) {
        return;
    }
    // ### this is a bit lazy... we shouldn't recreate menus all the time
    GList *children = gtk_container_get_children(GTK_CONTAINER(m_menubar));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    for (int i = 0; i < m_items.count(); ++i) {
        GtkMenuItem *it = m_items.at(i)->gtkMenuItem();
        qDebug() << it;
        if (!it)
            continue;
        gtk_menu_shell_append(GTK_MENU_SHELL(m_menubar), GTK_WIDGET(it));
    }
}

void QGtkMenuBar::insertMenu(QPlatformMenu *menu, QPlatformMenu *before)
{
    QGtkMenu *m = static_cast<QGtkMenu*>(menu);
    QGtkMenu *b = static_cast<QGtkMenu*>(before);

    int idx = m_items.indexOf(b);
    if (idx < 0)
        m_items.append(m);
    else
        m_items.insert(idx, m);

    connect(m, &QGtkMenu::changed, this, &QGtkMenuBar::regenerate);
    regenerate();
}

void QGtkMenuBar::removeMenu(QPlatformMenu *menu)
{
    QGtkMenu *m = static_cast<QGtkMenu*>(menu);

    m_items.removeAll(m);

    disconnect(m, &QGtkMenu::changed, this, &QGtkMenuBar::regenerate);
    regenerate();
}

void QGtkMenuBar::syncMenu(QPlatformMenu *menuItem)
{ qWarning() << "Stub"; }

void QGtkMenuBar::handleReparent(QWindow *newParentWindow)
{
    if (!newParentWindow) {
        m_menubar = 0;
        return;
    }

    // ### remove old menu contents if we are moved
    QGtkWindow *w = static_cast<QGtkWindow*>(newParentWindow->handle());
    m_menubar = w->gtkMenuBar();
    regenerate();
    qWarning() << "Reparented to " << newParentWindow;


/*
  GtkWidget *fileMenu = gtk_menu_new();

  GtkWidget *fileMi = gtk_menu_item_new_with_label("File");
  GtkWidget *quitMi = gtk_menu_item_new_with_label("Quit");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menubar), fileMi);
  */
}

QPlatformMenu *QGtkMenuBar::menuForTag(quintptr tag) const
{ qWarning() << "Stub"; }

QPlatformMenu *QGtkMenuBar::createMenu() const
{
    return new QGtkMenu;
}

