/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * static/mediaplayer2.h  This file is part of LibWoofer
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

#ifndef __WF_STATIC_MEDIAPLAYER2__
#define __WF_STATIC_MEDIAPLAYER2__

/* INCLUDES BEGIN */

#include <gio/gio.h>

/* INCLUDES END */

/* FUNCTION PROTOTYPES BEGIN */

GDBusInterfaceInfo * mp2_org_mpris_mediaplayer2_get_interface_info(void);

GDBusInterfaceInfo * mp2_org_mpris_mediaplayer2_player_get_interface_info(void);

/* FUNCTION PROTOTYPES END */

#endif /* __WF_STATIC_MEDIAPLAYER2__ */

/* END OF FILE */
