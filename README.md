vswm - very stupid window manager
=================================

Probably the most stupid window manager ever created, written over an ancient relic of a library called Xlib -- a library so old that it preceeded the birth of planet Earth itself.

- There are no workspaces.
- All windows are the same size.
- Windows can not be moved or resized (other than fullscreen, but all windows are affected.
- Only one window is visible at a time.
- This certainly isn't for everyone.


Keybindings
-----------

MOD4 + Tab               focus next window
MOD4 + Shift + Tab       focus prev window
MOD4 + q                 kill window
MOD4 + Shift + r         refresh wm [1] [2]
MOD4 + return            fullscreen mode

MOD4 + w                 qutebrowser [3]
MOD4 + t                 default tmux session [4]
MOD4 + space             dmenu_run

XF86_MonBrightnessDown   light -U 5
XF86_MonBrightnessUp     light -A 5
XF86_AudioLowerVolume    pamixer -d 5
XF86_AudioRaiseVolume    pamixer -i 5
XF86_AudioMute           pamixer -t

[1] Resize and reposition windows. Useful when connecting or disconnecting an external monitor, if e.g. screen size differ.

[2] This should no longer be necessary as the function is called whenever focus is shifted or a new window is mapped.

[3] Currently requires a hacky solution to avoid a 'No Child Processes' error.

[4] Highly recommend changing this keybinding to just open a terminal, or create a script called tmux-default in your $PATH that creates/attaches a tmux session.


Configuration
-------------

Modify the keybindings to your liking.


Dependencies
------------

You need the Xlib header files.


Installation
------------

Clean.

    $ make clean

Compile.

    $ make

Install.

    $ make install

All in one go.

    $ make clean install

Uninstall.

    $ make uninstall

You may need to run install as root.
DESTDIR and PREFIX are supported.


Credits
-------

fehawen for all of his previous work on this project and for inspiring me to continue attempting to learn how to write a window manager.

i3: https://github.com/i3/i3
dwm: https://git.suckless.org/dwm
sowm: https://github.com/dylanaraps/sowm
berry: https://github.com/JLErvin/berry
tinywm: http://incise.org/tinywm.html
katriawm: https://www.uninformativ.de/git/katriawm
