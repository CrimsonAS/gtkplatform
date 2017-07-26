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

#include <qpa/qwindowsysteminterface.h>

#include <QDebug>

Qt::KeyboardModifiers QGtkWindow::convertGdkKeyboardModsToQtKeyboardMods(guint mask)
{
    Qt::KeyboardModifiers mods = Qt::NoModifier;

    if (mask & GDK_SHIFT_MASK)
        mods |= Qt::ShiftModifier;
    if (mask & GDK_CONTROL_MASK)
        mods |= Qt::ControlModifier;
    if (mask & GDK_MOD1_MASK)
        mods |= Qt::AltModifier;
    if (mask & GDK_META_MASK)
        mods |= Qt::MetaModifier;
#if 0
    if (mask & GDK_SUPER_MASK)
        qDebug() << "Super";
    if (mask & GDK_HYPER_MASK)
        qDebug() << "Hyper";
    if (mask & GDK_MOD2_MASK)
        qDebug() << "Mod2";
    if (mask & GDK_MOD3_MASK)
        qDebug() << "Mod3";
    if (mask & GDK_MOD4_MASK)
        qDebug() << "Mod4";
    if (mask & GDK_MOD5_MASK)
        qDebug() << "Mod5";
#endif
    return mods;
}

bool QGtkWindow::onKeyPress(GdkEvent *event)
{
    GdkEventKey *ev = (GdkEventKey*)event;

    // ### doc says this is deprecated...
    const QString text = QString::fromUtf8(ev->string, ev->length);

    return QWindowSystemInterface::handleExtendedKeyEvent(
        window(),
        ev->time,
        QEvent::KeyPress,
        ev->keyval,
        Qt::KeyboardModifiers(0), // ###
        ev->hardware_keycode,
        ev->hardware_keycode,
        0,
        text
    );
}

bool QGtkWindow::onKeyRelease(GdkEvent *event)
{
    GdkEventKey *ev = (GdkEventKey*)event;

    // ### doc says this is deprecated...
    const QString text = QString::fromUtf8(ev->string, ev->length);

    return QWindowSystemInterface::handleExtendedKeyEvent(
        window(),
        ev->time,
        QEvent::KeyRelease,
        ev->keyval,
        Qt::KeyboardModifiers(0), // ###
        ev->hardware_keycode,
        ev->hardware_keycode,
        0,
        text
    );
}


