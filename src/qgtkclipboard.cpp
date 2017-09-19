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

#include "qgtkclipboard.h"
#include "qgtkhelpers.h"

#include <gtk/gtk.h>

#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcClipboard, "qt.qpa.gtk.clipboard");

QGtkClipboard::QGtkClipboard(QObject *parent)
    : QObject(parent)
    , m_clipData(QClipboard::Clipboard)
    , m_selData(QClipboard::Selection)
{
    QObject::connect(&m_clipData, &QGtkClipboardData::changed, this, [=](){ emitChanged(QClipboard::Clipboard); qCDebug(lcClipboard) << "Clipboard changed"; });
    QObject::connect(&m_selData, &QGtkClipboardData::changed, this, [=](){ emitChanged(QClipboard::Selection); qCDebug(lcClipboard) << "Selection changed"; });
}

QGtkClipboard::~QGtkClipboard()
{
}

QGtkClipboardData *QGtkClipboard::mimeForMode(QClipboard::Mode mode) const
{
    switch (mode) {
        case QClipboard::Clipboard:
            return const_cast<QGtkClipboardData*>(&m_clipData);
        case QClipboard::Selection:
            return const_cast<QGtkClipboardData*>(&m_selData);
        default:
            Q_UNREACHABLE();
    }
}

QMimeData *QGtkClipboard::mimeData(QClipboard::Mode mode)
{
    if (!supportsMode(mode))
        return 0;

    QGtkClipboardData *m = mimeForMode(mode);
    return m->mimeData();
}

void QGtkClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    if (!supportsMode(mode))
        return;
    QGtkClipboardData *m = mimeForMode(mode);
    m->setData(data);
    qCDebug(lcClipboard) << "setMimeData changed";
    emitChanged(mode);
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
    QGtkClipboardData *m = mimeForMode(mode);
    return m->ownsMode();
}

QGtkClipboardData::QGtkClipboardData(QClipboard::Mode clipboardMode)
    : m_mode(clipboardMode)
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

QGtkClipboardData::~QGtkClipboardData()
{
    gtk_clipboard_set_can_store(m_clipboard, NULL, 0);
    delete m_localData;
    delete m_systemData;
}

bool QGtkClipboardData::ownsMode() const
{
    return gtk_clipboard_get_owner(m_clipboard) != NULL;
}

static void getFun(GtkClipboard *, GtkSelectionData *selection_data, guint /*info*/, gpointer gtkClipboardData)
{
    qCDebug(lcClipboard) << "getFun";
    QGtkClipboardData *gdata = static_cast<QGtkClipboardData*>(gtkClipboardData);
    gdata->onLocalGet(selection_data);
}

static void clearFun(GtkClipboard*, gpointer gtkClipboardData)
{
    qCDebug(lcClipboard) << "clearFun";
    QGtkClipboardData *gdata = static_cast<QGtkClipboardData*>(gtkClipboardData);
    gdata->onLocalClear();
}

void QGtkClipboardData::onLocalClear()
{
    qCDebug(lcClipboard) << "Clear func" << m_mode;
    delete m_localData;
    m_localData = nullptr;
    Q_EMIT changed();
}

// Request for the data from our set clipboard to give to a remote client
void QGtkClipboardData::onLocalGet(GtkSelectionData *selection_data)
{
    GdkAtom targ = gtk_selection_data_get_target(selection_data);
    gchar *targstr = gdk_atom_name(targ);
    QByteArray data = m_localData ? m_localData->data(targstr) : QByteArray();
    qCDebug(lcClipboard) << "Got a request for data of type and size " << targstr << data.size() << m_mode;
    g_free(targstr);

    gtk_selection_data_set(selection_data, targ, 8, reinterpret_cast<const guchar*>(data.constData()), data.size());
}

// Set our local clipboard, and inform the system about it
void QGtkClipboardData::setData(QMimeData *data)
{
    qCDebug(lcClipboard) << "Setting mime data " << data << m_mode << (data ? data->formats() : QStringList());
    m_localData = data;
    if (!data || data->formats().isEmpty()) {
        qCDebug(lcClipboard) << "Clearing mime data" << data;
        gtk_clipboard_clear(m_clipboard);
        return;
    }

    if (data->hasText()) {
        QByteArray textData = data->text().toUtf8();
        gtk_clipboard_set_text(m_clipboard, textData.constData(), textData.size());
        return;
    }

    if (data->hasImage()) {
        QImage imageData = qvariant_cast<QImage>(data->imageData());
        gtk_clipboard_set_image(m_clipboard, qt_pixmapToPixbuf(QPixmap::fromImage(imageData)).get());
        return;
    }

    // ### rich text? html? what else should we handle...

    QStringList formats = m_localData->formats();
    qCDebug(lcClipboard) << "Setting mime data to " << formats;
    QVarLengthArray<GtkTargetEntry, 16> gtkTargets;
    for (QString &format : formats) {
        if (format.startsWith("text/plain")) {
            format = "text/plain;charset=utf-8";
        }
        // ### this seems suboptimal, is GtkTargetEntry supposed to be used
        // publically in this way?
        gtkTargets.append(GtkTargetEntry{strdup(format.toUtf8().constData()), 0, 0});
    }

    if (gtk_clipboard_set_with_data(
        m_clipboard,
        &gtkTargets[0],
        gtkTargets.size(),
        getFun,
        clearFun,
        this
    ) == TRUE) {
        gtk_clipboard_set_can_store(m_clipboard, NULL, 0);
    }

    for (GtkTargetEntry &gtkTarg : gtkTargets) {
        free(gtkTarg.target);
    }
}

QMimeData *QGtkClipboardData::mimeData() const
{
    qCDebug(lcClipboard) << "Getting data" << m_mode << m_localData << m_systemData;
    if (m_localData) {
        qCDebug(lcClipboard) << "Getting local data";
        return m_localData;
    }

    if (m_systemData) {
        qCDebug(lcClipboard) << "Clearing system data";
        m_systemData->clear();
    } else {
        qCDebug(lcClipboard) << "Creating system data";
        const_cast<QGtkClipboardData*>(this)->m_systemData = new QMimeData();
    }

    qCDebug(lcClipboard) << "Reading image data";
    QGtkRefPtr<GdkPixbuf> img = gtk_clipboard_wait_for_image(m_clipboard);
    if (img.get()) {
        qCDebug(lcClipboard) << "Image w h"
                   << gdk_pixbuf_get_width(img.get()) << gdk_pixbuf_get_height(img.get())
                   << "Stride alpha"
                   << gdk_pixbuf_get_rowstride(img.get())
                   << gdk_pixbuf_get_has_alpha(img.get());
        QImage data;
        if (gdk_pixbuf_get_has_alpha(img.get())) {
            data = QImage(gdk_pixbuf_get_pixels(img.get()),
                          gdk_pixbuf_get_width(img.get()), gdk_pixbuf_get_height(img.get()),
                          gdk_pixbuf_get_rowstride(img.get()),
                          QImage::Format_RGBA8888).copy();
            qCDebug(lcClipboard) << "Read RGBA8888 image " << data;
        } else {
            data = QImage(gdk_pixbuf_get_pixels(img.get()),
                          gdk_pixbuf_get_width(img.get()), gdk_pixbuf_get_height(img.get()),
                          gdk_pixbuf_get_rowstride(img.get()),
                          QImage::Format_RGB888).copy();
            qCDebug(lcClipboard) << "Read RGB32 image " << data;
        }
        if (!data.isNull()) {
            m_systemData->setImageData(QVariant::fromValue(data));
        }
        return m_systemData;
    }

    qCDebug(lcClipboard) << "Reading text data";
    const gchar *rdata = gtk_clipboard_wait_for_text(m_clipboard);
    if (rdata) {
        QString data = QString::fromUtf8((rdata), strlen(rdata));
        g_free((void*)rdata);
        if (!data.isNull()) {
            m_systemData->setText(data);
            qCDebug(lcClipboard) << "Read text " << data;
        }
        return m_systemData;
    }

    return m_systemData;
}

