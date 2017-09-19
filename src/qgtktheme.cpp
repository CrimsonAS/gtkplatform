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

#include "qgtk3dialoghelpers.h"
#include "qgtkmenubar.h"
#include "qgtkmenu.h"
#include "qgtkmenuitem.h"
#include "qgtktheme.h"
#include "qgtksystemtrayicon.h"

#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>
#include <QtGui/qpixmap.h>

const char *QGtkTheme::name = "gtkqpainternal";

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
    if (!m_fontConfigured) {
        m_fontConfigured = true;

        GtkSettings *s = gtk_settings_get_default();
        gchararray value;
        g_object_get(s, "gtk-font-name", &value, NULL);
        QString qtVal = QString::fromUtf8(value);
        g_free(value);

        if (qtVal.isNull()) {
            m_systemFont = QFont("Sans Serif", 11);
            m_monoFont = QFont("Monospace", m_systemFont.pointSize());
        } else {
            int lastSpace = qtVal.lastIndexOf(' ');
            int pointSize = qtVal.midRef(lastSpace+1).toInt();
            m_systemFont = QFont(qtVal.left(lastSpace), pointSize);
            // ### dconf also has monospace fonts, document fonts... how can we get these?
            // this is a bit of a hack...
            m_monoFont = QFont("Monospace", m_systemFont.pointSize());
        }
    }

    if (type == QPlatformTheme::FixedFont) {
        return &m_monoFont;
    }
    return &m_systemFont;
}

QPixmap QGtkTheme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
    return QPlatformTheme::standardPixmap(sp, size);
}

#if QT_VERSION >= QT_VERSION_CHECK(5,8,0)
QIcon QGtkTheme::fileIcon(const QFileInfo &fileInfo,
        QPlatformTheme::IconOptions options) const
{
    return QPlatformTheme::fileIcon(fileInfo, options);
}
#else
QPixmap QGtkTheme::fileIconPixmap(const QFileInfo &fileInfo,
        const QSizeF &size,
        QPlatformTheme::IconOptions options) const
{
    return QPlatformTheme::fileIconPixmap(fileInfo, size, options);
}
#endif

QVariant QGtkTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::SystemIconThemeName:
        return QVariant("Adwaita");
    case QPlatformTheme::StyleNames:
        return QStringList() << "Adwaita" << "Fusion";
    case QPlatformTheme::PasswordMaskCharacter:
        return QVariant(QChar(0x2022));
    default:
        break;
    }

    return QPlatformTheme::themeHint(hint);
}

QString QGtkTheme::standardButtonText(int button) const
{
    return QPlatformTheme::standardButtonText(button);
}

