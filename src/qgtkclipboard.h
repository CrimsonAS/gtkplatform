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

#ifndef QGTKCLIPBOARD_H
#define QGTKCLIPBOARD_H

#include <qpa/qplatformclipboard.h>
#include <private/qdnd_p.h>

#include <gtk/gtk.h>

QT_BEGIN_NAMESPACE

class QGtkClipboardData : public QObject
{
    Q_OBJECT
public:
    QGtkClipboardData(QClipboard::Mode clipboardMode);
    ~QGtkClipboardData();

    void setData(QMimeData *data);
    QMimeData *mimeData() const;
    bool ownsMode() const;

    void onLocalClear();
    void onLocalGet(GtkSelectionData *selection_data, guint info);

Q_SIGNALS:
    void changed();

private:
    QStringList formats() const;

    GtkClipboard *m_clipboard = nullptr;
    QMimeData *m_localData = nullptr; // app-local
    QMimeData *m_systemData = nullptr; // system-global, remote
    QClipboard::Mode m_mode;
};

class QGtkClipboard : public QPlatformClipboard, QObject
{
public:
    QGtkClipboard(QObject *parent = 0);
    ~QGtkClipboard();

    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

private:
    QGtkClipboardData m_clipData;
    QGtkClipboardData m_selData;
    QGtkClipboardData *mimeForMode(QClipboard::Mode mode) const;
};

QT_END_NAMESPACE

#endif // QGTKCLIPBOARD_H

