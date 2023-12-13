/*
 * Copyright (C) 2001-2005 Hiroshi Takekawa <sian@big.or.jp>
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "converter.h"
#include "../xmms/i18n.h"

#ifdef HAVE_ICONV
# define ICONV_OUTPUT_SIZE 65536
# include <iconv.h>
# ifdef HAVE_LANGINFO_CODESET
#  include <langinfo.h>
# endif
#else
# ifdef KANJI
#  include "kanji.h"
# endif
#endif

#ifdef HAVE_ICONV

/* From easytag-0.30/src/charset.[ch] with some modifications. */

typedef struct __charset_info{
	const char *title;
	const char *name;
} CharsetInfo;

#define CHARSET_TRANS_ARRAY_LEN (sizeof(charset_trans_array) / sizeof((charset_trans_array)[0]))
static const CharsetInfo charset_trans_array[] = { 
	{N_("Guess from locale"),                "guess"       },
	{N_("No conversion"),                    "noconv"      },
	{N_("User Input"),                       "user"        },
	{N_("Arabic (IBM-864)"),                 "IBM864"      },
	{N_("Arabic (ISO-8859-6)"),              "ISO-8859-6"  },
	{N_("Arabic (Windows-1256)"),            "windows-1256"},
	{N_("Baltic (ISO-8859-13)"),             "ISO-8859-13" },
	{N_("Baltic (ISO-8859-4)"),              "ISO-8859-4"  },
	{N_("Baltic (Windows-1257)"),            "windows-1257"},
	{N_("Celtic (ISO-8859-14)"),             "ISO-8859-14" },
	{N_("Central European (IBM-852)"),       "IBM852"      },
	{N_("Central European (ISO-8859-2)"),    "ISO-8859-2"  },
	{N_("Central European (Windows-1250)"),  "windows-1250"},
	{N_("Chinese Simplified (GB18030)"),     "GB18030"     },
	{N_("Chinese Simplified (GB2312)"),      "GB2312"      },
	{N_("Chinese Simplified (GBK)"),         "GBK"         },
	{N_("Chinese Traditional (Big5)"),       "Big5"        },
	{N_("Chinese Traditional (Big5-HKSCS)"), "Big5-HKSCS"  },
	{N_("Chinese Traditional (EUC-TW)"),     "EUC-TW"      },
	{N_("Cyrillic (IBM-855)"),               "IBM855"      },
	{N_("Cyrillic (ISO-8859-5)"),            "ISO-8859-5"  },
	{N_("Cyrillic (ISO-IR-111)"),            "ISO-IR-111"  },
	{N_("Cyrillic (KOI8-R)"),                "KOI8-R"      },
	{N_("Cyrillic (Windows-1251)"),          "windows-1251"},
	{N_("Cyrillic/Russian (CP-866)"),        "IBM866"      },
	{N_("Cyrillic/Ukrainian (KOI8-U)"),      "KOI8-U"      },
	{N_("English (US-ASCII)"),               "US-ASCII"    },
	{N_("Greek (ISO-8859-7)"),               "ISO-8859-7"  },
	{N_("Greek (Windows-1253)"),             "windows-1253"},
	{N_("Hebrew (IBM-862)"),                 "IBM862"      },
	{N_("Hebrew (Windows-1255)"),            "windows-1255"},
	{N_("Japanese (EUC-JP)"),                "EUC-JP"      },
	{N_("Japanese (ISO-2022-JP)"),           "ISO-2022-JP" },
	{N_("Japanese (Shift_JIS)"),             "Shift_JIS"   },
	{N_("Korean (EUC-KR)"),                  "EUC-KR"      },
	{N_("Korean (JOHAB)"),                   "JOHAB"       },
	{N_("Korean (UHC)"),                     "UHC"         },
	{N_("Nordic (ISO-8859-10)"),             "ISO-8859-10" },
	{N_("South European (ISO-8859-3)"),      "ISO-8859-3"  },
	{N_("Thai (TIS-620)"),                   "TIS-620"     },
	{N_("Turkish (IBM-857)"),                "IBM857"      },
	{N_("Turkish (ISO-8859-9)"),             "ISO-8859-9"  },
	{N_("Turkish (Windows-1254)"),           "windows-1254"},
	{N_("Unicode (UTF-7)"),                  "UTF-7"       },
	{N_("Unicode (UTF-8)"),                  "UTF-8"       },
	{N_("Unicode (UTF-16BE)"),               "UTF-16BE"    },
	{N_("Unicode (UTF-16LE)"),               "UTF-16LE"    },
	{N_("Unicode (UTF-32BE)"),               "UTF-32BE"    },
	{N_("Unicode (UTF-32LE)"),               "UTF-32LE"    },
	{N_("Vietnamese (VISCII)"),              "VISCII"      },
	{N_("Vietnamese (Windows-1258)"),        "windows-1258"},
	{N_("Vietnamese (TCVN)"),                "TCVN"        },
	{N_("Visual Hebrew (ISO-8859-8)"),       "ISO-8859-8"  },
	{N_("Western (IBM-850)"),                "IBM850"      },
	{N_("Western (ISO-8859-1)"),             "ISO-8859-1"  },
	{N_("Western (ISO-8859-15)"),            "ISO-8859-15" },
	{N_("Western (Windows-1252)"),           "windows-1252"},
};

static char *get_current_charset(void)
{
	char *charset = getenv("CHARSET");

#ifdef HAVE_LANGINFO_CODESET
	if (!charset)
		charset = nl_langinfo(CODESET);
#endif
	if (!charset)
		charset = "ISO-8859-1";

	return charset;
}

GList *xmms_converter_charset_list(void)
{
	GList *list = NULL;
	guint i;
	for (i = 0; i < CHARSET_TRANS_ARRAY_LEN; i++)
		list = g_list_append(list, _(charset_trans_array[i].title));
	return list;
}

char *xmms_converter_get_name_by_title(char *title)
{
	int i;

	if (title)
		for (i = 0; i < CHARSET_TRANS_ARRAY_LEN; i++)
			if (strcasecmp(_(title), _(charset_trans_array[i].title)) == 0)
				return charset_trans_array[i].name;
	return "";
}

char *xmms_converter_get_title_by_name(char *name)
{
	int i;

	if (name)
		for (i = 0; i < CHARSET_TRANS_ARRAY_LEN; i++)
			if (strcasecmp(name, charset_trans_array[i].name) == 0)
				return _(charset_trans_array[i].title);
	return _("User Input");
}

int xmms_converter_convert(gchar *s, gchar **d_r, char *from, char *to, int *err)
{
  iconv_t cd;
  gchar *d, *inptr, *outptr;
  size_t avail, insize, nconv;

  *err = 0;
  if (!s) {
    *d_r = NULL;
    return 0;
  }
  if ((insize = strlen(s)) == 0) {
    *d_r = g_strdup("");
    return 0;
  }
  if (strcasecmp(from, "noconv") == 0 || strcasecmp(to, "noconv") == 0) {
    *d_r = g_strdup(s);
    return strlen(s);
  }
  if (strcasecmp(from, "user") == 0 || strcasecmp(to, "user") == 0) {
    fprintf(stderr, "BUG: from %s, to %s: 'user' should be replaced with the user input charset.", from, to);
    *d_r = g_strdup(s);
    return strlen(s);
  }
  if (strcasecmp(from, "guess") == 0) {
    from = get_current_charset();
  }
  if (strcasecmp(to, "guess") == 0) {
    to = get_current_charset();
  }

  if ((d = malloc(ICONV_OUTPUT_SIZE)) == NULL) {
    fprintf(stderr, "No enough memory.\n");
    if ((*d_r = g_strdup(s)) == NULL)
      return 0;
    return strlen(s);
  }

  if ((cd = iconv_open(to, from)) == (iconv_t)-1) {
    free(d);
    *err = errno;
    if (errno == EINVAL) {
      *d_r = g_strdup(s);
      return strlen(s);
    }
    perror("xmms_converter_convert");
    return errno;
  }

  inptr = s;
  outptr = d;
  avail = ICONV_OUTPUT_SIZE - 1;

  if ((nconv = iconv(cd, (char **)&inptr, &insize, &outptr, &avail)) != (size_t)-1) {
    /* For state-dependent character sets we have to flush the state now. */
    iconv(cd, NULL, NULL, &outptr, &avail);
    *outptr = '\0';
    *d_r = g_strdup(d);
  } else {
    *err = errno;
    switch (errno) {
    case E2BIG:
      fprintf(stderr, _("Increase ICONV_OUTPUT_SIZE and recompile.\n"));
      break;
    case EILSEQ:
      fprintf(stderr, _("Invalid sequence passed.\n"));
      break;
    case EINVAL:
      fprintf(stderr, _("Incomplete multi-byte sequence passed.\n"));
      break;
    default:
      perror("xmms_converter_convert");
      break;
    }
    *d_r = g_strdup(s);
  }

  iconv_close(cd);
  free(d);

  return strlen(*d_r);
}
#else
# ifdef KANJI
static int kanjicode(char *codename)
{
  if (strcasecmp(codename, "EUC-JP") == 0 || strcasecmp(codename, "EUC") == 0)
    return EUC;
  if (strcasecmp(codename, "SJIS") == 0 || strcasecmp(codename, "Shift_JIS") == 0)
    return SJIS;
  if (strcasecmp(codename, "ISO-2022-JP") == 0 || strcasecmp(codename, "JIS") == 0)
    return JIS;
  fprintf(stderr, __FUNCTION__ ": A codename %s is unknown.\n", codename);

  return NOTDETERMINED;
}

int xmms_converter_convert(gchar *s, gchar **d_r, char *fromname, char *toname)
{
  int from, to, now;

  if (!s) {
    *d_r = NULL;
    return 0;
  }
  if (strlen(s) == 0) {
    *d_r = g_strdup("");
    return 0;
  }

  from = kanjicode(fromname);
  to   = kanjicode(toname);
  kanji_initialize();
  (void)kanji_checkcode_string(s, &now);

  if (now == to || now == ASCII) {
    if ((*d_r = g_strdup(s)) == NULL)
      return 0;
    return strlen(s);
  }

  if (now != from) {
    fprintf(stderr, "now != from(%d != %d)... from = now.\n", now, from);
    from = now;
  }

  /* Usually, I don't write like this... */ 
  switch (from) {
  case EUC:
    switch (to) {
    case SJIS: *d_r = kanji_euc2sjis_string(s); break;
    case JIS:  *d_r = kanji_euc2jis_string(s); break;
    case EUC:
    default:    *d_r = g_strdup(s); break;
    }
    break;
  case SJIS:
    switch (to) {
    case EUC:  *d_r = kanji_sjis2euc_string(s); break;
    case JIS:  *d_r = kanji_sjis2jis_string(s); break;
    case SJIS:
    default:    *d_r = g_strdup(s); break;
    }
    break;
  case JIS:
    switch (to) {
    case EUC:  *d_r = kanji_jis2euc_string(s); break;
    case SJIS: *d_r = kanji_jis2sjis_string(s); break;
    case JIS:
    default:    *d_r = g_strdup(s); break;
    }
    break;
  }

  return strlen(*d_r);
}
# else
int xmms_converter_convert(gchar *s, gchar **d_r, char *from, char *to)
{
  if (!s) {
    *d_r = NULL;
    return 0;
  }

  *d_r = g_strdup(s);
  return strlen(s);
}
# endif
#endif
