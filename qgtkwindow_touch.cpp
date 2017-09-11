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
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcTouch, "qt.qpa.gtk.touch");
Q_LOGGING_CATEGORY(lcTouchUpdate, "qt.qpa.gtk.touch.update");

static Qt::TouchPointState gtkTouchStateToQtTouchState(GdkEventType type)
{
    switch (type) {
    case GDK_TOUCH_BEGIN:
        return Qt::TouchPointPressed;
    case GDK_TOUCH_UPDATE:
        return Qt::TouchPointMoved;
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
        return Qt::TouchPointReleased;
    default:
        Q_UNREACHABLE();
    }
}

bool QGtkWindow::onTouchEvent(GdkEvent *event)
{
    GdkEventTouch *ev = (GdkEventTouch*)event;

    QWindowSystemInterface::TouchPoint *tp = 0;
    int touchpointId = int(reinterpret_cast<intptr_t>(ev->sequence));

    switch (ev->type) {
    case GDK_TOUCH_BEGIN:
        qCDebug(lcTouch) << "Begin " << touchpointId;
        m_activeTouchPoints.append(QWindowSystemInterface::TouchPoint());
        tp = &m_activeTouchPoints.last();
        tp->id = touchpointId;
        break;
    case GDK_TOUCH_UPDATE:
        qCDebug(lcTouchUpdate) << "Update " << touchpointId;
        for (int i = 0; i < m_activeTouchPoints.length(); ++i) {
            if (m_activeTouchPoints.at(i).id == touchpointId) {
                tp = &m_activeTouchPoints[i];
                break;
            }
        }
        break;
    case GDK_TOUCH_END:
        qCDebug(lcTouch) << "End " << touchpointId;
        for (int i = 0; i < m_activeTouchPoints.length(); ++i) {
            if (m_activeTouchPoints.at(i).id == touchpointId) {
                tp = &m_activeTouchPoints[i];
                break;
            }
        }
        break;
    case GDK_TOUCH_CANCEL:
        qCDebug(lcTouch) << "Cancel " << touchpointId;
        for (int i = 0; i < m_activeTouchPoints.length(); ++i) {
            if (m_activeTouchPoints.at(i).id == touchpointId) {
                tp = &m_activeTouchPoints[i];
                break;
            }
        }
        break;
    default:
        qWarning() << "Unknown touch type" << ev->type;
        return false;
    }

    // ### it's unfortunate that we have to report touch events so often.
    // perhaps we should tie the report of these into the frameclock so they are
    // only sent once per frame? or ask gtk for a 'touch frame' event?

    if (tp) {
        // Update it
        tp->pressure = 1.0; // ### should be able to read this somehow
        tp->state = gtkTouchStateToQtTouchState(ev->type);

        // ### touchpoint size?
        // the area is supposed to be centered on the point, hence the - 0.5
        // subtractions (as we're reporting a size of 1).
        QRectF tpArea = QRectF(ev->x - 0.5, ev->y - 0.5, 1, 1);

        // make sure it really moved...
        if (tp->state == Qt::TouchPointMoved) {
            if (tp->area == tpArea) {
                tp->state = Qt::TouchPointStationary;
            }
        }

        tp->area = tpArea;

        QScreen *s = window()->screen();
        qreal nx = ev->x / (window()->screen()->size().width() / 2);
        qreal ny = ev->y / (window()->screen()->size().height() / 2);
        tp->normalPosition = QPointF(nx, ny);
    }

    // report unconditionally even if tp was not found
    // (in that case there was a release event)
    QWindowSystemInterface::handleTouchEvent(
        window(),
        ev->time,
        m_touchDevice,
        m_activeTouchPoints,
        convertGdkKeyboardModsToQtKeyboardMods(ev->state)
    );

    switch (ev->type) {
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
        for (int i = 0; i < m_activeTouchPoints.length(); ++i) {
            if (m_activeTouchPoints.at(i).id == touchpointId) {
                m_activeTouchPoints.removeAt(i);
                break;
            }
        }
        break;
    default:
        break;
    }

    // Eat the event so that GTK doesn't synthesize pointer events from these.
    return true;
}


