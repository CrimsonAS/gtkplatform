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

#include "qgtkclipboard.h"

#include <gtk/gtk.h>

#include <QtCore/qdebug.h>

QGtkClipboard::QGtkClipboard()
{

}

QGtkClipboard::~QGtkClipboard()
{

}

static GdkAtom clipAtom(QClipboard::Mode mode)
{
    switch (mode) {
        case QClipboard::Clipboard:
            return gdk_atom_intern("CLIPBOARD", TRUE);
        case QClipboard::Selection:
            return gdk_atom_intern("PRIMARY", TRUE);
        default:
            Q_UNREACHABLE();
    }
}

QGtkClipboardMime *QGtkClipboard::mimeForMode(QClipboard::Mode mode)
{
    switch (mode) {
        case QClipboard::Clipboard:
            return m_clipData;
        case QClipboard::Selection:
            return m_selData;
        default:
            Q_UNREACHABLE();
    }
}

QMimeData *QGtkClipboard::mimeData(QClipboard::Mode mode)
{
    if (!supportsMode(mode))
        return 0;

    QGtkClipboardMime *m = mimeForMode(mode);
    return m;
}

void QGtkClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{

}

bool QGtkClipboard::supportsMode(QClipboard::Mode mode) const
{
    switch (mode) {
    case QClipboard::Clipboard:
    case QClipboard::Selection:
        return true;
    default:
        return false;
    }
}

bool QGtkClipboard::ownsMode(QClipboard::Mode mode) const
{
    if (!supportsMode(mode))
        return false;
    GtkClipboard *clip = gtk_clipboard_get(clipAtom(mode));
    return gtk_clipboard_get_owner(clip) != NULL;
}

QGtkClipboardMime::QGtkClipboardMime(QClipboard::Mode clipboardMode)
    : m_clipboardMode(clipboardMode)
{
}

QGtkClipboardMime::~QGtkClipboardMime()
{

}

bool QGtkClipboardMime::hasFormat_sys(const QString &mimeType) const
{
    return formats().contains(mimeType);
}

QStringList QGtkClipboardMime::formats_sys() const
{
    GdkAtom *targs;
    gint ntargs;
    GtkClipboard *clip = gtk_clipboard_get(clipAtom(m_clipboardMode));
    gboolean worked = gtk_clipboard_wait_for_targets(clip, &targs, &ntargs);
    // ### check worked


    QStringList formatList;
    formatList.reserve(ntargs);

    // ### consider caching
    for (int i = 0; i < ntargs; ++i) {
        formatList.append(gdk_atom_name(targs[i]));
    }

    return formatList;
}

QVariant QGtkClipboardMime::retrieveData_sys(const QString &mimeType, QVariant::Type type) const
{
    if (mimeType.isEmpty()) {
        return QVariant();
    }

    // ### slow stuff here
    if (type == QImage::Type)
}

