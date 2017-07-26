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

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qradiobutton.h>
#include <QtWidgets/qpushbutton.h>
#include <QtGui/qpainter.h>
#include <QtCore/qdebug.h>

#include "foreign-drawing.h"
#include "qgtkstyle.h"
#include "qgtkwindow.h"

QGtkStyle::QGtkStyle()
    : QCommonStyle()
{
    m_label = gtk_label_new(NULL);
}

QGtkStyle::~QGtkStyle()
{
}

static GtkStateFlags qStyleFlagsToGtkStyleFlags(const QStyle::State &state)
{
    int flgs = GTK_STATE_FLAG_NORMAL;

    if (!(state & QStyle::State_Enabled))
        flgs |= GTK_STATE_FLAG_INSENSITIVE;
    if (state & QStyle::State_HasFocus)
        flgs |= GTK_STATE_FLAG_FOCUSED;
    if (state & QStyle::State_On)
        flgs |= GTK_STATE_FLAG_CHECKED;
    if (state & QStyle::State_Sunken)
        flgs |= GTK_STATE_FLAG_ACTIVE;
    if (state & QStyle::State_MouseOver)
        flgs |= GTK_STATE_FLAG_PRELIGHT;

    return GtkStateFlags(flgs);
}

// Ensures that a cairo_t is created that can fit an image of w/h dimensions.
// Note that this is backed by a shared image, so you can only have one cairo_t
// active at a time from it.
//
// The returned cairo_t must be unreffed when done.
cairo_t *QGtkStyle::ensureImage(int w, int h) const
{
    // this should actually be an overallocation; the size of the full widget
    // rather than the primitive, but that doesn't matter I think.
    if (m_image.width() < w || m_image.height() < h) {
        m_image = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    }

    m_image.fill(Qt::transparent);
    cairo_surface_t *surf = cairo_image_surface_create_for_data(
            const_cast<uchar*>(m_image.constBits()),
            CAIRO_FORMAT_ARGB32,
            m_image.width(),
            m_image.height(),
            m_image.bytesPerLine()
    );

    cairo_t* ctx = cairo_create(surf);
    cairo_surface_destroy(surf); // ctx took a ref
    return ctx;
}

struct QGtkStyleContext
{
    QGtkStyleContext(GtkStyleContext *ctx)
        : m_ctx(ctx)
    {
    }
    ~QGtkStyleContext()
    {
        g_object_unref(m_ctx);
    }
    operator GtkStyleContext*()
    {
        return m_ctx;
    }
private:
    GtkStyleContext *m_ctx;
};

void QRadioButton::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);
    p.fillRect(0, 0, width(), height(), Qt::red);
}

void QGtkStyle::drawControl(ControlElement element, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    cairo_t *ctx = ensureImage(p->device()->width(), p->device()->height());
    int ww = 0; int hh = 0;

    const QStyleOptionButton *btnStyle = qstyleoption_cast<const QStyleOptionButton *>(opt);
    switch (element) {
    case CE_PushButtonBevel: {
        if (btnStyle && qobject_cast<const QPushButton*>(w)) {
            ww = btnStyle->rect.width();
            hh = btnStyle->rect.height();
            qDebug() << "button bevel" << ww << hh;
            QGtkStyleContext button_context = get_style (NULL, "button");

            gtk_style_context_set_state(button_context, qStyleFlagsToGtkStyleFlags(btnStyle->state));

            query_size (button_context, &ww, &hh);

            draw_style_common(button_context, ctx, 0, 0, ww, hh, NULL, NULL, NULL, NULL);
        }
        break;
    }
    default:
        QCommonStyle::drawControl(element, opt, p, w);
        break;
    }

    if (ww > 0 && hh > 0)
        p->drawImage(0, 0, m_image, 0, 0, ww, hh);

    cairo_destroy(ctx);
}

QSize QGtkStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt,
                            const QSize &contentsSize, const QWidget *w) const
{
    return QCommonStyle::sizeFromContents(ct, opt, contentsSize, w);
}

void QGtkStyle::drawPrimitive(QStyle::PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                        const QWidget *w) const
{
    cairo_t *ctx = ensureImage(p->device()->width(), p->device()->height());
    int ww = 0; int hh = 0;

    const QStyleOptionButton *btnStyle = qstyleoption_cast<const QStyleOptionButton *>(opt);

    switch (pe) {
    case PE_IndicatorCheckBox: {
        if (btnStyle && qobject_cast<const QCheckBox*>(w)) {
            QGtkStyleContext button_context = get_style (NULL, "checkbutton");
            QGtkStyleContext check_context = get_style (button_context, "check");
            gint contents_x, contents_y, contents_width, contents_height;

            gtk_style_context_set_state(check_context, qStyleFlagsToGtkStyleFlags(btnStyle->state));

            query_size (button_context, &ww, &hh);
            query_size (check_context, &ww, &hh);

            draw_style_common (button_context, ctx, 0, 0, ww, hh, NULL, NULL, NULL, NULL);
            draw_style_common (check_context, ctx, 0, 0, ww, hh, &contents_x, &contents_y, &contents_width, &contents_height);
            gtk_render_check (check_context, ctx, contents_x, contents_y, contents_width, contents_height);
        }
        break;
    }
    case PE_IndicatorRadioButton: {
        if (btnStyle && qobject_cast<const QRadioButton*>(w)) {
            QGtkStyleContext button_context = get_style (NULL, "radiobutton");
            QGtkStyleContext check_context = get_style (button_context, "radio");
            gint contents_x, contents_y, contents_width, contents_height;

            gtk_style_context_set_state(check_context, qStyleFlagsToGtkStyleFlags(btnStyle->state));

            query_size (button_context, &ww, &hh);
            query_size (check_context, &ww, &hh);

            draw_style_common (button_context, ctx, 0, 0, ww, hh, NULL, NULL, NULL, NULL);
            draw_style_common (check_context, ctx, 0, 0, ww, hh, &contents_x, &contents_y, &contents_width, &contents_height);
            gtk_render_check (check_context, ctx, contents_x, contents_y, contents_width, contents_height);
        }
        break;
    }
    default:
        QCommonStyle::drawPrimitive(pe, opt, p, w);
        break;
    }

    if (ww > 0 && hh > 0)
        p->drawImage(0, 0, m_image, 0, 0, ww, hh);

    cairo_destroy(ctx);

}

int QGtkStyle::pixelMetric(PixelMetric metric, const QStyleOption *opt,
                     const QWidget *w) const
{
    int width = 0, height = 0;
    const QStyleOptionButton *btnStyle = qstyleoption_cast<const QStyleOptionButton *>(opt);

    if (metric == PM_DefaultFrameWidth) {
        if (btnStyle && qobject_cast<const QPushButton*>(w)) {
            QGtkStyleContext button_context = get_style(NULL, "button");

            // ### .image-button, .text-button
            gtk_style_context_set_state(button_context, qStyleFlagsToGtkStyleFlags(btnStyle->state));

            query_size(button_context, &width, &height);

            return width;
        }
    }
    
    
    return QCommonStyle::pixelMetric(metric, opt, w);
}

QRect QGtkStyle::subElementRect(SubElement subElement, const QStyleOption *opt,
                          const QWidget *w) const
{
    return QCommonStyle::subElementRect(subElement, opt, w);
}


void QGtkStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                             const QWidget *widget) const
{
    return QCommonStyle::drawComplexControl(cc, opt, p, widget);
}

QStyle::SubControl QGtkStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                      const QPoint &pt, const QWidget *widget) const
{
    return QCommonStyle::hitTestComplexControl(cc, opt, pt, widget);
}

QRect QGtkStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                          QStyle::SubControl sc, const QWidget *widget) const
{
    return QCommonStyle::subControlRect(cc, opt, sc, widget);
}


int QGtkStyle::styleHint(StyleHint sh, const QStyleOption *opt,
                   const QWidget *widget, QStyleHintReturn* returnData) const
{
    return QCommonStyle::styleHint(sh, opt, widget, returnData);
}

QPixmap QGtkStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                            const QWidget *widget) const
{
return QCommonStyle::standardPixmap(standardPixmap, opt, widget);
}

QIcon QGtkStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option,
                        const QWidget *widget) const
{
    return QCommonStyle::standardIcon(standardIcon, option, widget);
}

QPixmap QGtkStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                 const QStyleOption *opt) const
{
    return QCommonStyle::generatedIconPixmap(iconMode, pixmap, opt);
}


int QGtkStyle::layoutSpacing(QSizePolicy::ControlType control1,
                       QSizePolicy::ControlType control2, Qt::Orientation orientation,
                       const QStyleOption *option, const QWidget *widget)  const
{
    return QCommonStyle::layoutSpacing(control1, control2, orientation, option, widget);
}

QPalette QGtkStyle::standardPalette() const
{
    return QCommonStyle::standardPalette();
}

