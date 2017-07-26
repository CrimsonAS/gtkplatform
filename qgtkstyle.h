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

#ifndef QGTKSTYLE_H
#define QGTKSTYLE_H

#include <QtWidgets/qstyle.h>
#include <QtWidgets/qcommonstyle.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

QT_BEGIN_NAMESPACE

class QGtkStyle : public QCommonStyle
{
public:
    explicit QGtkStyle();
    ~QGtkStyle();

     void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                                const QWidget *w = Q_NULLPTR) const override;

     void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                              const QWidget *w = Q_NULLPTR) const override;

     QRect subElementRect(SubElement subElement, const QStyleOption *option,
                                  const QWidget *widget = Q_NULLPTR) const override;




     void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                                     const QWidget *widget = Q_NULLPTR) const override;
     SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                              const QPoint &pt, const QWidget *widget = Q_NULLPTR) const  override;
     QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                                  SubControl sc, const QWidget *widget = Q_NULLPTR) const override;






     int pixelMetric(PixelMetric metric, const QStyleOption *option = Q_NULLPTR,
                             const QWidget *widget = Q_NULLPTR) const override;

     QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                                    const QSize &contentsSize, const QWidget *w = Q_NULLPTR) const override;



     int styleHint(StyleHint stylehint, const QStyleOption *opt = Q_NULLPTR,
                           const QWidget *widget = Q_NULLPTR, QStyleHintReturn* returnData = Q_NULLPTR) const override;



     QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt = Q_NULLPTR,
                                    const QWidget *widget = Q_NULLPTR) const override;

     QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = Q_NULLPTR,
                                const QWidget *widget = Q_NULLPTR) const override;

     QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                         const QStyleOption *opt) const override;


     int layoutSpacing(QSizePolicy::ControlType control1,
                               QSizePolicy::ControlType control2, Qt::Orientation orientation,
                               const QStyleOption *option = Q_NULLPTR, const QWidget *widget = Q_NULLPTR)  const override;

     QPalette standardPalette() const override;

private:
    mutable QImage m_image;
    cairo_t *ensureImage(int w, int h) const;
    void doneImage(cairo_t *ctx) const;
    GtkWidget *m_label;
};

QT_END_NAMESPACE

#endif

