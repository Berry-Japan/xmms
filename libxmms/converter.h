/*
 * Copyright (C) 2001,  Hiroshi Takekawa <sian@big.or.jp>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *                
 */

#ifndef XMMS_CONVERTER_H
#define XMMS_CONVERTER_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

int xmms_converter_convert(gchar *, gchar **, char *, char *, int *);
GList *xmms_converter_charset_list(void);
char *xmms_converter_get_name_by_title(char *);
char *xmms_converter_get_title_by_name(char *);

#ifdef __cplusplus
};
#endif

#endif /* !XMMS_CONVERTER_H */
