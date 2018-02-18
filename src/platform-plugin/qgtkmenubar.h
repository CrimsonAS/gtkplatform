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

#ifndef QGTKMENUBAR_H
#define QGTKMENUBAR_H

#include "qgtkrefptr.h"

#include <qpa/qplatformmenu.h>

#include <gtk/gtk.h>

QT_BEGIN_NAMESPACE

class QGtkMenu;

class QGtkMenuBar : public QPlatformMenuBar
{
    Q_OBJECT
public:
    QGtkMenuBar();
    ~QGtkMenuBar();

    void insertMenu(QPlatformMenu *menu, QPlatformMenu *before) override;
    void removeMenu(QPlatformMenu *menu) override;
    void syncMenu(QPlatformMenu *menuItem) override;
    void handleReparent(QWindow *newParentWindow) override;

    QPlatformMenu *menuForTag(quintptr tag) const override;
    QPlatformMenu *createMenu() const override;

Q_SIGNALS:
    void updated();

private Q_SLOTS:
    void queueRegenerate();
    void regenerate();

private:
    QGtkRefPtr<GtkMenuBar> m_menubar;
    QVector<QPointer<QGtkMenu>> m_items;
    bool m_regenerateQueued = false;
};

QT_END_NAMESPACE

#endif // QGTKMENUBAR

