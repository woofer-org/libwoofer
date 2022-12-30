# LibWoofer - The application back-end

## About

LibWoofer is the software's back-end, and built as a shared library.  The main
functionality of the full application is handled by this library and includes
things like managing songs, the song library, settings, integration with the
system and desktop but maybe the most important of all: choosing songs and
playing them.

This README will primarily focus on the practical side of things.  Information
about the project, its goals, targeted audience and more is described in the
project-wide README, read it first if you haven't already.

## Documentation

The documentation about getting started, technical details, adapted standards
and much more is shipped along with the project and located in the [doc](doc)
directory of the repository.  The documentation is also hosted on GitHub as the
repository's wiki.

## Cloning

If you want to start using a released version, download the source tarball from
the 'Releases' section of the repository on GitHub.

If you want to test the latest stable features, clone the Git repository and use
the 'master' branch using:

`git clone https://github.com/woofer-org/libwoofer.git -b master`

If you want to develop using the latest and greatest changes, use the 'develop'
branch:

`git clone https://github.com/woofer-org/libwoofer.git -b develop`

## Compiling

Executing

`./configure && make`

should be able to configure and compile the software on various systems and
environments and should only require minimal effort from the user.  Further
compilation instructions can be found in the [INSTALL](INSTALL) file or the
software's documentation.  Dependency packages required to compile can be found
in the documentation under section [System Packages](doc/System-Packages.md).

## Front-end linking

Using non-standard directories to compile and link the front-end software with
this shared/static library requires special compiler and linker flags that
specify where to find the necessary header files and binaries.  To simplify this
process a little bit, a file called [envsetup.sh](envsetup.sh) is written that
automatically sets these flags to the location of this repository.  All you need
to do is sourcing the file like:

`source ./envsetup.sh`

This will set the right flags in environmental variables of the current shell
session.  You can then follow the instructions of the front-end project on how
to set it up.

## Contributing

Every contribution helps improving the project, from small bug reports to
complete pull request that implement a feature, anything is appreciated!  To get
started with any type of contribution, read our
[code of conduct](CODE_OF_CONDUCT.md) and the
[contributing file](CONTRIBUTING.md).

## Copying

The source code is licensed under the terms of the GNU General Public License
version 3 or later.

The software's documentation is licensed under the terms of the GNU Free
Documentation License version 1.2 or later.

