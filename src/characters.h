/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * characters.h  This file is part of LibWoofer
 * Copyright (C) 2022  Quico Augustijn
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

#ifndef __WF_CHARACTERS__
#define __WF_CHARACTERS__

/* INCLUDES BEGIN */

#include <glib.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

guint32 wf_chars_get_hash(const gchar *str);
guint32 wf_chars_get_hash_converted(const gchar *str);

/* FUNCTION PROTOTYPES END */

#endif /* __WF_CHARACTERS__ */

/* END OF FILE */
