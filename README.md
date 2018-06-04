# introduction

gtkplatform is a Qt Platform Abstraction plugin providing Qt applications with
the capability to use gtk+ as a host toolkit, primarily intended for use on
Linux desktops.

That is: it lets Qt applications render with native gtk+ menus, and use gtk+ for
input (mouse, keyboard, touch), and getting window content on screen, the same
as it uses e.g. cocoa on macOS for instance.

Thanks to:

* Robin Burchell (@rburchell, initial idea & heavy lifting)
* John Brooks (@special, rendering work and OpenGL implementation)
* Gunnar Sletta (@sletta, all sorts of assistance and brainstorming)
* Donald Carr (@sirspudd, Arch Linux packaging)

If you'd like to have a chat with us, feel free to
[drop in on Telegram](https://t.me/joinchat/FlwGHw366p2Z9tBZ_f1yTA).

## what this is

It's a way to get better integrated, consistent application behaviour on the
Linux desktop.

## what this is **not**

It's not the most performant way to run applications, and as a result, not well
suited for the embedded environment. This is particularly noticable with QtQuick
applications, as they make use of OpenGL. This goes through a copy step: the
scene is drawn offscreen, copied, and uploaded to the gtk+ window, which is
rather inefficient. Hopefully, gtk+ will grow API to allow this to be done
better in the future.

## current state

What works:

* Showing, resizing, and hiding windows windows (all hopefully flicker-free)
* Rendering in those windows
    * Using QPainter
    * Using QOpenGLContext
    * A mix of OpenGL and software rendering in those windows (QOpenGLWidget, etc)
    * QtWebEngine (if patched, tracked at [#9](https://github.com/CrimsonAS/gtkplatform/issues/9))
* Simple clipboard interaction (text/image copying)
* Native gtk+ dialogs (taken from Qt)
* Native gtk+ menubar
* Notifications using libnotify
* Input events
    * Touch
    * Keyboard
    * Mouse, including smooth scroll events

Not everything does work, though. See the known issues section.

# screenshots

Here's Qt Creator running with the gtk+ platform plugin:

![Creator with the gtk+ platform plugin](https://qtl.me/E3D02BB87DD13C40C2DEF08E2440B735.png)

If you'd like to see more, [go take a look at the wiki](https://github.com/CrimsonAS/gtkplatform/wiki).

# building

These are the versions I test with.

* Qt 5.10.1
* gtk+ 3.22.30
* libnotify 0.7.7

These are all available in Fedora 28, which is where I do testing/development.
Good support is also available on Arch Linux, using [the package generously
maintained by @sirspudd](https://aur.archlinux.org/packages/qt-gtk-platform-plugin/).

Other distributions may, or may not work, but I don't have any involvement with them.

With dependencies installed:

* `qmake`
* `make`
* `make install` (as root)

Then try launch something after setting `QT_QPA_PLATFORM=gtk` (or `-platform gtk`
as a command line option)

# history

Qt is pretty portable. I don't think there's any doubt to this statement; just
take a look at the vast myriad of platform ports out there. It runs on macOS,
Windows, even Haiku. It's everywhere.

There's a bit of a fly in the ointment, though: on the Linux desktop, things
aren't quite so well defined. There isn't a "sanctioned" platform toolkit.
As a result, Qt has to do quite a lot of heavy lifting itself, and this doesn't
always result in something that is too well integrated with the host desktop
system.

As an additional problem, the Linux desktop world is changing. The stability
(which some may consider stagnation) of xcb has been giving way to the rise of
Wayland. In many ways, this change has been beneficial: it's a lot harder to
introduce some bad graphical glitches like flicker on resize. On the other hand,
it introduces a host of its own brand new problems.

Even discounting all of these as solvable problems on top of the usual
things like reasonably performant flicker-free graphics that we ought to have
and ought to be able to take for granted, there's the root issue
that there is a significant amount of duplicate work going on here: any new
development has to be solved in (at least) two major toolkits.

So with this background, we get to the situation that lead to this project. A
while ago, I moved from macOS back to Linux as my day to day desktop system, and
quickly experienced frustration with a myriad of bad, very user-visible bugs
on Linux like variances in how high DPI is dealt with, font sizing and selection
that wasn't identical, black flicker on resizing, bad trackpad scroll behaviour,
the reliance of Qt applications on xwayland rather than being first class
Wayland citizens, etc.

This project aims to help mitigate those issues.

# Known Issues

* Popup positioning (like combo box dropdowns) will often be wrong.

  When running on Wayland, this platform plugin will not allow absolute
  positioning of a window in global coordinate space. Instead, popups are
  positioned relative to their parent window. This usually manifests as windows
  appearing very far away from where they ought to have triggered because they
  failed to set a parent.

* Notifications don't work right.

  Right now, we're using libnotify, because using `GtkApplication` without using
  `g_application_run` doesn't seem trivial. This means that we're not using the
  latest and greatest stuff, unfortunately. I'd like to fix this somehow.

* Accessibility doesn't work
* Drag and drop doesn't work

  It isn't written yet. There's rather a lot of features like that, actually.
  I'm sure it will improve with time.

* My menu shortcuts have funny things in them

  The mapping of GTK+ keys to Qt keys is incomplete. See [#8](https://github.com/CrimsonAS/gtkplatform/issues/8).

* QtWebEngine doesn't work out of the box

  Correct. Currently there's a hardcoded "whitelist" of platform plugins that
  will work. The patch to make it work is quite trivial, see `src/core/content_browser_client_qt.cpp`,
  and make it request "eglcontext" unconditionally (or when using the "gtk"
  plugin). See [#9](https://github.com/CrimsonAS/gtkplatform/issues/9).

# FAQ

* **Q:** But my desktop works just fine. I don't want to use this.

  **A:** That's fine. Keep using what you are using today, and pretend this
  doesn't exist.

* **Q:** Why is this any better than a theme for Qt which looks like a gtk+ theme?

  **A:** These are separate, but related concerns. If I was just interested in
  getting the contents of a window to look like the contents of a Qt window,
  then sure, a theme/style plugin alone would be more than sufficient.

  More importantly than the contents of the window, though, I want consistency
  on the level of things underneath theming too. For instance, using this, you
  get transparent Wayland support that looks and works good *right now* without
  having to fix the numerous desktop-related pieces that are missing from QtWayland.
  You get consistent high DPI support. You get window resizing that doesn't
  flicker, unlike that of the xcb platform plugin.

  Longer term, I have even bigger goals than this. I want to be able to use
  platform-native features like GNotification, GtkHeaderBar, app menus, and more.
  I'd also like to look into mapping GtkGesture into something that Qt applications
  can make use of, so pinch/rotate/etc all work in the same way across all desktop
  applications.

  Most of this isn't realised yet, as for the time being I'm focusing on the
  "basics", but in the longer term I expect it will come. In the short term,
  this is about a more consistent, more usable out-of-the-box desktop
  experience.

* **Q:** Are you changing Qt's API?

  **A:** No. Your existing Qt applications of today will work with this with the
  same amount of tweaking they might need to make when running on a new platform
  like macOS or Wayland. We may provide some helpers to allow using some gtk+
  specific features in future, like GtkHeaderBar, though.

* **Q:** Why are you doing this? Qt supports _my pet feature_ better.

  **A:** That might be true. The main problem here is inconsistencies between
  different applications on my desktop. I want everything to look, and act the
  same. And maybe once I have that, we can focus on improving everything at once
  rather than fighting over who has the better toolkit.

* **Q:** Why isn't this part of Qt?

  **A:** Firstly, it's easier to develop something that is rapidly changing
  out-of-tree. Secondly, I want this to be usable on my desktop _now_, not
  in 6-12 months time.

* **Q:** There's no system tray icon support

  **A:** [Correct](https://bugzilla.gnome.org/show_bug.cgi?id=785956).

  Given that GtkStatusIcon is deprecated, and the system tray is a dead concept inside GNOME, I don't think this warrants supporting.

* **Q:** How does high DPI support work?

  **A:** We're using gtk+, so, it works the same way it does there. Window size
  etc is reported in units that aren't real pixels. The window content is drawn
  by scaling those sizes by the appropriate amount into a larger buffer, which
  is then drawn by gtk+.

* **Q:** How do multiple monitors work?

  **A:** They probably don't.

  More seriously, I haven't tested them much yet. There's probably going to
  be a lot of bugs there. When all that stabilizes, though, it should work the
  same as it does with gtk+ applications.

* **Q:** My application doesn't work!

  **A:** Not exactly a question, but: many applications rely on features of the
  underlying platform at build time. This of course won't work out of the box in
  many cases, sometimes due to missing mapping of a feature, and sometimes
  because it's outright not possible.

  It might be a bug in gtkplatform, or it might be that the application requires
  adaptation.

* **Q:** Does this render using gtk+?

  **A:** Sort of. With the exception of menu bars on windows, the content of the
  window is entirely rendered by Qt right now. Once Qt is done with that, it
  passes over to our code, and we use gtk+ to get that content on screen as
  well as to provide some of the desktop's settings, like font settings.

  So the reason that widgets look "native" is for the most part not down to the
  work that we've done, but rather the work of the people who wrote the Adwaita
  theme plugin. This having been said, it might be an area of interest to look
  at the possibility of using Pango to render text in the future, rather than
  fontconfig/freetype.

* **Q:** What technologies does this build on?

  **A:** The "glib" event dispatcher, originally written by [Bradley Hughes](https://blog.qt.io/blog/2006/02/24/qt-and-glib/)
  during his time working on Qt has proven very useful. Without that, this
  would have been a more difficult task.
  The ["adwaita" QStyle plugin](https://github.com/MartinBriza/adwaita-qt) is
  also great in that it helps the contents of windows blend in very well.

  Of course, none of this would be possible without the work of everyone in the
  greater Linux desktop community, too, particularly the gtk+ and Qt contributors.
