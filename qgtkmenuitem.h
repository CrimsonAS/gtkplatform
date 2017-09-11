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

#ifndef QGTKMENUITEM_H
#define QGTKMENUITEM_H

#include <qpa/qplatformmenu.h>

#include <gtk/gtk.h>

class QGtkMenu;

class QGtkMenuItem : public QPlatformMenuItem
{
    Q_OBJECT
public:
    QGtkMenuItem();
    ~QGtkMenuItem();

    void setTag(quintptr tag) override;
    quintptr tag()const override;
    void setText(const QString &text) override;
    void setIcon(const QIcon &icon) override;
    void setMenu(QPlatformMenu *menu) override;
    void setVisible(bool isVisible) override;
    void setIsSeparator(bool isSeparator) override;
    void setFont(const QFont &font) override;
    void setRole(MenuRole role) override;
    void setCheckable(bool checkable) override;
    void setChecked(bool isChecked) override;
    void setShortcut(const QKeySequence& shortcut) override;
    void setEnabled(bool enabled) override;
    void setIconSize(int size) override;
    void setNativeContents(WId item) override;
    void setHasExclusiveGroup(bool hasExclusiveGroup) override;

    GtkWidget *gtkMenuItem() const;

    void emitSelect();
    void emitActivate();
Q_SIGNALS:
    void changed();

private:
    QString m_text;
    bool m_isSeparator = false;
    bool m_visible = true;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_enabled = true;
    bool m_hasExclusiveGroup = false;
    QGtkMenu *m_childMenu = nullptr;
    QKeySequence m_shortcut;
    qintptr m_tag;
};

#endif // QGTKMENUITEM_H


