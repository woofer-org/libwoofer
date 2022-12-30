# Getting involved

## Overview

The development of the entire project takes place on GitHub
(https://github.com).  The source code is tracked in Git
(https://www.git-scm.com) using so-called 'commits'.  Anyone that contributes
can create commits on their local computer and open a pull request when they
have pushed their changes to their own fork.  The pull request is then reviewed
and merged into the repository's history when they satisfy certain requirements.
To keep the code consistent and at a high level of quality, everyone should
follow the specified coding conventions.

## Contributing

But even when you're not a programmer, you can help improving the project.
Whether you've found a bug or got an idea for a new feature, you can open an
issue on GitHub and then describe the bug or feature in detail to be reviewed.
Likewise, when you're familiar with Git, you can always help by improving
documentation, updating outdated information or fixing typos and then opening a
pull request.  Any help is appreciated!

There are some issue and pull request templates you can use to make it easier to
provide enough information so that others better understand your idea, feature
or bug report.

## Coding conventions

The Python philosophy states that:
* Beautiful is better than ugly.
* Explicit is better than implicit.
* Simple is better than complex.
* Complex is better than complicated.
* Flat is better than nested.
* Sparse is better than dense.
* Readability counts.

And this really should apply to everything you write.  This makes life so much
easier for the next guy that reads the code and, chances are, that next guy is
you.  So keep this in mind when making any changes to the code.  Aside from
that, here are some conventions to try to keep quality code.

In order of importance:
Mandatory - Encouraged - Recommended - Conventional - Optional

It is:
* Mandatory to use tabs instead of spaces for indention
* Recommended to use a tab size of 4
* Recommended to limit lines of code to 120 characters horizontally
* Encouraged to limit comment blocks to 80 characters horizontally
* Mandatory to limit lines to 80 characters in non-source documentation files
* Encouraged to use double spaces between sentences in comment blocks
* Mandatory to use double spaces in non-source documentation files
* Mandatory to start the name of function definitions in the first column
* Mandatory to keep matching squarely brackets in the same column
* Mandatory to always use squarely brackets for control statements
* Mandatory to use a space after `if`, `for` and `while` statements
* Mandatory to not use a space just after an opening bracket and just before the
  closing bracket
* Mandatory to use prefixes even in static functions or static variables
* Recommended to use the suffix `_cb` in function names that are used as
  callbacks
* Recommended to use the suffix `_rv` in return value parameters
* Mandatory to mention the use of `_cb` and `_rv` suffixes in a comment at the
  top of the source file (below the license notice)
* Encouraged to use GLib's standard types (gint, gchar, etc.)
* Mandatory to use `void` in function declarations and definitions that take no
  parameters
* Recommended to cast from void pointers
* Mandatory to initialize const variables
* Conventional to typedef structs, unions and enums
* Conventional to use enum typedefs before other typedefs
* Conventional to use simple comments before code blocks and/or functions
* Conventional to keep lines relatively simple by splitting them across multiple
  lines
* Encouraged to avoid gotos
* Encouraged to avoid inline functions
* Encouraged to avoid the creation of complex macros
* Encouraged to avoid global variables
* Conventional to define setters before getters
* Encouraged to refer to GObject properties in the respective getter/setter
  description
* Encouraged to only use `/* */` comments in header files
* Recommended to simply reference to a note in a comment when the explanation in
  the note is used multiple times throughout the source file
* Recommended to add a small, inline comment with the parameter name when using
  the value NULL in a function call

But most importantly, keep styling consistent; meaning adapt to the style that
is already applied to the software code.

