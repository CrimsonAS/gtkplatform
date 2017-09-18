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
#include "qgtkrefptr.h"

#include <QtCore/qdebug.h>

#ifndef QGTKHELPERS_H
#define QGTKHELPERS_H

QT_BEGIN_NAMESPACE

QGtkRefPtr<GdkPixbuf> qt_pixmapToPixbuf(const QPixmap &pixmap);
QGtkRefPtr<GdkPixbuf> qt_iconToPixbuf(const QIcon &icon);
QGtkRefPtr<GIcon> qt_iconToIcon(const QIcon &icon);
QString qt_convertToGtkMnemonics(const QString &text);
Qt::KeyboardModifiers qt_convertToQtKeyboardMods(guint mask);
Qt::Key qt_convertToQtKey(int keyval);
guint qt_convertToGdkKeyval(Qt::Key qKey);
Qt::MouseButton qt_convertGButtonToQButton(guint button);
Qt::TouchPointState qt_convertToQtTouchPointState(GdkEventType type);

QT_END_NAMESPACE

#endif
