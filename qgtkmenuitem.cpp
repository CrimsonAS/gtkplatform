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
}

void QGtkMenuItem::setIcon(const QIcon &icon)
{
    Q_UNUSED(icon);
}

void QGtkMenuItem::setMenu(QPlatformMenu *pmenu)
{
    QGtkMenu *childMenu = static_cast<QGtkMenu*>(pmenu);
    m_childMenu = childMenu;
}

void QGtkMenuItem::setVisible(bool isVisible)
{
    m_visible = isVisible;
}

void QGtkMenuItem::setIsSeparator(bool isSeparator)
{
    m_isSeparator = isSeparator;
}

void QGtkMenuItem::setFont(const QFont &font)
{
    Q_UNUSED(font);
}

void QGtkMenuItem::setRole(MenuRole role)
{
    Q_UNUSED(role);
}

void QGtkMenuItem::setCheckable(bool checkable)
{
    m_checkable = checkable;
}

void QGtkMenuItem::setChecked(bool isChecked)
{
    m_checked = isChecked;
}

void QGtkMenuItem::setShortcut(const QKeySequence& shortcut)
{
    m_shortcut = shortcut;
}

void QGtkMenuItem::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void QGtkMenuItem::setIconSize(int size)
{
    Q_UNUSED(size);
}

void QGtkMenuItem::setNativeContents(WId item)
{
    Q_UNUSED(item);
}

void QGtkMenuItem::setHasExclusiveGroup(bool hasExclusiveGroup)
{
    m_hasExclusiveGroup = hasExclusiveGroup;
}

QGtkRefPtr<GtkWidget> QGtkMenuItem::gtkMenuItem() const
{
    return m_gtkMenuItem;
}

// ### keyvalToQtKey in reverse, ugh
// ### finish and move to helpers
static guint convertQtKeyToGdkKey(Qt::Key qKey)
{
    switch (qKey) {
    case Qt::Key_Delete:
        return GDK_KEY_Delete;
    case Qt::Key_Left:
        return GDK_KEY_Left;
    case Qt::Key_Right:
        return GDK_KEY_Right;
    case Qt::Key_Up:
        return GDK_KEY_Up;
    case Qt::Key_Down:
        return GDK_KEY_Down;
    case Qt::Key_Tab:
        return GDK_KEY_Tab;
    case Qt::Key_F1:
        return GDK_KEY_F1;
    case Qt::Key_F2:
        return GDK_KEY_F2;
    case Qt::Key_F3:
        return GDK_KEY_F3;
    case Qt::Key_F4:
        return GDK_KEY_F4;
    case Qt::Key_F5:
        return GDK_KEY_F5;
    case Qt::Key_F6:
        return GDK_KEY_F6;
    case Qt::Key_F7:
        return GDK_KEY_F7;
    case Qt::Key_F8:
        return GDK_KEY_F8;
    case Qt::Key_F9:
        return GDK_KEY_F9;
    case Qt::Key_F10:
        return GDK_KEY_F10;
    case Qt::Key_F11:
        return GDK_KEY_F11;
    case Qt::Key_F12:
        return GDK_KEY_F12;
    case Qt::Key_F13:
        return GDK_KEY_F13;
    case Qt::Key_F14:
        return GDK_KEY_F14;
    case Qt::Key_F15:
        return GDK_KEY_F15;
    case Qt::Key_F16:
        return GDK_KEY_F16;
    case Qt::Key_F17:
        return GDK_KEY_F17;
    case Qt::Key_F18:
        return GDK_KEY_F18;
    case Qt::Key_F19:
        return GDK_KEY_F19;
    case Qt::Key_F20:
        return GDK_KEY_F20;
    case Qt::Key_F21:
        return GDK_KEY_F21;
    }

    return (guint)qKey;
}

QGtkRefPtr<GtkWidget> QGtkMenuItem::sync()
{
    if (m_isSeparator) {
        m_gtkMenuItem = gtk_separator_menu_item_new();
    } else if (m_childMenu) {
        QGtkRefPtr<GtkMenuItem> mi = m_childMenu->gtkMenuItem();
        //g_signal_connect(mi, "select", G_CALLBACK(select_cb), const_cast<QGtkMenuItem*>(this));
        //g_signal_connect(mi, "activate", G_CALLBACK(activate_cb), const_cast<QGtkMenuItem*>(this));

        // stick our title on it
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(mi.get()));
        gtk_label_set_markup_with_mnemonic(GTK_LABEL(child), m_text.toUtf8().constData());
        gtk_widget_set_sensitive(GTK_WIDGET(mi.get()), m_enabled);
        m_gtkMenuItem = GTK_WIDGET(mi.get());
    } else {
        GtkWidget *mi = nullptr;
        if (m_checkable) {
            mi = gtk_check_menu_item_new_with_mnemonic(m_text.toUtf8().constData());
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), m_checked);
        } else {
            mi = gtk_menu_item_new_with_mnemonic(m_text.toUtf8().constData());
        }

        if (GTK_IS_CHECK_MENU_ITEM(mi)) {
            g_object_set(mi, "draw-as-radio", m_hasExclusiveGroup, NULL);
        }
        gtk_widget_set_sensitive(mi, m_enabled);
        g_signal_connect(mi, "select", G_CALLBACK(select_cb), const_cast<QGtkMenuItem*>(this));
        g_signal_connect(mi, "activate", G_CALLBACK(activate_cb), const_cast<QGtkMenuItem*>(this));
        GtkWidget *label = gtk_bin_get_child(GTK_BIN(mi));

        Qt::KeyboardModifiers qtMods = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;

        // Only attempt to map top level shortcuts. GTK+ accels only take a
        // single key, but QKeySequence can take multiple-- to overcome this
        // mismatch, we map the simple key sequences (and their modifiers), but
        // don't even attempt to try map more complex sequences.
        if (m_shortcut[1] == 0 && m_shortcut[2] == 0 && m_shortcut[3] == 0) {
            guint gKey = convertQtKeyToGdkKey(Qt::Key(m_shortcut[0] & ~qtMods));
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
        }

        m_gtkMenuItem = mi;
    }

    gtk_widget_set_visible(m_gtkMenuItem.get(), m_visible);

    return m_gtkMenuItem;
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


