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

#ifndef QGTKMENU_H
#define QGTKMENU_H

#include "qgtkrefptr.h"

#include <qpa/qplatformmenu.h>

#include <gtk/gtk.h>

QT_BEGIN_NAMESPACE

class QGtkMenuItem;

class QGtkMenu : public QPlatformMenu
{
    Q_OBJECT
public:
    QGtkMenu();
    ~QGtkMenu();

    void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before) override;
    void removeMenuItem(QPlatformMenuItem *menuItem) override;
    void syncMenuItem(QPlatformMenuItem *menuItem) override;
    void syncSeparatorsCollapsible(bool enable) override;

    void setTag(quintptr tag) override;
    quintptr tag()const override;

    void setText(const QString &text) override;
    void setIcon(const QIcon &icon) override;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;
    void setVisible(bool visible) override;

    void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem  *item) override;
    void dismiss() override;

    QPlatformMenuItem *menuItemAt(int position) const override;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const override;

    QGtkRefPtr<GtkMenuItem> gtkMenuItem() const;

    QVector<QGtkMenuItem*> items() const;

private:
    QVector<QGtkMenuItem*> m_items;
    QVector<QGtkRefPtr<GtkWidget>> m_gtkItems;
    bool m_enabled = true;
    qintptr m_tag;
    QGtkRefPtr<GtkMenu> m_menu;
    QGtkRefPtr<GtkMenuItem> m_menuItem;
};

QT_END_NAMESPACE

#endif // QGTKMENU_H

