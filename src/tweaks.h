/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * tweaks.h  This file is part of LibWoofer
 * Copyright (C) 2021  Quico Augustijn
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

#ifndef __WF_TWEAKS__
#define __WF_TWEAKS__

/* INCLUDES BEGIN */

#include <glib.h>
#include <gio/gio.h>

/* INCLUDES END */

/* DEFINES BEGIN */

#ifndef G_APPLICATION_CAN_OVERRIDE_APP_ID
#define G_APPLICATION_CAN_OVERRIDE_APP_ID 0
#endif

/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

GDateTime * wf_tweaks_file_info_get_modification_date_time(GFileInfo *info);

/* FUNCTION PROTOTYPES END */

#endif /* __WF_TWEAKS__ */

/* END OF FILE */
