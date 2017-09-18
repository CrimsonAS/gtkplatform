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
#include <QtCore/qloggingcategory.h>

#include "CSystrace.h"

Q_LOGGING_CATEGORY(lcTouch, "qt.qpa.gtk.touch");
Q_LOGGING_CATEGORY(lcTouchUpdate, "qt.qpa.gtk.touch.update");

bool QGtkWindow::onTouchEvent(GdkEvent *event)
{
    TRACE_EVENT0("input", "QGtkWindow::onTouchEvent");
    GdkEventTouch *ev = (GdkEventTouch*)event;

    QWindowSystemInterface::TouchPoint *tp = 0;
    int touchpointId = int(reinterpret_cast<intptr_t>(ev->sequence));

    switch (ev->type) {
    case GDK_TOUCH_BEGIN:
        TRACE_EVENT_ASYNC_BEGIN0("input", "QGtkWindow::touchDown", (void*)touchpointId);
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
        tp->state = qt_convertToQtTouchPointState(ev->type);

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

        QSize s = window()->screen()->size();
        qreal nx = ev->x / (s.width() / 2);
        qreal ny = ev->y / (s.height() / 2);
        tp->normalPosition = QPointF(nx, ny);
    }

    // report unconditionally even if tp was not found
    // (in that case there was a release event)
    QWindowSystemInterface::handleTouchEvent(
        window(),
        ev->time,
        m_touchDevice,
        m_activeTouchPoints,
        qt_convertToQtKeyboardMods(ev->state)
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
        TRACE_EVENT_ASYNC_END0("input", "QGtkWindow::touchDown", (void*)touchpointId);
        break;
    default:
        break;
    }

    // Eat the event so that GTK doesn't synthesize pointer events from these.
    return true;
}


