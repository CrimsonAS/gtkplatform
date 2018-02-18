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
    Q_EMIT updated();
}

quintptr QGtkMenuItem::tag()const
{
    return m_tag;
}

void QGtkMenuItem::setText(const QString &text)
{
    m_text = qt_convertToGtkMnemonics(text);
    Q_EMIT updated();
}

void QGtkMenuItem::setIcon(const QIcon &icon)
{
    Q_UNUSED(icon);
}

void QGtkMenuItem::setMenu(QPlatformMenu *pmenu)
{
    QGtkMenu *childMenu = static_cast<QGtkMenu*>(pmenu);
    m_childMenu = childMenu;
    Q_EMIT updated();
}

void QGtkMenuItem::setVisible(bool isVisible)
{
    m_visible = isVisible;
    Q_EMIT updated();
}

void QGtkMenuItem::setIsSeparator(bool isSeparator)
{
    m_isSeparator = isSeparator;
    Q_EMIT updated();
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
    Q_EMIT updated();
}

void QGtkMenuItem::setChecked(bool isChecked)
{
    m_checked = isChecked;
    Q_EMIT updated();
}

void QGtkMenuItem::setShortcut(const QKeySequence& shortcut)
{
    m_shortcut = shortcut;
    Q_EMIT updated();
}

void QGtkMenuItem::setEnabled(bool enabled)
{
    m_enabled = enabled;
    Q_EMIT updated();
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
    Q_EMIT updated();
}

QGtkRefPtr<GtkWidget> QGtkMenuItem::gtkMenuItem() const
{
    QGtkRefPtr<GtkWidget> ret;
    if (m_isSeparator) {
        ret = gtk_separator_menu_item_new();
    } else if (m_childMenu) {
        QGtkRefPtr<GtkMenuItem> mi = m_childMenu->gtkMenuItem();
        //g_signal_connect(mi, "select", G_CALLBACK(select_cb), const_cast<QGtkMenuItem*>(this));
        //g_signal_connect(mi, "activate", G_CALLBACK(activate_cb), const_cast<QGtkMenuItem*>(this));

        // stick our title on it
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(mi.get()));
        gtk_label_set_markup_with_mnemonic(GTK_LABEL(child), m_text.toUtf8().constData());
        gtk_widget_set_sensitive(GTK_WIDGET(mi.get()), m_enabled);
        ret = GTK_WIDGET(mi.get());
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
            guint gKey = qt_convertToGdkKeyval(Qt::Key(m_shortcut[0] & ~qtMods));
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

        ret = mi;
    }

    gtk_widget_set_visible(ret.get(), m_visible);

    return ret;
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


