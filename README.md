# introduction

gtkplatform is a Qt Platform Abstraction plugin providing Qt applications with
the capability to use gtk+ as a host toolkit, primarily intended for use on
Linux desktops.

That is: it lets Qt applications render with native gtk+ menus, and use gtk+ for
input (mouse, keyboard, touch), and getting window content on screen, the same
as it uses e.g. cocoa on macOS for instance.

Thanks to:

* Robin Burchell (initial idea & heavy lifting)
* John Brooks (rendering work and OpenGL implementation)
* Gunnar Sletta (all sorts of assistance and brainstorming)

Needless to say, if you're interested in the work you see here, feel free to
[get in touch](https://crimson.no).

## what this is

It's a way to get better integrated, consistent application behaviour on the
Linux desktop.

## what this is **not**

It's not the most performant way to run applications, and as a result, not well
suited for the embedded environment.

This is not to say that performance is a non-goal or that performance is _bad_.
Rather, I say this to emphasize that while I would like to see better
performance, the first priority for me is having a nice, consistent desktop
environment that I can use today.

## current state

_use with caution_.

What works:

* Showing, resizing, and hiding windows windows (all hopefully flicker-free)
* Rendering in those windows
    * Using QPainter
    * Using QOpenGLContext (on Wayland; X11 is is tracked at
      [#11](https://github.com/CrimsonAS/gtkplatform/issues/11))
    * A mix of OpenGL and software rendering in those windows (QOpenGLWidget, etc)
    * QtWebEngine (if patched, tracked at [#9](https://github.com/CrimsonAS/gtkplatform/issues/9)
* Simple clipboard interaction (text/image copying)
* Native gtk+ dialogs (taken from Qt)
* Native gtk+ menubar
* Notifications using libnotify
* Input events
    * Touch
    * Keyboard
    * Mouse, including smooth scroll events

What doesn't:

* Basically everything else.

# screenshots

Here's Qt Creator running with the gtk+ platform plugin:

![Creator with the gtk+ platform plugin](https://qtl.me/E3D02BB87DD13C40C2DEF08E2440B735.png)

If you'd like to see more, [go take a look at the wiki](https://github.com/CrimsonAS/gtkplatform/wiki).

# building

* Qt 5.7.1
* gtk+ 3.22
* libnotify 0.7.7

Newer versions used as needed. Older versions, probably not, but let's see.
These are all available in Fedora 26 now.

With dependencies installed:

* qmake
* make

(may need to make as root to ensure the plugin is installed in your Qt plugin
directory properly).

Then try launch something after setting QT_QPA_PLATFORM=gtk (or `-platform gtk`
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

  Right now, we're using libnotify, because using GtkApplication without using
  g_application_run dodesn't seem trivial. This means that we're not using the
  latest and greatest stuff, unfortunately. I'd like to fix this somehow.

* Drag and drop doesn't work (or: Accessibility, your_feature_here)

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

* **Q:** If you prefer gtk+, why don't you just use that?

  **A:** I don't necessarily prefer gtk+, I just happen to regularly use a GNOME
  desktop. Besides that, I don't want to rewrite every application I use on a day
  to day basis, nor do I think that is an especially productive use of anybody's time.

* **Q:** Why isn't this part of Qt?

  **A:** Firstly, it's easier to develop something that is rapidly changing
  out-of-tree. Secondly, I want this to be usable on my desktop _now_, not
  in 6-12 months time.

* **Q:** Why didn't you provide a gtk+ platform using Qt?

  **A:** Because I consider the gtk+ application experience to be better on my
  particular desktop. QtWayland is not of very good quality for desktop use: it
  doesn't support smooth trackpad scrolling, it has bad (more or less unusable)
  window decorations, it has no real font configuration support, and so on. The
  xcb experience is better, but still not great. Font choices are often wrong,
  and there's a lot of black flickering when resizing things, as well as it not
  being particularly future proof.

  In addition to that, I'm more familiar with Qt's internals, so going this
  direction was a lot easier for me than the opposite would have been, and I
  don't think out of tree ports are possible with gtk+.

* **Q:** Aren't you just giving up on Qt on the desktop?

  **A:** I don't think so. For a start, the Linux desktop (where gtk+ is used)
  is not a huge part of the computer audience of the world right now. This isn't
  going to make the Mac or macOS ports any worse. Nor does it inherently affect
  or damage the non-gtk+ Linux desktop world: I don't imagine that KDE desktops
  will want to make use of this code, for instance, because they have their own
  window decorations, their own theming and font handling, etc. This code is
  specifically designed for use in environments like the GNOME desktop.

  That having been said, I have to say that my own *personal* view is that Qt,
  while a great platform for embedded use and single purpose systems, is not
  doing especially well for desktop use.

  There's a lot of rough edges on QtWayland on desktop, a separate set of
  issues on the xcb side, and it seems like this situation has been more or
  less indefinite (at least, since I started using Linux on a day to day
  basis in 2008). This is my own attempt at improving that situation. Whether or
  not it will be successful, I don't know.

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

  More seriously, I haven't tested them at all yet. There's probably going to
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
