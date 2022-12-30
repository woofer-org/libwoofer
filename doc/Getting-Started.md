# Getting started

## Introduction

If you just want to get started using the software as a music player, you should
pick a front-end interface and follow the instructions of the respective project
on how to get started.  If you want to start testing, contributing or
developing, you're better off compiling yourself.

## Obtaining the software

The easiest way to get the software, by far, is by using your distribution
software repository.  A quick search in the package manager should be all you
need.  If you do not get any results, chances are your distribution doesn't ship
any compiled binaries.  In that case, you will have to compile the source code
yourself.  In case it does have a package available, you can install that to use
it with any front-end interface.  Also install the dev package (if available) if
you want to compile the front-end yourself and install the doc package to have
all references and documentation installed on your system.

If the packages aren't available on your system or are outdated, you can
download the latest release from GitHub.  These are thoroughly tested and packed
into a compressed archive for you to download, extract and compile.  Just go to
the 'Releases' section of the project on GitHub.  You can also clone the Git
repository to run the latest development version, either the somewhat tested
'master' branch or the bleeding edge 'develop' branch.  Either way, if you have
Git installed, using `git clone http://github.com/woofer-org/libwoofer.git`
should get you a clone of the repository, ready to be compiled or improved.

## Compiling

To make the software flexible and universal, it should be able to run on a wide
variety of systems.  To achieve that, only a few, widely adopted and supported
dependencies and utilities are required.  These can vary somewhat from system to
system, but generally speaking you'll need a C compiler (one that supports at
least the C99 standard), (GNU) Make, pkg-config, GLib 2.0 and GStreamer 1.0 with
its core and base set of plugins.

The preferred way to compile is to use the well-known `./configure && make`.
The generated configure script from Autoconf will run a couple of tests and
search for the right dependencies.  When it finishes successfully, a Makefile is
generated that can be used by Make to compile the source code.

## Installing (optional)

Installing is not required to compile, link or use a front-end.  With the proper
environmental variables set, the right files can easily be found by the compiler
and runtime binaries.  The supplied `envsetup.sh` script can help with this.
More information can be found in the section about compiling a front-end.

But if you want to install the compiled library, header files and a few other
files into your system, executing `sudo make install` is all you need to do.
Keep in mind that when you have configured the project with `./configure` and no
arguments, the default prefix is set to `/usr/local`.  This is the base
directory under which the compiled and configured files will be installed.  By
proving `./configure` with the argument `--prefix=/path`, you can specify a
different prefix to use when installing.  For example, to install in the same
way as system packages, use `./configure --prefix=/usr`.

## Compile and link the front-end binary

After compiling the application library, it is time to compile the front-end
application and link it against the library (assuming the source code of the
front-end is written in an ahead-of-time compiled language like C).  Read the
instructions of the front-end of your choice on how to set it up.

To simplify the process of linking against the application library, a shell
script called `envsetup.sh` is written that automatically sets the compiler
flags to the right values so it is easier to compile and link the binaries.
All you need to do is source the script in your current shell session with
`source ./envsetup.sh`, go the repository of the front-end and execute the
documented commands to compile and link.

## Using

Depending a little on what front-end is used, but on first run, the song library
will be empty (obviously).  Songs will never be added automatically, you will
have to do that yourself.  You can add individual songs or choose a directory to
be scanned.  Any files that seem like media files will be added to the song
library.  When finished, all discovered songs are written to a new song library
file located in the users config directory (see `XDG_CONFIG_HOME` in the section
about adapted specifications).  This file can be edited manually with a text
editor when the software is not running.

The song library contains all songs that the software knows about and uses to
choose songs.  It is not comparable with playlists in that the playing order is
determined by the configurable algorithm.  Also, you only have one library;
tweak the settings to make it play the music you want (although nothing stops
you from creating multiple files and manually loading one).

Each item in the library is a song with a unique URI and statistics.  When you
delete any song from the library, all of its statistics are lost, so be careful
when doing so.

## First run

Before you start listening to any music, open up the software's settings (either
via the interface or by editing the settings file when the software is closed)
and configure it to your liking.  The default values should be sufficient to get
started, but it is a good idea to get familiar with the settings and how to use
them.  More details about how song choosing works and how to tweak it is
described in section 'Intelligence'.

## Enjoying

Now that you've added your songs to the library and have adjusted the settings
to your liking, it is time to play some music! Hit the play button and let the
software choose a song for you.  Don't like it? By skipping it, the statistics
of the song are updated to remember that you have skipped it and probably didn't
like it.  When configured correctly, those songs are played less and less (or
never chosen again), while playing songs that you like (by listening to it until
the end) more and more.  This does however mean that the software needs some
time to get to know your preference (by gathering statistics) and thus to
determine what you like and dislike.

