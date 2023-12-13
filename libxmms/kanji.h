/*
 * kanji.h -- kanji code conversion library header
 * (C)Copyright 1998, 99, 2000, 2001 by Hiroshi Takekawa
 * Last Modified: Tue Jan 16 02:23:32 2001.
 */

typedef unsigned char byte;
typedef unsigned short int word;

#define NOTDETERMINED  0
#define ASCII          1
#define JISROMAN       2
#define EUC            4
#define JIS            8
#define SJIS          16
#define X0201         32

void  kanji_initialize(void);
int   kanji_checkcode(byte *str, int *f);
int   kanji_checkcode_string(byte *str, int *f);
int   kanji_han2zen(byte *str, word *c);
int   kanji_zen2han(byte *str, word *c);
int   kanji_jis2euc(byte *str, word *c);
int   kanji_jis2sjis(byte *str, word *c);
int   kanji_euc2sjis(byte *str, word *c);
int   kanji_euc2jis(byte *str, word *c);
int   kanji_sjis2euc(byte *str, word *c);
int   kanji_sjis2jis(byte *str, word *c);
byte *kanji_jis2euc_string(byte *str);
byte *kanji_jis2sjis_string(byte *str);
byte *kanji_euc2jis_string(byte *str);
byte *kanji_euc2sjis_string(byte *str);
byte *kanji_sjis2euc_string(byte *str);
byte *kanji_sjis2jis_string(byte *str);
