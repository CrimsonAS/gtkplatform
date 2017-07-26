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

static Qt::MouseButton convert_g_button_to_q_button(guint button)
{
    Qt::MouseButton b = Qt::NoButton;
    switch (button) {
    case 1:
        b = Qt::LeftButton;
        break;
    case 2:
        b = Qt::RightButton;
        break;
    case 3:
        b = Qt::MiddleButton;
        break;
    case 4:
        b = Qt::ExtraButton1;
        break;
    case 5:
        b = Qt::ExtraButton2;
        break;
    case 6:
        b = Qt::ExtraButton3;
        break;
    case 7:
        b = Qt::ExtraButton4;
        break;
    case 8:
        b = Qt::ExtraButton5;
        break;
    case 9:
        b = Qt::ExtraButton6;
        break;
    case 10:
        b = Qt::ExtraButton7;
        break;
    case 11:
        b = Qt::ExtraButton8;
        break;
    case 12:
        b = Qt::ExtraButton9;
        break;
    case 13:
        b = Qt::ExtraButton10;
        break;
    case 14:
        b = Qt::ExtraButton11;
        break;
    case 15:
        b = Qt::ExtraButton12;
        break;
    case 16:
        b = Qt::ExtraButton13;
        break;
    case 17:
        b = Qt::ExtraButton14;
        break;
    case 18:
        b = Qt::ExtraButton15;
        break;
    case 19:
        b = Qt::ExtraButton16;
        break;
    case 20:
        b = Qt::ExtraButton17;
        break;
    case 21:
        b = Qt::ExtraButton18;
        break;
    case 22:
        b = Qt::ExtraButton19;
        break;
    case 23:
        b = Qt::ExtraButton20;
        break;
    case 24:
        b = Qt::ExtraButton21;
        break;
    case 25:
        b = Qt::ExtraButton22;
        break;
    case 26:
        b = Qt::ExtraButton23;
        break;
    case 27:
        b = Qt::ExtraButton24;
        break;
    default:
        qWarning() << "Unrecognized button" << button;
    }

    return b;
}

bool QGtkWindow::onButtonPress(GdkEvent *event)
{
    GdkEventButton *ev = (GdkEventButton*)event;

    // ### would be nice if we could support GDK_2BUTTON_PRESS/GDK_3BUTTON_PRESS
    // directly (and not via emulation internally).

    Qt::MouseButton b = convert_g_button_to_q_button(ev->button);
    m_buttons |= b;

    bool isTabletEvent = false;
    QWindowSystemInterface::handleMouseEvent(
        window(),
        ev->time,
        QPointF(ev->x, ev->y),
        QPointF(ev->x_root, ev->y_root), // ### _root is probably wrong.
        m_buttons,
        QGtkWindow::convertGdkKeyboardModsToQtKeyboardMods(ev->state),
        isTabletEvent ? Qt::MouseEventSynthesizedByQt : Qt::MouseEventNotSynthesized
    );
}

bool QGtkWindow::onButtonRelease(GdkEvent *event)
{
    GdkEventButton *ev = (GdkEventButton*)event;

    Qt::MouseButton b = convert_g_button_to_q_button(ev->button);
    m_buttons &= ~b;

    bool isTabletEvent = false;
    QWindowSystemInterface::handleMouseEvent(
        window(),
        ev->time,
        QPointF(ev->x, ev->y),
        QPointF(ev->x_root, ev->y_root),
        m_buttons,
        QGtkWindow::convertGdkKeyboardModsToQtKeyboardMods(ev->state),
        isTabletEvent ? Qt::MouseEventSynthesizedByQt : Qt::MouseEventNotSynthesized
    );
}

bool QGtkWindow::onMotionNotify(GdkEvent *event)
{
    GdkEventButton *ev = (GdkEventButton*)event;

    bool isTabletEvent = false;
    QWindowSystemInterface::handleMouseEvent(
        window(),
        ev->time,
        QPointF(ev->x, ev->y),
        QPointF(ev->x_root, ev->y_root),
        m_buttons,
        QGtkWindow::convertGdkKeyboardModsToQtKeyboardMods(ev->state),
        isTabletEvent ? Qt::MouseEventSynthesizedByQt : Qt::MouseEventNotSynthesized
    );
}

bool QGtkWindow::onScrollEvent(GdkEvent *event)
{
    GdkEventScroll *ev = (GdkEventScroll*)event;

    QPoint angleDelta;
    QPoint pixelDelta;
    Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;

    if (ev->direction == GDK_SCROLL_SMOOTH) {
        // ### I have literally no idea what I'm doing here
        const int pixelsToDegrees = 50;
        angleDelta.setX(ev->delta_x * pixelsToDegrees);
        angleDelta.setY(ev->delta_y * pixelsToDegrees);
        source = Qt::MouseEventSynthesizedBySystem;

        pixelDelta.setX(ev->delta_x * pixelsToDegrees);
        pixelDelta.setY(ev->delta_y * pixelsToDegrees);
    } else if (ev->direction == GDK_SCROLL_UP ||
               ev->direction == GDK_SCROLL_DOWN) {
        angleDelta.setY(qBound(-120, int(ev->delta_y * 10000), 120));
    } else if (ev->direction == GDK_SCROLL_LEFT ||
               ev->direction == GDK_SCROLL_RIGHT) {
        angleDelta.setX(qBound(-120, int(ev->delta_x * 10000), 120));
    } else {
        Q_UNREACHABLE();
    }


    qDebug() << "Scroll " << ev->x << ev->y << pixelDelta << angleDelta;
    QWindowSystemInterface::handleWheelEvent(
        window(),
        ev->time,
        QPointF(ev->x, ev->y),
        QPointF(ev->x_root, ev->y_root),
        pixelDelta,
        angleDelta,
        QGtkWindow::convertGdkKeyboardModsToQtKeyboardMods(ev->state),
        Qt::NoScrollPhase,
        source,
        false /* isInverted */
    );
}


