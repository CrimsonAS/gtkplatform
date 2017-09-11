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
#include "qgtkmenu.h"

#include <QtCore/qdebug.h>

QGtkMenuItem::QGtkMenuItem()
    : m_tag((qintptr)this)
{
}

QGtkMenuItem::~QGtkMenuItem()
{
}


void QGtkMenuItem::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QGtkMenuItem::tag()const
{
    return m_tag;
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

void QGtkMenuItem::setMenu(QPlatformMenu *pmenu)
{
    if (m_childMenu) {
        disconnect(m_childMenu, &QGtkMenu::changed, this, &QGtkMenuItem::changed);
    }
    m_childMenu = static_cast<QGtkMenu*>(pmenu);
    if (m_childMenu) {
        connect(m_childMenu, &QGtkMenu::changed, this, &QGtkMenuItem::changed);
    }
    Q_EMIT changed();
}

void QGtkMenuItem::setVisible(bool isVisible)
{
    m_visible = isVisible;
    Q_EMIT changed();
}

void QGtkMenuItem::setIsSeparator(bool isSeparator)
{
    m_isSeparator = isSeparator;
    Q_EMIT changed();
}

void QGtkMenuItem::setFont(const QFont &font)
{
}

void QGtkMenuItem::setRole(MenuRole role)
{
}

void QGtkMenuItem::setCheckable(bool checkable)
{
    m_checkable = checkable;
    Q_EMIT changed();
}

void QGtkMenuItem::setChecked(bool isChecked)
{
    m_checked = isChecked;
    Q_EMIT changed();
}

void QGtkMenuItem::setShortcut(const QKeySequence& shortcut)
{
    m_shortcut = shortcut;
    Q_EMIT changed();
}

void QGtkMenuItem::setEnabled(bool enabled)
{
    m_enabled = enabled;
    Q_EMIT changed();
}

void QGtkMenuItem::setIconSize(int size)
{
    qWarning() << "Stub";
}

void QGtkMenuItem::setNativeContents(WId item)
{
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

GtkWidget *QGtkMenuItem::gtkMenuItem() const
{
    if (!m_visible) {
        return 0;
    }

    if (m_isSeparator) {
        GtkWidget *sep = gtk_separator_menu_item_new();
        return sep;
    }

    if (m_childMenu) {
        GtkMenuItem *mi = m_childMenu->gtkMenuItem();
        g_signal_connect(mi, "select", G_CALLBACK(select_cb), const_cast<QGtkMenuItem*>(this));
        g_signal_connect(mi, "activate", G_CALLBACK(activate_cb), const_cast<QGtkMenuItem*>(this));

        // stick our title on it
        GtkWidget *child = gtk_bin_get_child (GTK_BIN (mi));
        gtk_label_set_markup_with_mnemonic(GTK_LABEL(child), m_text.toUtf8().constData());
        gtk_widget_set_sensitive(GTK_WIDGET(mi), m_enabled);
        return GTK_WIDGET(mi);
    }

    GtkWidget *mi = nullptr;
    if (m_checkable) {
        mi = gtk_check_menu_item_new_with_mnemonic(m_text.toUtf8().constData());
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), m_checked);
    } else {
        mi = gtk_menu_item_new_with_mnemonic(m_text.toUtf8().constData());
    }

    gtk_widget_set_sensitive(mi, m_enabled);
    g_signal_connect(mi, "select", G_CALLBACK(select_cb), const_cast<QGtkMenuItem*>(this));
    g_signal_connect(mi, "activate", G_CALLBACK(activate_cb), const_cast<QGtkMenuItem*>(this));
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(mi));

    Qt::KeyboardModifiers qtMods = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;

    // ### what about the other keys?
    guint gKey = m_shortcut[0] & ~qtMods;
    guint gModifiers = 0;

    if (m_shortcut[0] & Qt::ShiftModifier) {
        gModifiers |= GDK_SHIFT_MASK;
    }
    if (m_shortcut[0] & Qt::ControlModifier) {
        gModifiers |= GDK_CONTROL_MASK;
    }
    if (m_shortcut[0] & Qt::AltModifier) {
        gModifiers |= GDK_MOD1_MASK;
    }
    if (m_shortcut[0] & Qt::MetaModifier) {
        gModifiers |= GDK_META_MASK;
    }

    gtk_accel_label_set_accel(GTK_ACCEL_LABEL(label), gKey, GdkModifierType(gModifiers));

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


