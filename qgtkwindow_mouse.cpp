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
#include "qgtkhelpers.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>

#include "CSystrace.h"

Q_LOGGING_CATEGORY(lcMouse, "qt.qpa.gtk.mouse");
Q_LOGGING_CATEGORY(lcMouseMotion, "qt.qpa.gtk.mouse.motion");

bool QGtkWindow::onButtonPress(GdkEvent *event)
{
    TRACE_EVENT0("input", "QGtkWindow::onButtonPress");
    TRACE_EVENT_ASYNC_BEGIN0("input", "QGtkWindow::mouseDown", this);
    GdkEventButton *ev = (GdkEventButton*)event;

    // ### would be nice if we could support GDK_2BUTTON_PRESS/GDK_3BUTTON_PRESS
    // directly (and not via emulation internally).

    Qt::MouseButton b = qt_convertGButtonToQButton(ev->button);
    m_buttons |= b;
    qCDebug(lcMouse) << "Pressed " << b << " at " << ev->x << ev->y << ev->x_root << ev->y_root << " total pressed " << m_buttons;

    bool isTabletEvent = false;
    QWindowSystemInterface::handleMouseEvent(
        window(),
        ev->time,
        QPointF(ev->x, ev->y),
        QPointF(ev->x_root, ev->y_root), // ### _root is probably wrong.
        m_buttons,
        qt_convertToQtKeyboardMods(ev->state),
        isTabletEvent ? Qt::MouseEventSynthesizedByQt : Qt::MouseEventNotSynthesized
    );
    return true;
}

bool QGtkWindow::onButtonRelease(GdkEvent *event)
{
    TRACE_EVENT0("input", "QGtkWindow::onButtonRelease");
    GdkEventButton *ev = (GdkEventButton*)event;

    Qt::MouseButton b = qt_convertGButtonToQButton(ev->button);
    m_buttons &= ~b;
    qCDebug(lcMouse) << "Released " << b << " at " << ev->x << ev->y << ev->x_root << ev->y_root << " total pressed " << m_buttons;

    bool isTabletEvent = false;
    QWindowSystemInterface::handleMouseEvent(
        window(),
        ev->time,
        QPointF(ev->x, ev->y),
        QPointF(ev->x_root, ev->y_root),
        m_buttons,
        qt_convertToQtKeyboardMods(ev->state),
        isTabletEvent ? Qt::MouseEventSynthesizedByQt : Qt::MouseEventNotSynthesized
    );
    TRACE_EVENT_ASYNC_END0("input", "QGtkWindow::mouseDown", this);
    return true;
}

bool QGtkWindow::onMotionNotify(GdkEvent *event)
{
    TRACE_EVENT0("input", "QGtkWindow::onMotionNotify");
    GdkEventButton *ev = (GdkEventButton*)event;
    qCDebug(lcMouseMotion) << "Moved mouse at " << ev->x << ev->y << ev->x_root << ev->y_root;

    bool isTabletEvent = false;
    QWindowSystemInterface::handleMouseEvent(
        window(),
        ev->time,
        QPointF(ev->x, ev->y),
        QPointF(ev->x_root, ev->y_root),
        m_buttons,
        qt_convertToQtKeyboardMods(ev->state),
        isTabletEvent ? Qt::MouseEventSynthesizedByQt : Qt::MouseEventNotSynthesized
    );
    return true;
}

bool QGtkWindow::onScrollEvent(GdkEvent *event)
{
    TRACE_EVENT0("input", "QGtkWindow::onScrollEvent");
    GdkEventScroll *ev = (GdkEventScroll*)event;

    QPoint angleDelta;
    QPoint pixelDelta;
    Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;

    // We cache the modifiers as they should not change after the scroll has
    // started. Doing that means that you'll zoom text in Creator or something,
    // which is pretty annoying.
    if (!m_scrollStarted) {
        m_scrollStarted = true;
        m_scrollModifiers = qt_convertToQtKeyboardMods(ev->state);
    }

    if (gdk_event_is_scroll_stop_event(event)) {
        m_scrollStarted = false;
        m_scrollModifiers = Qt::NoModifier;
    }

    if (ev->direction == GDK_SCROLL_SMOOTH) {
        // ### I have literally no idea what I'm doing here
        const int pixelsToDegrees = 50;
        angleDelta.setX(-ev->delta_x * pixelsToDegrees);
        angleDelta.setY(-ev->delta_y * pixelsToDegrees);
        source = Qt::MouseEventSynthesizedBySystem;

        pixelDelta.setX(ev->delta_x * pixelsToDegrees);
        pixelDelta.setY(-ev->delta_y * pixelsToDegrees);
    } else if (ev->direction == GDK_SCROLL_UP ||
               ev->direction == GDK_SCROLL_DOWN) {
        angleDelta.setY(qBound(-120, int(ev->delta_y * 10000), 120));
    } else if (ev->direction == GDK_SCROLL_LEFT ||
               ev->direction == GDK_SCROLL_RIGHT) {
        angleDelta.setX(qBound(-120, int(ev->delta_x * 10000), 120));
    } else {
        Q_UNREACHABLE();
    }

    qCDebug(lcMouseMotion) << "Scrolled mouse at " << ev->x << ev->y << ev->x_root << ev->y_root << " angle delta " << angleDelta << " pixelDelta " << pixelDelta << " original deltas " << ev->delta_x << ev->delta_y;

    QWindowSystemInterface::handleWheelEvent(
        window(),
        ev->time,
        QPointF(ev->x, ev->y),
        QPointF(ev->x_root, ev->y_root),
        pixelDelta,
        angleDelta,
        m_scrollModifiers,
        Qt::NoScrollPhase,
        source,
        false /* isInverted */
    );
    return true;
}


