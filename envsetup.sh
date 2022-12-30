# SPDX-License-Identifier: GPL-3.0-or-later
#
# envsetup.sh  This file is part of LibWoofer
# Copyright (C) 2022  Quico Augustijn
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed "as is" in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  If your
# computer no longer boots, divides by 0 or explodes, you are the only
# one responsible.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# version 3 along with this library.  If not, see
# <https://www.gnu.org/licenses/gpl-3.0.html>.
#
#
# Source this file in your running shell to automatically set the right
# compiler and linker flags.  This makes compiling, linking and running
# front-ends very simple without installing anything.

REALPATH=`realpath "${0}"`
DIR=`dirname "${REALPATH}"`

export LD_LIBRARY_PATH="${DIR}/build"
export CFLAGS="-I${DIR}/src"
export LDFLAGS="-L${DIR}/build"
