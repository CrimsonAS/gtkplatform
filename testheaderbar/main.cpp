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

#include <QApplication>
#include <QWidget>
#include <QWindow>
#include <QLabel>
#include <QDebug>

#include <qpa/qplatformnativeinterface.h>

#include <gtk/gtk.h>

class TestWindow : public QWidget
{
public:
    TestWindow()
    {
        // ensure pwin is created
        show();
        hide();

        QWindow *qwin = windowHandle();
        if (!qwin) {
            qwin = nativeParentWidget()->windowHandle();
        }

        if (!qwin) {
            qFatal("no QWindow!");
        }

        GtkWidget *hb = gtk_header_bar_new();
        gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "A magical Qt test");
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Featuring a real GtkHeaderBar");
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);

        GtkWidget *button = gtk_button_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_header_bar_pack_end(GTK_HEADER_BAR(hb), button);

        QPlatformNativeInterface *platformNativeInterface = QApplication::platformNativeInterface();
        GtkWidget *w = static_cast<GtkWidget*>(platformNativeInterface->nativeResourceForWindow("gtkwindow", qwin));
        if (!w) {
            qFatal("no GtkWidget! (not using the right QPA?)");
        }
        gtk_window_set_titlebar(GTK_WINDOW(w), hb);
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    TestWindow w;

    QLabel l(&w);
    l.setText("I'm a QLabel");

    w.resize(200, 200);
    w.show();

    return app.exec();
}
