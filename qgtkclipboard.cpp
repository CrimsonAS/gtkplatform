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

#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcClipboard, "qt.qpa.gtk.clipboard");

QGtkClipboard::QGtkClipboard()
    : m_clipData(new QGtkClipboardMime(QClipboard::Clipboard))
    , m_selData(new QGtkClipboardMime(QClipboard::Selection))
{
}

QGtkClipboard::~QGtkClipboard()
{
}

QGtkClipboardMime *QGtkClipboard::mimeForMode(QClipboard::Mode mode) const
{
    switch (mode) {
        case QClipboard::Clipboard:
            return m_clipData.get();
        case QClipboard::Selection:
            return m_selData.get();
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
    if (!supportsMode(mode))
        return;
    QGtkClipboardMime *m = mimeForMode(mode);
    m->setMimeData(data);
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
    QGtkClipboardMime *m = mimeForMode(mode);
    return m->ownsMode();
}

QGtkClipboardMime::QGtkClipboardMime(QClipboard::Mode clipboardMode)
{
    switch (clipboardMode) {
    case QClipboard::Clipboard:
        m_clipboard = gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", TRUE));
        break;
    case QClipboard::Selection:
        m_clipboard = gtk_clipboard_get(gdk_atom_intern("PRIMARY", TRUE));
        break;
    default:
        Q_UNREACHABLE();
    }
}

QGtkClipboardMime::~QGtkClipboardMime()
{
    gtk_clipboard_store(m_clipboard);
}

bool QGtkClipboardMime::hasFormat_sys(const QString &mimeType) const
{
    return formats().contains(mimeType);
}

QStringList QGtkClipboardMime::formats_sys() const
{
    GdkAtom *targs;
    gint ntargs;

    // ### this involves event loop reentry, so not very safe
    gboolean hasTargets = gtk_clipboard_wait_for_targets(m_clipboard, &targs, &ntargs);
    if (!hasTargets) {
        return QStringList();
    }

    QStringList formatList;
    formatList.reserve(ntargs);

    // ### consider caching?
    for (int i = 0; i < ntargs; ++i) {
        gchar *str = gdk_atom_name(targs[i]);
        formatList.append(str);
        g_free(str);
    }

    g_free(targs);
    return formatList;
}

QVariant QGtkClipboardMime::retrieveData_sys(const QString &mimeType, QVariant::Type type) const
{
    if (mimeType.isEmpty()) {
        return QVariant();
    }

    QStringList targets = formats();
    if (type == QVariant::String) {
        GtkSelectionData *data = gtk_clipboard_wait_for_contents(m_clipboard, gdk_atom_intern("text/plain", TRUE));
        guchar *text = gtk_selection_data_get_text(data);
        if (text) {
            return QString::fromUtf8(reinterpret_cast<char*>(text), strlen(reinterpret_cast<char*>(text)));
            g_free(text);
        } else {
            return QString();
        }
    }

    // ### QImage, type mapping, etc.

    return QVariant();
}

bool QGtkClipboardMime::ownsMode() const
{
    return gtk_clipboard_get_owner(m_clipboard) != NULL;
}

static void getFun(GtkClipboard *, GtkSelectionData *selection_data, guint /*info*/, gpointer gtkClipboardMime)
{
    QGtkClipboardMime *gmime = static_cast<QGtkClipboardMime*>(gtkClipboardMime);
    GdkAtom targ = gtk_selection_data_get_target(selection_data);
    gchar *targstr = gdk_atom_name(targ);
    QMimeData *realMime = gmime->currentData();
    QByteArray data = realMime ? realMime->data(targstr) : QByteArray();
    qCWarning(lcClipboard) << "Got a request for data of type and size " << targstr << data.size();
    g_free(targstr);

    gtk_selection_data_set(selection_data, targ, 8, reinterpret_cast<const guchar*>(data.constData()), data.size());
}

static void clearFun(GtkClipboard*, gpointer /*gtkClipboardMime*/)
{
    qCWarning(lcClipboard) << "clear func";
}

void QGtkClipboardMime::setMimeData(QMimeData *data)
{
    if (!data) {
        qCWarning(lcClipboard) << "Clearing mime data" << data;
        gtk_clipboard_clear(m_clipboard);
        return;
    }
    m_currentData = data;

    const QStringList formats = m_currentData->formats();
    qCWarning(lcClipboard) << "Setting mime data to " << formats;
    QVarLengthArray<GtkTargetEntry*, 16> gtkTargets;
    for (const QString &format : formats) {
        gtkTargets.append(gtk_target_entry_new(format.toUtf8().constData(), 0, 0));
    }

    if (gtk_clipboard_set_with_data(
        m_clipboard,
        gtkTargets[0],
        gtkTargets.size(),
        getFun,
        clearFun,
        this
    )) {
        gtk_clipboard_set_can_store(m_clipboard, gtkTargets[0], gtkTargets.size());
    }

    for (GtkTargetEntry *gtkTarg : gtkTargets) {
        gtk_target_entry_free(gtkTarg);
    }
}
