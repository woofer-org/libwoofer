# Front-end creation

## Overview

In general, the software of a front-end has to do very little in order to run
the application.  It should initialize an application object (provided by the
back-end), connect to signals and then run it to set everything in action.  The
important signals to connect to are:
* Startup: This is when the application is going to initialize (startup) its
  internal modules.  You can use this to, for example, initialize the widget
  toolkit.
* Activate: This is when the application should create a new graphical window or
  show its existing one (in the case of a graphical application).  Note that
  this can be fired multiple times but only one main window should exist at any
  time, so act accordingly.
* Shutdown: This is when the application is going to cleanup and shutdown its
  modules.  You can use this to, for example, destroy the existing widgets
  (again, in the case of a graphical application).

## Languages

The back-end is written in the programming language C, but this does not
necessarily mean the front-end needs to be written in the same language.  That's
where language bindings come in.  These binding provide the translation layer
between the C code of the back-end and any other language.  The language
bindings may be not available for every language and support for new languages
will be expanded over-time.

## A simple example: headless runner

Here is a simple example that sets up the application back-end and runs the
application.  Everything else is handled by the back-end and includes
initializing the remote interface over D-Bus.

```c
#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <woofer/app.h>

int
main(int argc, char *argv[])
{
	GApplication *app;
	int status;

	app = (GApplication *) g_object_new(WF_TYPE_APP, NULL);

	status = g_application_run(app, (gint) argc, (gchar **) argv);

	g_object_unref(app);

	return status;
}
```

When saved as `main.c`, this code can (assuming that the right compiler flags
are set) be compiled with:

`gcc main.c -o woofer -lm -lwoofer $(pkg-config --cflags --libs glib-2.0 gio-2.0
gobject-2.0)`

