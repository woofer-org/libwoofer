/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * characters.c  This file is part of LibWoofer
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

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>

// Global includes
/*< none >*/

// Module includes
/*< none >*/

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This module handles character conversion and string hashing.  Character
 * conversion primarily involves replacing some Latin letters with diacritics
 * to their ASCII variant.  This makes matching or hashing strings with
 * different uses of diacritics more reliable.
 *
 * Since this module only contains utilities for other modules, all of these
 * "utilities" are part of the normal module functions and constructors,
 * destructors, etc are left out.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static gboolean wf_chars_is_start_of_multibyte(guchar ch);

static guchar wf_chars_special_to_normal(gunichar ch);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* MODULE FUNCTIONS BEGIN */

static gboolean
wf_chars_is_start_of_multibyte(guchar ch)
{
	/*
	 * Check for the bits that indicate the start of a multibyte character
	 * sequence.  Note that this will also return %FALSE for ASCII values.
	 */

	return (0x80 != (0xC0 & ch));
}

static guchar
wf_chars_special_to_normal(gunichar ch)
{
	/*
	 * These are statically allocated char arrays containing hexadecimal values
	 * (essentially Unicode) that match the character maps of UTF-8 characters.
	 * These can then be converted to their normal alpha variant.  Do keep in mind
	 * that high-level UTF-8 characters consist of two or more individual
	 * characters (in the high range of > 127) that when printed directly after
	 * each other, show up as the correct special character.  Because of this, a
	 * bit more work has to be done in order to find the right characters.  This
	 * involves checking for a high-level character and using the ones that follow
	 * to determine what character it is.  The reason the combined hexadecimal
	 * values are defined here is because defining the split UTF-8 characters and
	 * matching them would make things even more complicated.
	 */
	static const gunichar special_a[] = { 'a', 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x0100, 0x0101, 0x0102, 0x0103, 0x0104, 0x0105, 0 };
	static const gunichar special_c[] = { 'c', 0x00C7, 0x00E7, 0x0106, 0x0107, 0x0108, 0x0109, 0x010A, 0x010B, 0x010C, 0x010D, 0 };
	static const gunichar special_d[] = { 'd', 0x010E, 0x010F, 0x0110, 0x0111, 0 };
	static const gunichar special_e[] = { 'e', 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0112, 0x0113, 0x0114, 0x0115, 0x0116, 0x0117, 0x0118, 0x0119, 0x011A, 0x011B, 0 };
	static const gunichar special_g[] = { 'g', 0x011C, 0x011D, 0x011E, 0x011F, 0x0120, 0x0121, 0x0122, 0x0123, 0 };
	static const gunichar special_h[] = { 'h', 0x0124, 0x0125, 0x0126, 0x0127, 0 };
	static const gunichar special_i[] = { 'i', 0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x0128, 0x0129, 0x012A, 0x012B, 0x012C, 0x012D, 0x012E, 0x012F, 0x0130, 0x0131, 0 };
	static const gunichar special_j[] = { 'j', 0x0134, 0x0135, 0 };
	static const gunichar special_k[] = { 'k', 0x0136, 0x0137, 0 };
	static const gunichar special_l[] = { 'l', 0x0139, 0x013A, 0x013B, 0x013C, 0x013D, 0x013E, 0x013F, 0x0140, 0x0141, 0x0142, 0 };
	static const gunichar special_n[] = { 'n', 0x00D1, 0x00F1, 0x0143, 0x0144, 0x0145, 0x0146, 0x0147, 0x0148, 0x0149, 0 };
	static const gunichar special_o[] = { 'o', 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D8, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F8, 0x014C, 0x014D, 0x014E, 0x014F, 0x0150, 0x0151, 0 };
	static const gunichar special_r[] = { 'r', 0x0154, 0x0155, 0x0156, 0x0157, 0x0158, 0x0159, 0 };
	static const gunichar special_s[] = { 's', 0x015A, 0x015B, 0x015C, 0x015D, 0x015E, 0x015F, 0x0160, 0x0161, 0 };
	static const gunichar special_t[] = { 't', 0x0162, 0x0163, 0x0164, 0x0165, 0x0166, 0x0167, 0 };
	static const gunichar special_u[] = { 'u', 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0168, 0x0169, 0x016A, 0x016B, 0x016C, 0x016D, 0x016E, 0x016F, 0x0170, 0x0171, 0x0172, 0x0173, 0 };
	static const gunichar special_w[] = { 'w', 0x0174, 0x0175, 0 };
	static const gunichar special_y[] = { 'y', 0x00DD, 0x00FD, 0x00FF, 0x0176, 0x0177, 0x0178, 0 };
	static const gunichar special_z[] = { 'z', 0x0179, 0x017A, 0x017B, 0x017C, 0x017D, 0x017E, 0 };

	static const gunichar *special_char_array[] = { special_a, special_c, special_d, special_e, special_g, special_h, special_i, special_j, special_k, special_l, special_n, special_o, special_r, special_s, special_t, special_u, special_w, special_y, special_z, NULL };

	const gunichar **arrays, *chars;
	gunichar item;

	// Loop through all arrays
	for (arrays = special_char_array; *arrays != NULL; arrays++)
	{
		chars = *arrays; // Current array to search through
		item = *chars; // On match, use this character as replacement

		// Start at array position 1 (past the first and single ASCII char)
		chars++;

		// Loop through all special characters
		while (*chars != 0)
		{
			// Matched?
			if (ch == *chars)
			{
				// Use this character as a replacement
				return (gchar) item;
			}

			chars++;
		}
	}

	// If no match, just keep this character
	return ch;
}

guint32
wf_chars_get_hash(const gchar *str)
{
	const gchar *p;
	gchar ch;
	guchar uchar;
	guint x;
	guint32 hash = 0;

	if (str == NULL)
	{
		return 0;
	}

	for (p = str; *p != '\0'; p++)
	{
		ch = *p; // Dereference; take character
		uchar = (guchar) ch; // Force use unsigned char
		x = (guint) uchar; // Now convert to integer

		hash = (hash << 5) + hash + x; // Do the magic
	}

	return hash;
}

// Get hash with some special characters converted
guint32
wf_chars_get_hash_converted(const gchar *str)
{
	const gchar *p;
	gchar ch;
	guchar uchar;
	gunichar unichar;
	guint x;
	guint pos = 0;
	guint32 hash = 0;

	if (str == NULL)
	{
		return 0;
	}

	for (p = str; *p != '\0'; p = g_utf8_next_char(p))
	{
		pos++; // Set position

		ch = *p; // Dereference; take character
		uchar = (guchar) ch; // Force use unsigned char

		if (uchar >= 128 && wf_chars_is_start_of_multibyte(uchar))
		{
			// First byte in a multibyte sequence, so get the Unicode char
			unichar = g_utf8_get_char(p);

			if (unichar > 0)
			{
				/*
				 * Attempt to convert this special character to ASCII.  Note
				 * that if this fails, it will return the original char.
				 */
				uchar = wf_chars_special_to_normal(unichar);
			}
		}
		else
		{
			// If ASCII, always use lowercase
			uchar = g_ascii_tolower(uchar);
		}

		x = (guint) uchar; // Now convert to integer

		hash = (hash << 5) + hash + x; // Do the magic
	}

	return hash;
}

/* MODULE FUNCTIONS END */

/* END OF FILE */
