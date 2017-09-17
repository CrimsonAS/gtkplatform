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
