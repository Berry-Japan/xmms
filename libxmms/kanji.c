/*
 * kanji.c -- kanji code convertion routine
 * (C)Copyright 1998, 99, 2000, 2001 by Hiroshi Takekawa <sian@big.or.jp>
 * Last Modified: Sat Mar 31 14:53:56 2001.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kanji.h"

/* 半角 -> 全角 */
word han2zen[] = {
  0x0000, 0x2123, 0x2156, 0x2157, /* 　。「」 */
  0x2122, 0x2126, 0x2572, 0x2521, /* 、・ヲァ */
  0x2523, 0x2525, 0x2527, 0x2529, /* ィゥェォ */
  0x2563, 0x2565, 0x2567, 0x2543, /* ャュョッ */
  0x213c, 0x2522, 0x2524, 0x2526, /* ーアイウ */
  0x2528, 0x252a, 0x252b, 0x252d, /* エオカキ */
  0x252f, 0x2531, 0x2533, 0x2535, /* クケコサ */
  0x2537, 0x2539, 0x253b, 0x253d, /* シスセソ */
  0x253f, 0x2541, 0x2544, 0x2546, /* タチツテ */
  0x2548, 0x254a, 0x254b, 0x254c, /* トナニヌ */
  0x254d, 0x254e, 0x254f, 0x2552, /* ネノハヒ */
  0x2555, 0x2558, 0x255b, 0x255e, /* フヘホマ */
  0x255f, 0x2560, 0x2561, 0x2562, /* ミムメモ */
  0x2564, 0x2566, 0x2568, 0x2569, /* ヤユヨラ */
  0x256a, 0x256b, 0x256c, 0x256d, /* リルレロ */
  0x256f, 0x2573, 0x212b, 0x212c, /* ワン゛゜ */
};

/* 半角 + 濁点 -> 全角 */
word han2zend[] = {
  0x0000, 0x0000, 0x0000, 0x0000, /* 　。「」 */
  0x0000, 0x0000, 0x0000, 0x0000, /* 、・ヲァ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ィゥェォ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ャュョッ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ーアイウ */
  0x0000, 0x0000, 0x252c, 0x252e, /* エオカキ */
  0x2530, 0x2532, 0x2534, 0x2536, /* クケコサ */
  0x2538, 0x253a, 0x253c, 0x353e, /* シスセソ */
  0x2540, 0x2542, 0x2545, 0x2547, /* タチツテ */
  0x2549, 0x0000, 0x0000, 0x0000, /* トナニヌ */
  0x0000, 0x0000, 0x2550, 0x2553, /* ネノハヒ */
  0x2556, 0x2559, 0x255c, 0x0000, /* フヘホマ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ミムメモ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ヤユヨラ */
  0x0000, 0x0000, 0x0000, 0x0000, /* リルレロ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ワン゛゜ */
};

/* 半角 + 半濁点 -> 全角 */
word han2zenh[] = {
  0x0000, 0x0000, 0x0000, 0x0000, /* 　。「」 */
  0x0000, 0x0000, 0x0000, 0x0000, /* 、・ヲァ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ィゥェォ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ャュョッ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ーアイウ */
  0x0000, 0x0000, 0x0000, 0x0000, /* クケコサ */
  0x2538, 0x253a, 0x253c, 0x353e, /* シスセソ */
  0x2540, 0x2542, 0x2545, 0x2547, /* タチツテ */
  0x2549, 0x0000, 0x0000, 0x0000, /* トナニヌ */
  0x0000, 0x0000, 0x2550, 0x2553, /* ネノハヒ */
  0x2556, 0x2559, 0x255c, 0x0000, /* フヘホマ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ミムメモ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ヤユヨラ */
  0x0000, 0x0000, 0x0000, 0x0000, /* リルレロ */
  0x0000, 0x0000, 0x0000, 0x0000, /* ワン゛゜ */
};

/* preserve charset specified by JIS escape code */
static int Charset = NOTDETERMINED;
static int Charflag = NOTDETERMINED;

/* clear charset before processing new text */
void kanji_initialize(void)
{
  Charset = NOTDETERMINED;
  Charflag = -1;
}

/*
 * kanji_checkcode
 * function: make an effort to determine kanji code
 * str: pointer to string
 * f: pointer to code flag
 *          0: cannot determine, maybe treated as ascii
 *          1: ascii
 *          2: euc
 *          4: jis
 *          8: sjis
 *         16: JISX0201 hankaku kana
 * return: read num of bytes
 */
int kanji_checkcode(byte *str, int *f)
{
  byte first = *str, second = *(str + 1);

  *f = 0;
  if (first >= 0xa0 && first <= 0xdf)
    *f |= X0201;

  if (((first >= 0x81 && first <= 0x9f) || (first >= 0xe0 && first <= 0xfc)) &&
      (second >= 0x40 && second <= 0xfc && second != 0x7f)) {
    *f |= SJIS;
    return 2;
  }

  if (first >= 0xa1 && first <= 0xfe && second >= 0xa1 && second <= 0xfe &&
      !(Charset & X0201)) {
    *f &= ~X0201;
    *f |= EUC;
    return 2;
  }

  if (first == 0x8e && second >= 0xa0 && second <= 0xdf) {
    *f |= (EUC | X0201);
    return 2;
  }

  if (first < 0x80) {
    *f = 1;
    return 1;
  }

  return 1;
}

/*
 * kanji_checkcode_string
 *
 * function: make an effort to determine kanji code
 * str: pointer to string
 * f: pointer to code flag
 *          0: cannot determine, maybe treated as ascii
 *          1: ascii
 *          2: euc
 *          4: jis
 *          8: sjis
 *         16: JISX0201 hankaku kana
 * return: read num of bytes
 */
int kanji_checkcode_string(byte *str, int *f_return)
{
  int f;
  int c, l = 0;

  do {
    if (*str == '\0')
      break;
    c = kanji_checkcode(str, &f);
    l += c;
    str += c;
  } while (f == ASCII);

  *f_return = f;

  return l;
}

/*
 * kanji_han2zen
 * function: convert JISX0201 hankaku kana to zenkaku
 * str: pointer to string
 * c: pointer to converted char
 * return: read byte size
 */
int kanji_han2zen(byte *str, word *c)
{
  byte f = *str++;
  byte s = *str++;

  /* 半角カナか？ */
  if (f < 0xa0 || f > 0xdf) {
    *c = (word)f;
    return 1;
  }

  /* 半角カナらしい */
  f -= 0xa0;
  if (s == 0xde) {
    /* 濁点がついている */
    if (han2zend[f]) {
      /* 濁点つきを一字であらわせる */
      *c = han2zend[f];
      return 2;
    }
    /* ナ゛ニ〜 とか書かれてるかもしれない… */
    *c = han2zen[f];
    return 1;
  } else if (s == 0xdf) {
    /* 半濁点がついている */
    if (han2zenh[f]) {
      *c = han2zenh[f];
      return 2;
    }
    /* 濁点に比べると格段に需要がない気もする */
    *c = han2zen[f];
    return 1;
  }

  *c = han2zen[f];
  return 1;
}

/*
 * kanji_zen2han
 * function: convert JISX0208 Katakana to JISX0201 Katakana
 * str: pointer to string
 * c: pointer to converted char
 * return: read byte size
 */

/* very ugly... but works... */
int kanji_zen2han(byte *str, word *c)
{
  int i;
  word w;

  w = (*str << 8) + *(str + 1);
  w &= ~0x8080; /* EUCでもOK */

  /* 清音の中にあるか探す */
  for (i = 0; i < 64; i++)
    if (w == han2zen[i]) {
      *c = i + 0x20;
      return 2;
    }

  /* 濁音の中にあるか探す */
  for (i = 0; i < 64; i++)
    if (w == han2zend[i]) {
      *c = ((i + 0x20) << 8) + 0x5e;
      return 2;
    }

  /* 半濁音の中にあるか探す */
  for (i = 0; i < 64; i++)
    if (w == han2zenh[i]) {
      *c = ((i + 0x20) << 8) + 0x5f;
      return 2;
    }

  return 0;
}

/*
 * kanji_jis2euc
 * function: convert from jis to euc
 * str: pointer to string
 * c: pointer to converted char
 * return: read byte size
 */
int kanji_jis2euc(byte *str, word *c)
{
  int first, second, a = 0, f = Charset;

  first  = *str;
  second = *(str + 1);

  if (first == 0x1b) {
    byte third  = *(str + 2);
    byte fourth = *(str + 3);

    /* JIS escape code */
    if (second == '$' && third == 'B')
      /* JIS X0208-1983 */
      f = JIS;
    else if (second == '$' && third == '@')
      /* JIS X0208-1978 */
      f = JIS;
    else if (second == '(' && third == 'J')
      /* JIS X0201ローマ字 */
      f = JISROMAN;
    else if (second == '(' && third == 'I')
      /* JIS X0201片仮名 */
      f = JIS | X0201;
    else if (second == '(' && third == 'B')
      f = ASCII;
    else/* 他のコードセットはサポートしていない… */
      f = NOTDETERMINED;
    Charset = f;
    if (f == JIS && (fourth >= 0xa0 && fourth <= 0xdf))
      f |= X0201;
    str += 3;
    a = 3;
  }

  if (!(f & JIS)) {
    *c = *str;
    return a + 1;
  }

  if (f & X0201) {
    *c = 0x8e80 | *str;
    return a + 1;
  }

  *c = ((*str << 8) | *(str + 1)) | 0x8080;
  return a + 2;
}

/*
 * kanji_jis2sjis
 * function: convert from jis to sjis
 * str: pointer to string
 * c: pointer to converted char
 * return: read byte size
 */
int kanji_jis2sjis(byte *str, word *c)
{
  int a = 0, f = Charset;
  byte first, second, h, l;

  h = first  = *str;
  l = second = *(str + 1);

  if (first == 0x1b) {
    byte third  = *(str + 2);
    byte fourth = *(str + 3);

    /* JIS escape code */
    if (second == '$' && third == 'B')
      /* JIS X0208-1983 */
      f = JIS;
    else if (second == '$' && third == '@')
      /* JIS X0208-1978 */
      f = JIS;
    else if (second == '(' && third == 'J')
      /* JIS X0201ローマ字 */
      f = JISROMAN;
    else if (second == '(' && third == 'I')
      /* JIS X0201片仮名 */
      f = JIS | X0201;
    else if (second == '(' && third == 'B')
      f = ASCII;
    else/* 他のコードセットはサポートしていない… */
      f = NOTDETERMINED;
    Charset = f;
    if (f == JIS && (fourth >= 0xa0 && fourth <= 0xdf))
      f |= X0201;
    str += 3;
    a = 3;
  }

  if (!(f & JIS) || (!(f * X0201))) {
    *c = *str;
    return a + 1;
  }
    
  l += (h & 1) ? 0x1f : 0x7d;
  if (l >= 0x7f)
    l++;
  h = ((h - 0x21) >> 1) + 0x81;
  if (h > 0x9f)
    h += 0x40;

  *c = (h << 8) | l;
  return a + 2;
}

/*
 * kanji_euc2jis
 * function: convert from euc to jis
 * str: pointer to string
 * c: pointer to converted char
 * return: read byte size
 */
int kanji_euc2jis(byte *str, word *c)
{
  int f = 0;
  byte first, second;

  first = *str;
  second = *(str + 1);

  if (first >= 0xa1 && first <= 0xfe && second >= 0xa1 && second <= 0xfe)
    f = EUC;

  if (first == 0x8e && second >= 0xa0 && second <= 0xdf)
    f = EUC | X0201;

  Charflag = f;

  if (!(f & EUC)) {
    *c = *str;
    return 1;
  }

  if (f & X0201) {
    str++;
    /* return 1 + kanji_han2zen(str, c); */
    *c = *str & 0x7f;
  } else
    *c = ((*str << 8) | *(str + 1)) & 0x7f7f;
  return 2;
}

/*
 * kanji_euc2sjis
 * function: convert from euc to sjis
 * str: pointer to string
 * c: pointer to convert char
 * return: read byte size
 */
int kanji_euc2sjis(byte *str, word *c)
{
  int f = 0;
  word j;
  byte h, l, first, second;

  first = *str;
  second = *(str + 1);

  if (first >= 0xa1 && first <= 0xfe && second >= 0xa1 && second <= 0xfe)
    f = EUC;

  if (first == 0x8e && second >= 0xa0 && second <= 0xdf)
    f = EUC | X0201;

  if (f & X0201) {
    *c = *(str + 1);
    return 2;
  }

  if (!(f & EUC)) {
    *c = *str;
    return 1;
  }

  j = ((*str << 8) | *(str + 1)) & 0x7f7f;

  h = j >> 8;
  l = j & 0xff;

  l += (h & 1) ? 0x1f : 0x7d;
  if (l >= 0x7f)
    l++;
  h = ((h - 0x21) >> 1) + 0x81;
  if (h > 0x9f)
    h += 0x40;

  *c = (h << 8) | l;
  return 2;
}

/*
 * kanji_sjis2euc
 * function: convert from sjis to euc
 * str: pointer to string
 * c: pointer to convert char
 * return: read byte size
 */
int kanji_sjis2euc(byte *str, word *c)
{
  int f = 0;
  byte h, l, first, second;

  first = *str;
  second = *(str + 1);

  if (first >= 0xa0 && first <= 0xdf)
    f = X0201;

  if (((first >= 0x81 && first <= 0x9f) || (first >= 0xe0 && first <= 0xfc)) &&
      (second >= 0x40 && second <= 0xfc && second != 0x7f))
    f |= SJIS;

  if (f & X0201) {
    *c = 0x8e00 | *str;
    return 1;
  }

  if (!(f & SJIS)) {
    *c = *str;
    return 1;
  }

  h = *str;
  l = *(str + 1);

  h -= (h <= 0x9f) ? 0x71 : 0xb1;
  h = (h << 1) + 1;
  if (l >= 0x9f) {
    l -= 0x7e;
    h++;
  } else if (l > 0x7f)
    l -= 0x20;
  else
    l -= 0x1f;

  h |= 0x80;
  l |= 0x80;

  *c = (h << 8) | l;
  return 2;
}

/*
 * kanji_sjis2jis
 * function: convert from sjis to jis
 * str: pointer to string
 * c: pointer to convert char
 * return: read byte size
 */
int kanji_sjis2jis(byte *str, word *c)
{
  int f = 0;
  byte h, l, first, second;

  first = *str;
  second = *(str + 1);

  if (first >= 0xa0 && first <= 0xdf)
    f = X0201;

  if (((first >= 0x81 && first <= 0x9f) || (first >= 0xe0 && first <= 0xfc)) &&
      (second >= 0x40 && second <= 0xfc && second != 0x7f))
    f |= SJIS;

  if (f & X0201) {
    *c = *str;
    return 1;
  }

  if (!(f & SJIS)) {
    *c = *str;
    return 1;
  }

  h = *str;
  l = *(str + 1);

  h -= (h <= 0x9f) ? 0x71 : 0xb1;
  h = (h << 1) + 1;
  if (l >= 0x9e) {
    l -= 0x7e;
    h++;
  } else if (l > 0x7f)
    l -= 0x20;
  else
    l -= 0x1f;

  *c = (h << 8) | l;
  return 2;
}

/*
 * kanji_jis2euc_string
 * function: convert string from jis to euc
 * input: input jis string
 * return: pointer to output euc string
 */
byte *kanji_jis2euc_string(byte *input)
{
  byte *ptr, *output;
  word c;

  if ((ptr = output = malloc(strlen(input) * 2)) == NULL)
    return NULL;

  while (*input != '\0') {
    input += kanji_jis2euc(input, &c);
    if ((c >> 8) == 0)
      *ptr++ = (byte)c;
    else {
      *ptr++ = (byte)(c >> 8);
      *ptr++ = (byte)c & 0xff;
    }
  }
  *ptr++ = '\0';

  return realloc(output, ptr - output);
}

/*
 * kanji_jis2sjis_string
 * function: convert string from jis to sjis
 * input: input jis string
 * return: pointer to output sjis string
 */
byte *kanji_jis2sjis_string(byte *input)
{
  byte *ptr, *output;
  word c;

  if ((ptr = output = malloc(strlen(input) * 2)) == NULL)
    return NULL;

  while (*input != '\0') {
    input += kanji_jis2sjis(input, &c);
    if ((c >> 8) == 0)
      *ptr++ = (byte)c;
    else {
      *ptr++ = (byte)(c >> 8);
      *ptr++ = (byte)c & 0xff;
    }
  }
  *ptr++ = '\0';

  return realloc(output, ptr - output);
}

/*
 * kanji_euc2jis_string
 * function: convert string from euc to jis
 * input: input euc string
 * return: pointer to output jis string
 */
byte *kanji_euc2jis_string(byte *input)
{
  byte *ptr, *output;
  int fold;
  word c;

  if ((ptr = output = malloc(strlen(input) * 2)) == NULL)
    return NULL;

  while (*input != '\0') {
    fold = Charflag;
    input += kanji_euc2jis(input, &c);
    Charflag &= ~SJIS;
    if (fold != Charflag) {
      *ptr++ = 0x1b;
      if (Charflag == (EUC | X0201)) {
	*ptr++ = '(';
	*ptr++ = 'I';
      } else if (Charflag == EUC) {
	*ptr++ = '$';
	*ptr++ = 'B';
      } else {
	*ptr++ = '(';
	*ptr++ = 'B';
      }
    }

    if ((c >> 8) == 0)
      *ptr++ = (byte)c;
    else {
      *ptr++ = (byte)(c >> 8);
      *ptr++ = (byte)c & 0xff;
    }
  }
  *ptr++ = '\0';

  return realloc(output, ptr - output);
}

/*
 * kanji_euc2sjis_string
 * function: convert string from euc to sjis
 * input: input euc string
 * return: pointer to output sjis string
 */
byte *kanji_euc2sjis_string(byte *input)
{
  byte *ptr, *output;
  word c;

  if ((ptr = output = malloc(strlen(input) * 2)) == NULL)
    return NULL;

  while (*input != '\0') {
    input += kanji_euc2sjis(input, &c);
    if ((c >> 8) == 0)
      *ptr++ = (byte)c;
    else {
      *ptr++ = (byte)(c >> 8);
      *ptr++ = (byte)c & 0xff;
    }
  }
  *ptr++ = '\0';

  return realloc(output, ptr - output);
}

/*
 * kanji_sjis2euc_string
 * function: convert string from sjis to euc
 * input: input sjis string
 * return: pointer to output euc string
 */
byte *kanji_sjis2euc_string(byte *input)
{
  byte *ptr, *output;
  word c;

  if ((ptr = output = malloc(strlen(input) * 2)) == NULL)
    return NULL;

  while (*input != '\0') {
    input += kanji_sjis2euc(input, &c);
    if ((c >> 8) == 0)
      *ptr++ = (byte)c;
    else {
      *ptr++ = (byte)(c >> 8);
      *ptr++ = (byte)c & 0xff;
    }
  }
  *ptr++ = '\0';

  return realloc(output, ptr - output);
}

/*
 * kanji_euc2sjis_string
 * function: convert string from sjis to jis
 * input: input sjis string
 * return: pointer to output jis string
 */
byte *kanji_sjis2jis_string(byte *input)
{
  byte *ptr, *output;
  int fold;
  word c;

  if ((ptr = output = malloc(strlen(input) * 2)) == NULL)
    return NULL;

  while (*input != '\0') {
    fold = Charflag;
    input += kanji_sjis2jis(input, &c);
    Charflag &= ~EUC;
    if (fold != Charflag) {
      *ptr++ = 0x1b;
      if (Charflag & X0201) {
	*ptr++ = '(';
	*ptr++ = 'I';
      } else if (Charflag & SJIS) {
	*ptr++ = '$';
	*ptr++ = 'B';
      } else {
	*ptr++ = '(';
	*ptr++ = 'B';
      }
    }

    if ((c >> 8) == 0)
      *ptr++ = (byte)c;
    else {
      *ptr++ = (byte)(c >> 8);
      *ptr++ = (byte)c & 0xff;
    }
  }
  *ptr++ = '\0';

  return realloc(output, ptr - output);
}
