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

#include <math.h> // M_PI

#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcGesture, "qt.qpa.gtk.gesture");


static void populateTsAndPoint(GtkGesture *gesture, guint32 &ts, QPointF &contentPoint)
{
    gdouble x;
    gdouble y;

    GdkEventSequence *seq = gtk_gesture_get_last_updated_sequence(gesture);
    gtk_gesture_get_point(gesture, seq, &x, &y);
    contentPoint = QPointF(x, y);
    const GdkEvent *ev = gtk_gesture_get_last_event(gesture, seq);
    ts = gdk_event_get_time(ev);
}

void QGtkWindow::zoom_cb(GtkGestureZoom *pt, gdouble scale, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    guint32 ts;
    QPointF contentPoint;
    populateTsAndPoint(GTK_GESTURE(pt), ts, contentPoint);
    GdkEventSequence *seq = gtk_gesture_get_last_updated_sequence(GTK_GESTURE(pt));
    gtk_gesture_set_sequence_state(GTK_GESTURE(pt), seq, GTK_EVENT_SEQUENCE_CLAIMED); // ### not really sure this should be done here, but crashes in begin

    pw->zoom(contentPoint, scale, ts);
}

void QGtkWindow::begin_zoom_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    guint32 ts;
    QPointF contentPoint;
    populateTsAndPoint(GTK_GESTURE(pt), ts, contentPoint);

    qCDebug(lcGesture) << "Begin zoom " << pw->window() << ts << contentPoint;
    pw->beginZoom(contentPoint, ts);
}

void QGtkWindow::end_zoom_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    guint32 ts;
    QPointF contentPoint;
    populateTsAndPoint(GTK_GESTURE(pt), ts, contentPoint);

    qCDebug(lcGesture) << "End zoom " << pw->window() << ts << contentPoint;
    pw->endZoom(contentPoint, ts);
}

void QGtkWindow::cancel_zoom_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    guint32 ts;
    QPointF contentPoint;
    populateTsAndPoint(GTK_GESTURE(pt), ts, contentPoint);

    qCDebug(lcGesture) << "Cancel zoom " << pw->window() << ts << contentPoint;
    pw->endZoom(contentPoint, ts);
}

void QGtkWindow::beginZoom(QPointF &contentPoint, guint32 ts)
{
    m_initialZoomSet = false;
    if (m_activeNativeGestures++ == 0) {
        qCDebug(lcGesture) << "Started native gesture sequence (due to zoom)";
        QWindowSystemInterface::handleGestureEvent(window(), ts, Qt::BeginNativeGesture, contentPoint, contentPoint);
    }
}

void QGtkWindow::zoom(QPointF &contentPoint, double scale, guint32 ts)
{
    if  (scale == 0.0) {
        return; // insane
    }

    if (!m_initialZoomSet) {
        m_initialZoomSet = true;
        m_initialZoom = scale;
    }
    double modScale = (scale - m_initialZoom) / m_initialZoom;
    m_initialZoom = scale;
    QWindowSystemInterface::handleGestureEventWithRealValue(window(), ts, Qt::ZoomNativeGesture, modScale, contentPoint, contentPoint);
}

void QGtkWindow::endZoom(QPointF &contentPoint, guint32 ts)
{
    if (--m_activeNativeGestures == 0) {
        qCDebug(lcGesture) << "Ended native gesture sequence (due to zoom)";
        QWindowSystemInterface::handleGestureEvent(window(), ts, Qt::EndNativeGesture, contentPoint, contentPoint);
    }
}

void QGtkWindow::rotate_cb(GtkGestureRotate *pt, gdouble angle, gdouble angle_delta, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    guint32 ts;
    QPointF contentPoint;
    populateTsAndPoint(GTK_GESTURE(pt), ts, contentPoint);
    GdkEventSequence *seq = gtk_gesture_get_last_updated_sequence(GTK_GESTURE(pt));
    gtk_gesture_set_sequence_state(GTK_GESTURE(pt), seq, GTK_EVENT_SEQUENCE_CLAIMED); // ### not really sure this should be done here, but crashes in begin

    pw->rotate(contentPoint, angle, angle_delta, ts);
}

void QGtkWindow::begin_rotate_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    guint32 ts;
    QPointF contentPoint;
    populateTsAndPoint(GTK_GESTURE(pt), ts, contentPoint);

    qCDebug(lcGesture) << "Begin rotate " << pw->window() << ts << contentPoint;
    pw->beginRotate(contentPoint, ts);
}

void QGtkWindow::end_rotate_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    guint32 ts;
    QPointF contentPoint;
    populateTsAndPoint(GTK_GESTURE(pt), ts, contentPoint);

    qCDebug(lcGesture) << "End rotate " << pw->window() << ts << contentPoint;
    pw->endRotate(contentPoint, ts);
}

void QGtkWindow::cancel_rotate_cb(GtkGesture *pt, GdkEventSequence*, gpointer platformWindow)
{
    QGtkWindow *pw = static_cast<QGtkWindow*>(platformWindow);
    guint32 ts;
    QPointF contentPoint;
    populateTsAndPoint(GTK_GESTURE(pt), ts, contentPoint);

    qCDebug(lcGesture) << "Cancel rotate " << pw->window() << ts << contentPoint;
    pw->endRotate(contentPoint, ts);
}

void QGtkWindow::beginRotate(QPointF &contentPoint, guint32 ts)
{
    m_initialRotateSet = false;
    if (m_activeNativeGestures++ == 0) {
        qCDebug(lcGesture) << "Started native gesture sequence (due to rotate)";
        QWindowSystemInterface::handleGestureEvent(window(), ts, Qt::BeginNativeGesture, contentPoint, contentPoint);
    }
}

void QGtkWindow::rotate(QPointF &contentPoint, double angle, double angle_delta, guint32 ts)
{
    Q_UNUSED(angle_delta)
    angle = -angle;
    if (!m_initialRotateSet) {
        m_initialRotateSet = true;
        m_initialRotate = angle * 180 / M_PI;
    }
    double degrees = m_initialRotate - (angle * 180 / M_PI);
    m_initialRotate = angle * 180 / M_PI;
    QWindowSystemInterface::handleGestureEventWithRealValue(window(), ts, Qt::RotateNativeGesture, degrees, contentPoint, contentPoint);
}

void QGtkWindow::endRotate(QPointF &contentPoint, guint32 ts)
{
    if (--m_activeNativeGestures == 0) {
        qCDebug(lcGesture) << "Ended native gesture sequence (due to rotate)";
        QWindowSystemInterface::handleGestureEvent(window(), ts, Qt::EndNativeGesture, contentPoint, contentPoint);
    }
}

