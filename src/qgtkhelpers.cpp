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

#include "qgtkhelpers.h"

QGtkRefPtr<GdkPixbuf> qt_pixmapToPixbuf(const QPixmap &pixmap)
{
    // ### assert pixmap is in RGBA
    QImage i = pixmap.toImage();

    QGtkRefPtr<GdkPixbuf> gpb = gdk_pixbuf_new_from_data(
        i.constBits(),
        GDK_COLORSPACE_RGB,
        true,
        8,
        i.width(),
        i.height(),
        i.bytesPerLine(),
        NULL,
        NULL
    );
    return gpb;
}

QImage qt_pixbufToImage(const QGtkRefPtr<GdkPixbuf> &pixbuf)
{
    QImage data;
    if (gdk_pixbuf_get_has_alpha(pixbuf.get())) {
        data = QImage(gdk_pixbuf_get_pixels(pixbuf.get()),
                      gdk_pixbuf_get_width(pixbuf.get()), gdk_pixbuf_get_height(pixbuf.get()),
                      gdk_pixbuf_get_rowstride(pixbuf.get()),
                      QImage::Format_RGBA8888).copy();
    } else {
        data = QImage(gdk_pixbuf_get_pixels(pixbuf.get()),
                      gdk_pixbuf_get_width(pixbuf.get()), gdk_pixbuf_get_height(pixbuf.get()),
                      gdk_pixbuf_get_rowstride(pixbuf.get()),
                      QImage::Format_RGB888).copy();
    }
    return data;
}

// Returns the largest image for this icon, in RGBA format.
QImage qt_getBiggestImageForIcon(const QIcon &icon)
{
    QIcon::State st = QIcon::On;
    QList<QSize> sizes = icon.availableSizes(QIcon::Normal, st);
    if (!sizes.length()) {
        st = QIcon::Off;
        sizes = icon.availableSizes(QIcon::Normal, st);
        if (!sizes.length()) {
            qWarning() << "No available icons for icon?" << icon;
            return QImage();
        }
    }

    // Find the largest size, hopefully it's the best looking.
    QSize sz;
    for (int i = 0; i < sizes.length(); ++i) {
        const QSize &nsz = sizes.at(i);

        if (nsz.width() * nsz.height() >= sz.width() * sz.height()) {
            sz = nsz;
        }
    }

    QPixmap p = icon.pixmap(sz, QIcon::Normal, st);
    QImage i = p.toImage().convertToFormat(QImage::Format_RGBA8888);
    return i;
}

// Convert a QIcon to a GdkPixbuf for use elsewhere.
QGtkRefPtr<GdkPixbuf> qt_iconToPixbuf(const QIcon &icon)
{
    QImage i = qt_getBiggestImageForIcon(icon);
    if (i.isNull())
        return 0;
    QGtkRefPtr<GdkPixbuf> gpb = gdk_pixbuf_new_from_data(
        i.constBits(),
        GDK_COLORSPACE_RGB,
        true,
        8,
        i.width(),
        i.height(),
        i.bytesPerLine(),
        NULL,
        NULL
    );

    return gpb;
}

// Convert a QIcon to a GIcon for use elsewhere.
QGtkRefPtr<GIcon> qt_iconToIcon(const QIcon &icon)
{
    QImage i = qt_getBiggestImageForIcon(icon);
    QGtkRefPtr<GBytes> bytes = g_bytes_new_take(const_cast<uchar*>(i.constBits()), i.byteCount());
    QGtkRefPtr<GIcon> ico = g_bytes_icon_new(bytes.get());
    return ico;
}

// Qt uses &, gtk uses _.
QString qt_convertToGtkMnemonics(const QString &text)
{
    QString cpy = text;
    return cpy.replace("&", "_"); // ### too simple! need to leave &&.
}

Qt::KeyboardModifiers qt_convertToQtKeyboardMods(guint mask)
{
    Qt::KeyboardModifiers mods = Qt::NoModifier;

    if (mask & GDK_SHIFT_MASK)
        mods |= Qt::ShiftModifier;
    if (mask & GDK_CONTROL_MASK)
        mods |= Qt::ControlModifier;
    if (mask & GDK_MOD1_MASK)
        mods |= Qt::AltModifier;
    if (mask & GDK_META_MASK)
        mods |= Qt::MetaModifier;
#if 0
    if (mask & GDK_SUPER_MASK)
        qDebug() << "Super";
    if (mask & GDK_HYPER_MASK)
        qDebug() << "Hyper";
    if (mask & GDK_MOD2_MASK)
        qDebug() << "Mod2";
    if (mask & GDK_MOD3_MASK)
        qDebug() << "Mod3";
    if (mask & GDK_MOD4_MASK)
        qDebug() << "Mod4";
    if (mask & GDK_MOD5_MASK)
        qDebug() << "Mod5";
#endif
    return mods;
}

Qt::Key qt_convertToQtKey(int keyval)
{
    switch (keyval) {
    case GDK_KEY_BackSpace:
        return Qt::Key_Backspace;
    case GDK_KEY_KP_Tab:
    case GDK_KEY_Tab:
        return Qt::Key_Tab;
    case GDK_KEY_Clear:
        return Qt::Key_Clear;
    case GDK_KEY_Return:
        return Qt::Key_Return;
    case GDK_KEY_KP_Enter:
        return Qt::Key_Enter;
    case GDK_KEY_Pause:
        return Qt::Key_Pause;
    case GDK_KEY_Scroll_Lock:
        return Qt::Key_ScrollLock;
    case GDK_KEY_Sys_Req:
        return Qt::Key_SysReq;
    case GDK_KEY_Escape:
        return Qt::Key_Escape;
    case GDK_KEY_KP_Delete:
    case GDK_KEY_Delete:
        return Qt::Key_Delete;
    case GDK_KEY_Multi_key:
        return Qt::Key_Multi_key;
    case GDK_KEY_Codeinput:
        return Qt::Key_Codeinput;
    case GDK_KEY_SingleCandidate:
        return Qt::Key_SingleCandidate;
    case GDK_KEY_MultipleCandidate:
        return Qt::Key_MultipleCandidate;
    case GDK_KEY_PreviousCandidate:
        return Qt::Key_PreviousCandidate;
    case GDK_KEY_Kanji:
        return Qt::Key_Kanji;
    case GDK_KEY_Muhenkan:
        return Qt::Key_Muhenkan;
    case GDK_KEY_Henkan:
        return Qt::Key_Henkan;
    case GDK_KEY_Romaji:
        return Qt::Key_Romaji;
    case GDK_KEY_Hiragana:
        return Qt::Key_Hiragana;
    case GDK_KEY_Katakana:
        return Qt::Key_Katakana;
    case GDK_KEY_Hiragana_Katakana:
        return Qt::Key_Hiragana_Katakana;
    case GDK_KEY_Zenkaku:
        return Qt::Key_Zenkaku;
    case GDK_KEY_Hankaku:
        return Qt::Key_Hankaku;
    case GDK_KEY_Zenkaku_Hankaku:
        return Qt::Key_Zenkaku_Hankaku;
    case GDK_KEY_Touroku:
        return Qt::Key_Touroku;
    case GDK_KEY_Massyo:
        return Qt::Key_Massyo;
    case GDK_KEY_Kana_Lock:
        return Qt::Key_Kana_Lock;
    case GDK_KEY_Kana_Shift:
        return Qt::Key_Kana_Shift;
    case GDK_KEY_Eisu_Shift:
        return Qt::Key_Eisu_Shift;
    case GDK_KEY_Eisu_toggle:
        return Qt::Key_Eisu_toggle;
    case GDK_KEY_KP_Home:
    case GDK_KEY_Home:
        return Qt::Key_Home;
    case GDK_KEY_KP_Left:
    case GDK_KEY_Left:
        return Qt::Key_Left;
    case GDK_KEY_KP_Up:
    case GDK_KEY_Up:
        return Qt::Key_Up;
    case GDK_KEY_KP_Right:
    case GDK_KEY_Right:
        return Qt::Key_Right;
    case GDK_KEY_KP_Down:
    case GDK_KEY_Down:
        return Qt::Key_Down;
    case GDK_KEY_KP_Page_Up:
    case GDK_KEY_Page_Up:
        return Qt::Key_PageUp;
    case GDK_KEY_KP_Page_Down:
    case GDK_KEY_Page_Down:
        return Qt::Key_PageDown;
    case GDK_KEY_KP_End:
    case GDK_KEY_End:
        return Qt::Key_End;
    case GDK_KEY_KP_Begin:
    case GDK_KEY_Begin:
        return Qt::Key_Home;
    case GDK_KEY_Select:
        return Qt::Key_Select;
    case GDK_KEY_Print:
        return Qt::Key_Print;
    case GDK_KEY_Execute:
        return Qt::Key_Execute;
    case GDK_KEY_KP_Insert:
    case GDK_KEY_Insert:
        return Qt::Key_Insert;
    case GDK_KEY_Menu:
        return Qt::Key_Menu;
    case GDK_KEY_Cancel:
        return Qt::Key_Cancel;
    case GDK_KEY_Help:
        return Qt::Key_Help;
    case GDK_KEY_Mode_switch:
        return Qt::Key_Mode_switch;
    case GDK_KEY_Num_Lock:
        return Qt::Key_NumLock;
    case GDK_KEY_F1:
    case GDK_KEY_KP_F1:
        return Qt::Key_F1;
    case GDK_KEY_F2:
    case GDK_KEY_KP_F2:
        return Qt::Key_F2;
    case GDK_KEY_F3:
    case GDK_KEY_KP_F3:
        return Qt::Key_F3;
    case GDK_KEY_F4:
    case GDK_KEY_KP_F4:
        return Qt::Key_F4;
    case GDK_KEY_F5:
        return Qt::Key_F5;
    case GDK_KEY_F6:
        return Qt::Key_F6;
    case GDK_KEY_F7:
        return Qt::Key_F7;
    case GDK_KEY_F8:
        return Qt::Key_F8;
    case GDK_KEY_F9:
        return Qt::Key_F9;
    case GDK_KEY_F10:
        return Qt::Key_F10;
    case GDK_KEY_F11:
        return Qt::Key_F11;
    case GDK_KEY_F12:
        return Qt::Key_F12;
    case GDK_KEY_F13:
        return Qt::Key_F13;
    case GDK_KEY_F14:
        return Qt::Key_F14;
    case GDK_KEY_F15:
        return Qt::Key_F15;
    case GDK_KEY_F16:
        return Qt::Key_F16;
    case GDK_KEY_F17:
        return Qt::Key_F17;
    case GDK_KEY_F18:
        return Qt::Key_F18;
    case GDK_KEY_F19:
        return Qt::Key_F19;
    case GDK_KEY_F20:
        return Qt::Key_F20;
    case GDK_KEY_F21:
        return Qt::Key_F21;
    case GDK_KEY_F22:
        return Qt::Key_F22;
    case GDK_KEY_F23:
        return Qt::Key_F23;
    case GDK_KEY_F24:
        return Qt::Key_F24;
    case GDK_KEY_F25:
        return Qt::Key_F25;
    case GDK_KEY_F26:
        return Qt::Key_F26;
    case GDK_KEY_F27:
        return Qt::Key_F27;
    case GDK_KEY_F28:
        return Qt::Key_F28;
    case GDK_KEY_F29:
        return Qt::Key_F29;
    case GDK_KEY_F30:
        return Qt::Key_F30;
    case GDK_KEY_F31:
        return Qt::Key_F31;
    case GDK_KEY_F32:
        return Qt::Key_F32;
    case GDK_KEY_F33:
        return Qt::Key_F33;
    case GDK_KEY_F34:
        return Qt::Key_F34;
    case GDK_KEY_F35:
        return Qt::Key_F35;
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
        return Qt::Key_Shift;
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
        return Qt::Key_Control;
    case GDK_KEY_Caps_Lock:
        return Qt::Key_CapsLock;
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
        return Qt::Key_Meta;
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
        return Qt::Key_Alt;
    case GDK_KEY_Super_L:
        return Qt::Key_Super_L;
    case GDK_KEY_Super_R:
        return Qt::Key_Super_R;
    case GDK_KEY_Hyper_L:
        return Qt::Key_Hyper_R;
    case GDK_KEY_Hyper_R:
        return Qt::Key_Hyper_R;
    case GDK_KEY_MonBrightnessUp:
        return Qt::Key_MonBrightnessUp;
    case GDK_KEY_MonBrightnessDown:
        return Qt::Key_MonBrightnessDown;
    case GDK_KEY_KbdLightOnOff:
        return Qt::Key_KeyboardLightOnOff;
    case GDK_KEY_KbdBrightnessUp:
        return Qt::Key_KeyboardBrightnessUp;
    case GDK_KEY_KbdBrightnessDown:
        return Qt::Key_KeyboardBrightnessDown;
    case GDK_KEY_Standby:
        return Qt::Key_Standby;
    case GDK_KEY_AudioLowerVolume:
        return Qt::Key_VolumeDown;
    case GDK_KEY_AudioMute:
        return Qt::Key_VolumeMute;
    case GDK_KEY_AudioRaiseVolume:
        return Qt::Key_VolumeUp;
    case GDK_KEY_AudioPlay:
        return Qt::Key_MediaPlay;
    case GDK_KEY_AudioStop:
        return Qt::Key_MediaStop;
    case GDK_KEY_AudioPrev:
        return Qt::Key_MediaPrevious;
    case GDK_KEY_AudioNext:
        return Qt::Key_MediaNext;
    case GDK_KEY_HomePage:
        return Qt::Key_HomePage;
    case GDK_KEY_Mail:
        return Qt::Key_LaunchMail;
    case GDK_KEY_Start:
        return Qt::Key_Standby;
    case GDK_KEY_Find:
    case GDK_KEY_Search:
        return Qt::Key_Search;
    case GDK_KEY_AudioRecord:
    case GDK_KEY_Calculator:
        return Qt::Key_Calculator;
    case GDK_KEY_Memo:
        return Qt::Key_Memo;
    case GDK_KEY_ToDoList:
        return Qt::Key_ToDoList;
    case GDK_KEY_Calendar:
        return Qt::Key_Calendar;
    case GDK_KEY_PowerDown:
        return Qt::Key_PowerDown;
    case GDK_KEY_ContrastAdjust:
        return Qt::Key_ContrastAdjust;
    case GDK_KEY_Back:
        return Qt::Key_Back;
    case GDK_KEY_Forward:
        return Qt::Key_Forward;
    case GDK_KEY_Stop:
        return Qt::Key_Stop;
    case GDK_KEY_Refresh:
        return Qt::Key_Refresh;
    case GDK_KEY_PowerOff:
        return Qt::Key_PowerOff;
    case GDK_KEY_WakeUp:
        return Qt::Key_WakeUp;
    case GDK_KEY_Eject:
        return Qt::Key_Eject;
    case GDK_KEY_ScreenSaver:
        return Qt::Key_ScreenSaver;
    case GDK_KEY_WWW:
        return Qt::Key_WWW;
    case GDK_KEY_Sleep:
        return Qt::Key_Sleep;
    case GDK_KEY_Favorites:
        return Qt::Key_Favorites;
    case GDK_KEY_AudioPause:
        return Qt::Key_MediaPause;
    case GDK_KEY_AudioMedia:
        return Qt::Key_LaunchMedia;
    case GDK_KEY_MyComputer:
        return Qt::Key_Launch0;
    case GDK_KEY_VendorHome:
        return Qt::Key_OfficeHome;
    case GDK_KEY_LightBulb:
        return Qt::Key_LightBulb;
    case GDK_KEY_Shop:
        return Qt::Key_Shop;
    case GDK_KEY_History:
        return Qt::Key_History;
    case GDK_KEY_OpenURL:
        return Qt::Key_OpenUrl;
    case GDK_KEY_AddFavorite:
        return Qt::Key_AddFavorite;
    case GDK_KEY_HotLinks:
        return Qt::Key_HotLinks;
    case GDK_KEY_BrightnessAdjust:
        return Qt::Key_BrightnessAdjust;
    case GDK_KEY_Finance:
        return Qt::Key_Finance;
    case GDK_KEY_Community:
        return Qt::Key_Community;
    case GDK_KEY_AudioRewind:
        return Qt::Key_AudioRewind;
    case GDK_KEY_BackForward:
        return Qt::Key_BackForward;
    case GDK_KEY_Launch0:
        return Qt::Key_Launch0;
    case GDK_KEY_Launch1:
        return Qt::Key_Launch1;
    case GDK_KEY_Launch2:
        return Qt::Key_Launch2;
    case GDK_KEY_Launch3:
        return Qt::Key_Launch3;
    case GDK_KEY_Launch4:
        return Qt::Key_Launch4;
    case GDK_KEY_Launch5:
        return Qt::Key_Launch5;
    case GDK_KEY_Launch6:
        return Qt::Key_Launch6;
    case GDK_KEY_Launch7:
        return Qt::Key_Launch7;
    case GDK_KEY_Launch8:
        return Qt::Key_Launch8;
    case GDK_KEY_Launch9:
        return Qt::Key_Launch9;
    case GDK_KEY_LaunchA:
        return Qt::Key_LaunchA;
    case GDK_KEY_LaunchB:
        return Qt::Key_LaunchB;
    case GDK_KEY_LaunchC:
        return Qt::Key_LaunchC;
    case GDK_KEY_LaunchD:
        return Qt::Key_LaunchD;
    case GDK_KEY_LaunchE:
        return Qt::Key_LaunchE;
    case GDK_KEY_LaunchF:
        return Qt::Key_LaunchF;
    case GDK_KEY_ApplicationLeft:
        return Qt::Key_ApplicationLeft;
    case GDK_KEY_ApplicationRight:
        return Qt::Key_ApplicationRight;
    case GDK_KEY_Book:
        return Qt::Key_Book;
    case GDK_KEY_CD:
        return Qt::Key_CD;
    case GDK_KEY_Close:
        return Qt::Key_Close;
    case GDK_KEY_Copy:
        return Qt::Key_Copy;
    case GDK_KEY_Cut:
        return Qt::Key_Cut;
    case GDK_KEY_Display:
        return Qt::Key_Display;
    case GDK_KEY_DOS:
        return Qt::Key_DOS;
    case GDK_KEY_Documents:
        return Qt::Key_Documents;
    case GDK_KEY_Excel:
        return Qt::Key_Excel;
    case GDK_KEY_Explorer:
        return Qt::Key_Explorer;
    case GDK_KEY_Game:
        return Qt::Key_Game;
    case GDK_KEY_Go:
        return Qt::Key_Go;
    case GDK_KEY_iTouch:
        return Qt::Key_iTouch;
    case GDK_KEY_LogOff:
        return Qt::Key_LogOff;
    case GDK_KEY_Market:
        return Qt::Key_Market;
    case GDK_KEY_Meeting:
        return Qt::Key_Meeting;
    case GDK_KEY_MenuKB:
        return Qt::Key_MenuKB;
    case GDK_KEY_MenuPB:
        return Qt::Key_MenuPB;
    case GDK_KEY_MySites:
        return Qt::Key_MySites;
    case GDK_KEY_News:
        return Qt::Key_News;
    case GDK_KEY_OfficeHome:
        return Qt::Key_OfficeHome;
    case GDK_KEY_Option:
        return Qt::Key_Option;
    case GDK_KEY_Paste:
        return Qt::Key_Paste;
    case GDK_KEY_Phone:
        return Qt::Key_Phone;
    case GDK_KEY_Reply:
        return Qt::Key_Reply;
    case GDK_KEY_Reload:
        return Qt::Key_Reload;
    case GDK_KEY_RotateWindows:
        return Qt::Key_RotateWindows;
    case GDK_KEY_RotationPB:
        return Qt::Key_RotationPB;
    case GDK_KEY_RotationKB:
        return Qt::Key_RotationKB;
    case GDK_KEY_Save:
        return Qt::Key_Save;
    case GDK_KEY_Send:
        return Qt::Key_Send;
    case GDK_KEY_Spell:
        return Qt::Key_Spell;
    case GDK_KEY_SplitScreen:
        return Qt::Key_SplitScreen;
    case GDK_KEY_Support:
        return Qt::Key_Support;
    case GDK_KEY_TaskPane:
        return Qt::Key_TaskPane;
    case GDK_KEY_Terminal:
        return Qt::Key_Terminal;
    case GDK_KEY_Tools:
        return Qt::Key_Tools;
    case GDK_KEY_Travel:
        return Qt::Key_Travel;
    case GDK_KEY_Video:
        return Qt::Key_Video;
    case GDK_KEY_Word:
        return Qt::Key_Word;
    case GDK_KEY_Xfer:
        return Qt::Key_Xfer;
    case GDK_KEY_ZoomIn:
        return Qt::Key_ZoomIn;
    case GDK_KEY_ZoomOut:
        return Qt::Key_ZoomOut;
    case GDK_KEY_Away:
        return Qt::Key_Away;
    case GDK_KEY_Messenger:
        return Qt::Key_Messenger;
    case GDK_KEY_WebCam:
        return Qt::Key_WebCam;
    case GDK_KEY_MailForward:
        return Qt::Key_MailForward;
    case GDK_KEY_Pictures:
        return Qt::Key_Pictures;
    case GDK_KEY_Music:
        return Qt::Key_Music;
    case GDK_KEY_Battery:
        return Qt::Key_Battery;
    case GDK_KEY_Bluetooth:
        return Qt::Key_Bluetooth;
    case GDK_KEY_WLAN:
        return Qt::Key_WLAN;
    case GDK_KEY_UWB:
        return Qt::Key_UWB;
    case GDK_KEY_AudioForward:
        return Qt::Key_AudioForward;
    case GDK_KEY_AudioRepeat:
        return Qt::Key_AudioRepeat;
    case GDK_KEY_AudioRandomPlay:
        return Qt::Key_AudioRandomPlay;
    case GDK_KEY_Subtitle:
        return Qt::Key_Subtitle;
    case GDK_KEY_AudioCycleTrack:
        return Qt::Key_AudioCycleTrack;
    case GDK_KEY_Time:
        return Qt::Key_Time;
    case GDK_KEY_View:
        return Qt::Key_View;
    case GDK_KEY_TopMenu:
        return Qt::Key_TopMenu;
    case GDK_KEY_Suspend:
        return Qt::Key_Suspend;
    case GDK_KEY_Hibernate:
        return Qt::Key_Hibernate;
    case GDK_KEY_ClearGrab:
        return Qt::Key_ClearGrab;
    }

    if (keyval < 256) {
        if (isprint(keyval))
            keyval = toupper(keyval);
    }
    return Qt::Key(keyval);
}

// ### keyvalToQtKey in reverse, ugh
// ### finish
guint qt_convertToGdkKeyval(Qt::Key qKey)
{
    switch (qKey) {
    case Qt::Key_Insert:
        return GDK_KEY_Insert;
    case Qt::Key_Delete:
        return GDK_KEY_Delete;
    case Qt::Key_Left:
        return GDK_KEY_Left;
    case Qt::Key_Right:
        return GDK_KEY_Right;
    case Qt::Key_Up:
        return GDK_KEY_Up;
    case Qt::Key_Down:
        return GDK_KEY_Down;
    case Qt::Key_Tab:
        return GDK_KEY_Tab;
    case Qt::Key_F1:
        return GDK_KEY_F1;
    case Qt::Key_F2:
        return GDK_KEY_F2;
    case Qt::Key_F3:
        return GDK_KEY_F3;
    case Qt::Key_F4:
        return GDK_KEY_F4;
    case Qt::Key_F5:
        return GDK_KEY_F5;
    case Qt::Key_F6:
        return GDK_KEY_F6;
    case Qt::Key_F7:
        return GDK_KEY_F7;
    case Qt::Key_F8:
        return GDK_KEY_F8;
    case Qt::Key_F9:
        return GDK_KEY_F9;
    case Qt::Key_F10:
        return GDK_KEY_F10;
    case Qt::Key_F11:
        return GDK_KEY_F11;
    case Qt::Key_F12:
        return GDK_KEY_F12;
    case Qt::Key_F13:
        return GDK_KEY_F13;
    case Qt::Key_F14:
        return GDK_KEY_F14;
    case Qt::Key_F15:
        return GDK_KEY_F15;
    case Qt::Key_F16:
        return GDK_KEY_F16;
    case Qt::Key_F17:
        return GDK_KEY_F17;
    case Qt::Key_F18:
        return GDK_KEY_F18;
    case Qt::Key_F19:
        return GDK_KEY_F19;
    case Qt::Key_F20:
        return GDK_KEY_F20;
    case Qt::Key_F21:
        return GDK_KEY_F21;
    }

    return (guint)qKey;
}

Qt::MouseButton qt_convertGButtonToQButton(guint button)
{
    Qt::MouseButton b = Qt::NoButton;
    switch (button) {
    case 1:
        b = Qt::LeftButton;
        break;
    case 2:
        b = Qt::MiddleButton;
        break;
    case 3:
        b = Qt::RightButton;
        break;
    case 4:
        b = Qt::ExtraButton1;
        break;
    case 5:
        b = Qt::ExtraButton2;
        break;
    case 6:
        b = Qt::ExtraButton3;
        break;
    case 7:
        b = Qt::ExtraButton4;
        break;
    case 8:
        b = Qt::ExtraButton5;
        break;
    case 9:
        b = Qt::ExtraButton6;
        break;
    case 10:
        b = Qt::ExtraButton7;
        break;
    case 11:
        b = Qt::ExtraButton8;
        break;
    case 12:
        b = Qt::ExtraButton9;
        break;
    case 13:
        b = Qt::ExtraButton10;
        break;
    case 14:
        b = Qt::ExtraButton11;
        break;
    case 15:
        b = Qt::ExtraButton12;
        break;
    case 16:
        b = Qt::ExtraButton13;
        break;
    case 17:
        b = Qt::ExtraButton14;
        break;
    case 18:
        b = Qt::ExtraButton15;
        break;
    case 19:
        b = Qt::ExtraButton16;
        break;
    case 20:
        b = Qt::ExtraButton17;
        break;
    case 21:
        b = Qt::ExtraButton18;
        break;
    case 22:
        b = Qt::ExtraButton19;
        break;
    case 23:
        b = Qt::ExtraButton20;
        break;
    case 24:
        b = Qt::ExtraButton21;
        break;
    case 25:
        b = Qt::ExtraButton22;
        break;
    case 26:
        b = Qt::ExtraButton23;
        break;
    case 27:
        b = Qt::ExtraButton24;
        break;
    default:
        qWarning() << "Unrecognized button" << button;
    }

    return b;
}

Qt::TouchPointState qt_convertToQtTouchPointState(GdkEventType type)
{
    switch (type) {
    case GDK_TOUCH_BEGIN:
        return Qt::TouchPointPressed;
    case GDK_TOUCH_UPDATE:
        return Qt::TouchPointMoved;
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
        return Qt::TouchPointReleased;
    default:
        Q_UNREACHABLE();
    }
}

cairo_region_t *qt_convertToCairoRegion(const QRegion &region)
{
    cairo_region_t *r = cairo_region_create();
    for (const QRect &qrect : region.rects()) {
        cairo_rectangle_int_t rect = { qrect.x(), qrect.y(), qrect.width(), qrect.height() };
        cairo_region_union_rectangle(r, &rect);
    }
    return r;
}


