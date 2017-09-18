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

#include "qgtkwindow.h"
#include "qgtkhelpers.h"

#include <qpa/qwindowsysteminterface.h>

#include <QtCore/qdebug.h>

#include "CSystrace.h"

bool QGtkWindow::onKeyPress(GdkEvent *event)
{
    GdkEventKey *ev = (GdkEventKey*)event;
    TRACE_EVENT_ASYNC_BEGIN0("input", "QGtkWindow::keyDown", (void*)ev->hardware_keycode);
    TRACE_EVENT0("input", "QGtkWindow::onKeyPress");

    const QString text = QString::fromUtf8(ev->string, ev->length);
    Qt::KeyboardModifiers qtMods = qt_convertToQtKeyboardMods(ev->state);
    Qt::Key qtKey = qt_convertToQtKey(ev->keyval);

    return QWindowSystemInterface::handleExtendedKeyEvent(
        window(),
        ev->time,
        QEvent::KeyPress,
        qtKey,
        qtMods,
        ev->hardware_keycode,
        ev->hardware_keycode,
        0,
        text
    );
}

bool QGtkWindow::onKeyRelease(GdkEvent *event)
{
    GdkEventKey *ev = (GdkEventKey*)event;
    TRACE_EVENT_ASYNC_END0("input", "QGtkWindow::keyDown", (void*)ev->hardware_keycode);
    TRACE_EVENT0("input", "QGtkWindow::onKeyRelease");

    const QString text = QString::fromUtf8(ev->string, ev->length);
    Qt::KeyboardModifiers qtMods = qt_convertToQtKeyboardMods(ev->state);
    Qt::Key qtKey = qt_convertToQtKey(ev->keyval);

    return QWindowSystemInterface::handleExtendedKeyEvent(
        window(),
        ev->time,
        QEvent::KeyRelease,
        qtKey,
        qtMods,
        ev->hardware_keycode,
        ev->hardware_keycode,
        0,
        text
    );
}


