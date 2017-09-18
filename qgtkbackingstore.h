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

#ifndef QBACKINGSTORE_GTK_H
#define QBACKINGSTORE_GTK_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformwindow.h>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

class QGtkBackingStore : public QPlatformBackingStore
{
public:
    QGtkBackingStore(QWindow *window);
    ~QGtkBackingStore();

    QPaintDevice *paintDevice() override;
    void beginPaint(const QRegion &region) override;
    void endPaint() override;
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                         QPlatformTextureList *textures, QOpenGLContext *context, bool translucentBackground) override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;
    QImage toImage() const override;
    bool scroll(const QRegion &region, int dx, int dy) override;

private:
    QImage *m_paintImage;
};

QT_END_NAMESPACE

#endif
