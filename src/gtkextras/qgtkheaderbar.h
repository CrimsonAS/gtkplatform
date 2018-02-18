/****************************************************************************
**
** Copyright (C) 2018 Crimson AS <info@crimson.no>
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

#ifndef QGTKHEADERBAR_H
#define QGTKHEADERBAR_H

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <gtk/gtk.h>

#include "qgtkextrasglobal.h"

QT_BEGIN_NAMESPACE

class Q_GTKEXTRAS_EXPORT QGtkHeaderBar : public QObject
{
public:
    QGtkHeaderBar(QObject *parent);
    ~QGtkHeaderBar();

    GtkWidget *headerBarWidget() const;

private:
    class QGtkHeaderBarPrivate;
    QGtkHeaderBarPrivate *d;
};

QT_END_NAMESPACE

#endif // QGTKHEADERBAR_H
