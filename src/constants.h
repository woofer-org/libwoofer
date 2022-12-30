/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * constants.h  This file is part of LibWoofer
 * Copyright (C) 2021, 2022  Quico Augustijn
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed "as is" in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  If your
 * computer no longer boots, divides by 0 or explodes, you are the only
 * one responsible.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this library.  If not, see
 * <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

#ifndef __WF_CONSTANTS__
#define __WF_CONSTANTS__

/* Application constants */
#define WF_NAME "Woofer"
#define WF_TAG "woofer"
#define WF_LIB_TAG "lib" WF_TAG
#define WF_DISPLAY_NAME WF_NAME " Music Player"
#define WF_ICON_NAME WF_TAG
#define WF_ID "org." WF_TAG
#define WF_PATH "/org/" WF_TAG
#define WF_VERSION "0.1.0"
#define WF_WEBSITE "https://github.com/woofer-org"
#define WF_SUMMARY "The configurable intelligent music player"
#define WF_DESCRIPTION "Flexible, efficient and universal music player for " \
                       "GNU+Linux that gives you the tools to automate the " \
                       "playback of your music collection."
#define WF_COPYRIGHT "Copyright (C) 2021, 2022  Quico Augustijn"
#define WF_LICENSE "License GPLv3+: GNU GPL version 3 or later"
#define WF_LICENSE_MESSAGE "This program comes with ABSOLUTELY NO WARRANTY, " \
                           "to the extent permitted by law.\n" \
                           "This is free software; you are free to change " \
                           "and redistribute it under certain conditions."
#define WF_HELP_MESSAGE WF_NAME "; " WF_DESCRIPTION

#endif /* __WF_CONSTANTS__ */

/* END OF FILE */
