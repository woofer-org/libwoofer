# Technical details

## Overview

The source code of the application library is written in the programming
language C.  Because of this, each source file naturally has a corresponding
header file.  These combined are a so called 'module'.  Each module should be
responsible for one thing and one thing only.  This is called a modular design
and it makes a project more flexible and easier to maintain.  The 'app' module
is the top-level module and is responsible for globally managing the
application.  A handful of modules form the fundamentals of the application.
These 'core modules' are: app, song, intelligence, settings and library and
front-ends will primarily be interacting with these modules.

## Dependencies

One of the goals is to provide flexibility and this is done by only using
widely-adopted dependencies.  These in row depend on a lot of other (indirect)
system dependencies, but it shouldn't have any negative consequences.  The
required, direct dependencies are:
* GLib 2.0 (including GIO and GObject) provide low-level routines, file
  utilities and a type and object system.
* GStreamer 1.0 provides the media player framework.  Note that you need the
  plugin sets 'core', 'base' and 'good'.

## Adapted standards and specifications

Nothing is worse than inconsistency, so to integrate the application into a
variety of systems, it is best to follow specifications that try to streamline
systems and desktops.  Some are built-in by low-level dependencies, some are
followed by the application library and a few are encouraged to be adapted to by
front-end implementations.

### XDG (Freedesktop)

#### Base Directory Specification

Specifies where certain files should be located.

Environmental variables that may specify paths to write certain files to:
* `$XDG_DATA_HOME`: User-specific directory where data files can be stored
  (defaults to `$HOME/.local/share`)
* `$XDG_CONFIG_HOME`: User-specific directory where configuration files can be
  stored (defaults to `$HOME/.config`)
* `$XDG_STATE_HOME`: User-specific directory where state data can be stored
  (defaults to `$HOME/.local/state`)
* `$XDG_CACHE_HOME`: User-specific directory where (non-essential) cache data
  can be stored (defaults to `$HOME/.cache`)
* `$XDG_RUNTIME_HOME`: User-specific directory where runtime files can be stored
  (no default specified; `$XDG_CACHE_HOME` is used as fallback)
* `$XDG_DATA_DIRS`: Base directories to search for data files (defaults to
  `/usr/local/share/:/usr/share/`)
* `$XDG_CONFIG_DIRS`: Base directories to search for configuration files
  (defaults to /etc/xdg)

The paths and variables defined in this specification are implemented by GLib
and are thus automatically followed by the application back-end.  Front-end
implementations are strongly encouraged to follow them as well.

Reference:
https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html

#### Desktop Entry Specification

Specifies the syntax of desktop entry files and where to they should be located.

This specification should be followed by front-ends that wish to ship with a
desktop entry file.  A template is provided in the /data/templates directory of
the repository.

Reference:
https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html

#### Icon Naming Specification

Specifies how themed icons should be named and defines some default icon names.
These can be used for application icons, hardware devices, action icons used in
graphical interfaces and much more.

This specification should be followed by front-ends that provide a graphical
interface.  When doing so, themed interface icons are consistent across
applications and improve the overall aesthetics of the user's desktop.

Reference:
https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html

#### Media Player Remote Interfacing Specification (MPRIS)

Specifies a few D-Bus interfaces that media applications can implement to
provide media controls in the desktop environment.

An important thing to note is that applications following this specification
provide the D-Bus interfaces to clients.  The application serves as the
server-side and the desktop environment is the client interacting with the
interfaces.

The application back-end implements the interfaces defined in this
specification.  It uses GDBus and is activated whenever D-Bus is available.  No
action is required by front-end implementations.

Reference:
https://specifications.freedesktop.org/mpris-spec/latest

#### Desktop Menu Specification

Specifies how to add applications to a desktop application menu by providing
some metadata files.

This specification should be followed by front-ends that wish to add an
application menu item (usually only useful for graphical interfaces).

Reference:
https://specifications.freedesktop.org/menu-spec/menu-spec-latest.html

#### Desktop Notifications Specification

Specifies how to send desktop notifications via D-Bus.  GIO provides built-in
support for desktop notifications.

Reference:
https://specifications.freedesktop.org/notification-spec/notification-spec-latest.html

### AppStream (Freedesktop)

AppStream is part of FreeDesktop and is a specification to be used by software
components to provide consistent metadata that can be used for example by
software centers to know about the software available.  The metadata is written
in XML files named `<id>.metadata.xml`, with `<id>` being the unique identifier.
The specified location for these files is `/usr/share/metainfo/`

The application library provides a file with 'Generic' metadata and provides a
template in the /data/templates directory of the repository that can be used by
front-end projects.

Reference:
https://www.freedesktop.org/software/appstream/docs/

## Intelligence

The algorithm lives in the 'intelligence' module of the code.  In that module,
there are multiple functions that process just one thing, but these functions
stringed together creates some form of artificial intelligence and machine
learning that ultimately chooses a song to play for the user.  The procedure
itself is pretty simple and involves a few steps that can be broken down into
three parts:
* Filtering the complete collection of songs
* Determining the probability of each individual song
* Drawing one song as the winner

The latter two do relatively little and involve some mathematics.  The process
of filtering involves removing an undefined number of items (ideally not more
than the total amount of songs present in the song collection) that do not match
the configured criteria.  This is where the song choosing magic starts; with a
list of all known songs.

### Filtering

The filtering process starts with filtering by availability.  It does not make
sense to choose a song, only to report to the user that it isn't playable.  It
is the responsibility of the user to check that all songs in the song library do
indeed exist and are readable.

After that, songs are filtered based on their artist.  A list of artists of
recently played songs is collected and any song that matches any of the artists
in that list, are filtered out.  This creates the behavior that does not play
any songs of the same artist close after one another.

Then, songs that have been recently played are filtered out, because we simply
don't want to play them for a while.  Finally, the list with remaining songs is
finetuned by filtering out any songs that simply do not satisfy certain
statistical values.

At this point all that is left is a (hopefully non-empty) list of qualified
songs.

### Probabilities

For each qualified song, the chance of winning is determined by using the
gathered statistics to calculate a number.  This number is like the amount of
entries a person has in a lottery.  The higher the numbers (or entries), the
higher the chance of winning because the ratio of entries to total entries is
higher.

### Choosing

When all chances are determined, a random number is chosen and limited to the
range of the total amount of entries.  From the front to the back of the list,
the individual entries are summed up, one by one, until it is higher than the
drawn number.  At that point, the song in the current iteration is the winning
song and it is returned back to the application, ready to be played.

## GStreamer utilization

GStreamer is the multimedia framework used to, simply put, convert audio files
into sound waves that come out of your speakers.  GStreamer primarily provides
the function calls and structure that is implemented in the application (1),
plugins to do many different things with files, data and much more (2), and a
pipeline architecture to create a data flow, from source to sink (3).

In the foundation of the framework are *Elements*.  These elements can be
provided by GStreamer itself or from plugins and they can process many different
forms of data.  It is the choice of elements that makes the application do what
it needs to do.

### Elements

Making an application that is built with efficiency and flexibility in mind, the
choice of elements is very important.  Generally, we only need a couple of
elements:
* A 'source' element that can read any file containing audio, via any protocol
  supported by the operating system.  For this, we create the appropriate source
  element on-the-fly before playing a given file.  The exact element is
  determined by GStreamer, but this is usually 'filesrc' when playing a local,
  on-disk file.
* A 'decoder' that decodes (or converts) the file (or source) into useful data
  that can be processed by other elements. This is a fixed 'decodebin' that is
  created on application startup and exists until the application shuts down.
* A 'sink' element that outputs the processed data to the sound services of the
  system, so it can be processed further by the sound card or other sound
  devices to produce sound from the speakers.  What happens to the data after
  the sink element is out of the scope of the application and GStreamer.  For
  this sink element, we use a fixed 'playsink' that is configured to *only*
  process audio, not video (in the case that a video file is playing).  This
  will save a lot of processing power when playing a video file, as the video
  doesn't have to be rendered on-screen (it is a music player after all).

## The song library

The song library is the collection of audio files that are known by the
application.  This collection is organized in a linked list, initially sorted by
the time it was added, then alphabetically (in the case of adding multiple at
once), and written to disk as a regular text file.  This file can be freely
edited manually without the need of the application.  The syntax is the same as
specified in the Desktop Entry Specification.

### Song objects

Once initialized during startup, every item in the song library lives in the
system's memory as a song object, linked together to form a list that is in the
same order as the items in the song library file.  Every song (object) in the
library has its own set of statistics.  Changes to songs or the list are
regularly written back to disk (and always when the application is about to shut
down).

### Incognito

When enabled, incognito prevent the application from updating statistics of any
song, so you can freely start, stop or change the playback without impacting the
applications algorithm.

