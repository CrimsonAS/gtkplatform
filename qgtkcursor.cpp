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

#include "qgtkcursor.h"
#include "qgtkhelpers.h"
#include "qgtkwindow.h"

QGtkCursor::QGtkCursor()
    : QPlatformCursor()
{
}

void QGtkCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    Qt::CursorShape shape = Qt::BlankCursor;
    if (windowCursor) {
        shape = windowCursor->shape();
    }

    QByteArray gtkCursorName;
    bool bitmapCursor = false;

    switch (shape) {
    case Qt::BlankCursor:
    case Qt::CustomCursor:
    case Qt::ArrowCursor:
        gtkCursorName = "default";
        break;
    case Qt::UpArrowCursor:
        gtkCursorName = "default";
        break;
    case Qt::CrossCursor:
        gtkCursorName = "crosshair";
        break;
    case Qt::WaitCursor:
        gtkCursorName = "wait";
        break;
    case Qt::IBeamCursor:
        gtkCursorName = "text";
        break;
    case Qt::SizeVerCursor:
        gtkCursorName = "row-resize";
        break;
    case Qt::SizeHorCursor:
        gtkCursorName = "col-resize";
        break;
    case Qt::SizeBDiagCursor:
        gtkCursorName = "nesw-resize";
        break;
    case Qt::SizeFDiagCursor:
        gtkCursorName = "nwse-resize";
        break;
    case Qt::SizeAllCursor:
        gtkCursorName = "all-scroll";
        break;
    case Qt::SplitVCursor:
        gtkCursorName = "ns-resize";
        break;
    case Qt::SplitHCursor:
        gtkCursorName = "ew-resize";
        break;
    case Qt::PointingHandCursor:
        gtkCursorName = "pointer";
        break;
    case Qt::ForbiddenCursor:
        gtkCursorName = "not-allowed";
        break;
    case Qt::OpenHandCursor:
        gtkCursorName = "grab";
        break;
    case Qt::ClosedHandCursor:
        gtkCursorName = "grabbing";
        break;
    case Qt::WhatsThisCursor:
        gtkCursorName = "help";
        break;
    case Qt::BusyCursor:
        gtkCursorName = "progress";
        break;
    case Qt::DragMoveCursor:
        gtkCursorName = "grabbing";
        break;
    case Qt::DragCopyCursor:
        gtkCursorName = "copy";
        break;
    case Qt::DragLinkCursor:
        gtkCursorName = "alias";
        break;
    case Qt::BitmapCursor:
        bitmapCursor = true;
        break;
    }

    GdkCursor *c = nullptr;
    if (bitmapCursor == false) {
        c = gdk_cursor_new_from_name(gdk_display_get_default(), gtkCursorName.constData());
    } else {
        c = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), qt_pixmapToPixbuf(windowCursor->pixmap()).get(), 0, 0);
    }

    QGtkWindow *pw = static_cast<QGtkWindow*>(window->handle());

    // ### are we called before being realized, sometimes? why does this happen?
    if (gtk_widget_get_window(pw->gtkWindow().get()) != nullptr) {
        gdk_window_set_cursor(gtk_widget_get_window(pw->gtkWindow().get()), c);
    }
}

QPoint QGtkCursor::pos() const
{
    // not supported.
    return m_pos;
}

void QGtkCursor::setPos(const QPoint &pos)
{
    // not supported.
    m_pos = pos;
}

