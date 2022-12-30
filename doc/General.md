# General information

## Disclaimer

Please be aware that the project is still heavily under development.  Things can
change a lot and can change frequently.  As such, the current documentation may
have become inaccurate.  If you notice something incorrect or incomplete, please
open an issue on GitHub.

## Overview

LibWoofer is the application back-end of the Woofer Music Player.  Written in
pure C, it is designed to be fast and flexible.  Most of the application work is
done by this library (regardless of the front-end), which gives end-users the
ability to choose and change the front-end to their liking, while keeping the
behavior and user data of the application the same.  The software choosing songs
for you and being able to configure how that is done is the main thing that
makes the software so unique, plus you can use it however you like: using a
graphical environment, in a command-line or even headless.  The choice is yours.

## Definitions

The *application back-end* is built as a shared library and it is what does most
of the application's work.  This is sometimes referred to as the
*application core*, *application library* or just simply *library* (although do
not be confused by a part of the application called the *song library*).  The
user needs something to interact with the application: an *interface*, or
sometimes referred to as the *application front-end*.  These are separate
projects that can focus on different aspects of the user experience: graphical
or command-line, simplicity or customization.  It is up to the user to choose
one they like.

## Motivation for the creation of the library

Initially, the complete application was comprised of the library and the GTK
interface in one single application.  Then, the idea of adding a command-line
interface introduced some problems, primarily when compiling and running.  If
you want to be able to use the command-line interface without any window manager
at all, the application should run without any GTK elements initializing.  Also,
if you do not have any windows manager and graphical toolkits installed, the
application is still dependent on the GTK libraries.  Customizing during
compilation would work, but complicate the project significantly, so the best
solution was to split the application into an application library that would do
all the application work, and an interface that would take care of the user
experience.  This also opened up the possibility to create other types of
interfaces using different toolkits.

## Front-end projects

Separate front-end projects provide the software for an interface the user can
use to interact with the application.  The projects are free to focus on what
they find important, and can thus differ in user experience.  Some provide a
graphical interface, others a command-line application.  But the key point here
is that they all have the same underlying behavior, song library and settings.
While one application instance (and one window, in case of a graphical
interface) should exist at a time, the user is free to swap their front-end as
they wish and so this gives the end-user control over how they want to use the
software.

## Versioning

The software of the application library uses Semantic Versioning 2.0.0
(https://semver.org).  Simply put, the version consists of three numbers: major,
minor and patch. 'Major' is increased when API breaking changes are introduced.
'Minor' is increased when functionality is added but when it is still backward
compatible.  'Patch' is increased when mistakes or bugs are fixed, while not
introducing any functionality changes or new features.

