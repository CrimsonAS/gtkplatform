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

#include "qgtkmenuitem.h"
#include "qgtkhelpers.h"

#include <QtCore/qdebug.h>

QGtkMenuItem::QGtkMenuItem()
{
}

QGtkMenuItem::~QGtkMenuItem()
{
}


void QGtkMenuItem::setTag(quintptr tag)
{

    qWarning() << "Stub";
}

quintptr QGtkMenuItem::tag()const
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setText(const QString &text)
{
    m_text = qt_convertToGtkMnemonics(text);
    Q_EMIT changed();
}

void QGtkMenuItem::setIcon(const QIcon &icon)
{
    //GIcon *ico = q_iconToIcon(icon);
    //g_menu_item_set_icon(m_menuItem, ico);
    //g_object_unref(ico);
    //Q_EMIT changed();
}

// Turn this item into a menu container
void QGtkMenuItem::setMenu(QPlatformMenu *pmenu)
{
    qWarning() << "Stub";
}

void QGtkMenuItem::setVisible(bool isVisible)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setIsSeparator(bool isSeparator)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setFont(const QFont &font)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setRole(MenuRole role)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setCheckable(bool checkable)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setChecked(bool isChecked)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setShortcut(const QKeySequence& shortcut)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setEnabled(bool enabled)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setIconSize(int size)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setNativeContents(WId item)
{

    qWarning() << "Stub";
}

void QGtkMenuItem::setHasExclusiveGroup(bool hasExclusiveGroup)
{

    qWarning() << "Stub";
}

static void select_cb(GtkMenuItem *, gpointer qgtkMenuItem)
{
    QGtkMenuItem *gm = static_cast<QGtkMenuItem*>(qgtkMenuItem);
    gm->emitSelect();
}

static void activate_cb(GtkMenuItem *, gpointer qgtkMenuItem)
{
    QGtkMenuItem *gm = static_cast<QGtkMenuItem*>(qgtkMenuItem);
    gm->emitActivate();
}

GtkMenuItem *QGtkMenuItem::gtkMenuItem() const
{
    GtkMenuItem *mi = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(m_text.toUtf8().constData()));
    g_signal_connect(mi, "select", G_CALLBACK(select_cb), const_cast<QGtkMenuItem*>(this));
    g_signal_connect(mi, "activate", G_CALLBACK(activate_cb), const_cast<QGtkMenuItem*>(this));
    return mi;
}

void QGtkMenuItem::emitSelect()
{
    // ### right?
    Q_EMIT hovered();
}

void QGtkMenuItem::emitActivate()
{
    Q_EMIT activated();
}


