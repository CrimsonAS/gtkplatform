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

#include "qgtk3dialoghelpers.h"
#include "qgtkmenubar.h"
#include "qgtkmenu.h"
#include "qgtkmenuitem.h"
#include "qgtktheme.h"
#include "qgtksystemtrayicon.h"

#include <QtCore/qvariant.h>
#include <QtGui/qpixmap.h>

const char *QGtkTheme::name = "gtk";

QGtkTheme::QGtkTheme()
{

}

QGtkTheme::~QGtkTheme()
{

}

QPlatformMenuItem* QGtkTheme::createPlatformMenuItem() const
{
    return new QGtkMenuItem;
}

QPlatformMenu* QGtkTheme::createPlatformMenu() const
{
    return new QGtkMenu;
}

QPlatformMenuBar* QGtkTheme::createPlatformMenuBar() const
{
    return new QGtkMenuBar;
}

#ifndef QT_NO_SYSTEMTRAYICON
QPlatformSystemTrayIcon *QGtkTheme::createPlatformSystemTrayIcon() const
{
    return new QGtkSystemTrayIcon;
}
#endif

bool QGtkTheme::usePlatformNativeDialog(DialogType dialogType) const
{
    switch (dialogType) {
    case QPlatformTheme::FileDialog:
    case QPlatformTheme::FontDialog:
    case QPlatformTheme::ColorDialog:
        return true;
    case QPlatformTheme::MessageDialog:
        break;
    }
    return false;
}

QPlatformDialogHelper *QGtkTheme::createPlatformDialogHelper(DialogType dialogType) const
{
    switch (dialogType) {
    case QPlatformTheme::FileDialog:
        return new QGtk3FileDialogHelper;
    case QPlatformTheme::FontDialog:
        return new QGtk3FontDialogHelper;
    case QPlatformTheme::ColorDialog:
        return new QGtk3ColorDialogHelper;
    case QPlatformTheme::MessageDialog:
        break;
    }
    return 0;
}

const QPalette *QGtkTheme::palette(Palette type) const
{
    return QPlatformTheme::palette(type);
}

const QFont *QGtkTheme::font(Font type) const
{
    return QPlatformTheme::font(type);
}

QPixmap QGtkTheme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
    return QPlatformTheme::standardPixmap(sp, size);
}

QPixmap QGtkTheme::fileIconPixmap(const QFileInfo &fileInfo,
        const QSizeF &size,
        QPlatformTheme::IconOptions options) const
{
    return QPlatformTheme::fileIconPixmap(fileInfo, size, options);
}

QVariant QGtkTheme::themeHint(ThemeHint hint) const
{
    return QPlatformTheme::themeHint(hint);
}

QString QGtkTheme::standardButtonText(int button) const
{
    return QPlatformTheme::standardButtonText(button);
}

