/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * remote.h  This file is part of LibWoofer
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

#ifndef __WF_REMOTE__
#define __WF_REMOTE__

/* INCLUDES BEGIN */

#include <glib.h>

/* INCLUDES END */

G_BEGIN_DECLS

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */

void wf_remote_init(GDBusConnection *connection);
void wf_remote_setup(GDBusConnection *connection);

/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */
/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */
/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */

void wf_remote_finalize(void);

/* DESTRUCTOR PROTOTYPES END */

G_END_DECLS

#endif /* __WF_REMOTE__ */

/* END OF FILE */
