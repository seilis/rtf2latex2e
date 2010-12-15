/*
 * rtf.h - RTF document processing stuff.  Release 1.11.
 */

#ifndef _RTF_H

#define _RTF_H

/*
 * Twentieths of a point (twips) per inch (Many RTF measurements
 * are in twips per inch (tpi) units).  Assumes 72 points/inch.
 */

# define	rtfTpi		1440

/*
 * RTF buffer size (avoids BUFSIZ, which differs across systems)
 */

# define	rtfBufSiz	1024

/*
 * Tokens are associated with up to three classification numbers:
 *
 * Class number: Broadest (least detailed) breakdown.  For programs
 * 	that only care about gross token distinctions.
 * Major/minor numbers: Within their class, tokens have a major
 * 	number, and may also have a minor number to further
 * 	distinquish tokens with the same major number.
 *
 *	*** Class, major and minor token numbers are all >= 0 ***
 *
 * Tokens that can't be classified are put in the "unknown" class.
 * For such, the major and minor numbers are meaningless, although
 * rtfTextBuf may be of interest then.
 *
 * Text tokens are a single character, and the major number indicates
 * the character value (note: can be non-ascii, i.e., greater than 127).
 * There is no minor number.
 *
 * Control symbols may have a parameter value, which will be found in
 * rtfParam.  If no parameter was given, rtfParam = rtfNoParam.
 *
 * RTFGetToken() return value is the class number, but it sets all the
 * global token vars.
 *
 * rtfEOF is a fake token used by the reader; the writer never sees
 * it (except in the token reader hook, if it installs one).
 */


/*
 * Information pertaining to last token read by RTFToken.  The
 * text is exactly as it occurs in the input file, e.g., "\{"
 * will be found in rtfTextBuf as "\{", even though it means "{".
 * These variables are also set when styles are reprocessed.
 */

extern char	*rtfTextBuf;		/* text of token */
extern short	rtfTextLen;		/* length of token in rtfTextBuf */
extern short	rtfClass;		/* token class */
extern short	rtfMajor;		/* token major number */
extern short	rtfMinor;		/* token minor number */
extern long	rtfParam;		/* control symbol parameter */

# define	rtfNoParam	(-1000000)

extern long	rtfLineNum;		/* input line number */
extern short	rtfLinePos;		/* input line position */

/*
 * For some reason, the no-style number is 222
 */

# define	rtfNoStyleNum		222
# define	rtfNormalStyleNum	0


/*
 * Token classes (must be zero-based and sequential)
 */

# define	rtfUnknown	0
# define	rtfGroup	1
# define	rtfText		2
# define	rtfControl	3
# define	rtfEOF		4
# define	rtfMaxClass	5	/* highest class + 1 */

/*
 * Group class major numbers
 */

# define	rtfBeginGroup	0
# define	rtfEndGroup	1

/* -------------------------------------------------------- */

/*
 * Control class major and minor numbers.
 *
 * To add a new symbol:
 * - edit rtf-controls
 * - run rtfprep to regenerate rtf-ctrl and rtf-ctrldef.h
 * - reinstall rtf-ctrl.
 * - recompile your translators.
 */

# include	"rtfprep/rtf-ctrldef.h"

/* -------------------------------------------------------- */


/*
 * \wmetafile argument values
 */

# define	rtfWmMmText		1
# define	rtfWmMmLometric		2
# define	rtfWmMmHimetric		3
# define	rtfWmMmLoenglish	4
# define	rtfWmMmHienglish	5
# define	rtfWmMmTwips		6
# define	rtfWmMmIsotropic	7
# define	rtfWmMmAnisotropic	8

/*
 * \pmmetafile argument values
 */

# define	rtfPmPuArbitrary	4
# define	rtfPmPuPels		8
# define	rtfPmPuLometric		12
# define	rtfPmPuHimetric		16
# define	rtfPmPuLoenglish	20
# define	rtfPmPuHienglish	24
# define	rtfPmPuTwips		28

/*
 * \lang argument values
 */

# define	rtfLangNoLang			0x0400
# define	rtfLangAlbanian			0x041c
# define	rtfLangArabic			0x0401
# define	rtfLangBahasa			0x0421
# define	rtfLangBelgianDutch		0x0813
# define	rtfLangBelgianFrench		0x080c
# define	rtfLangBrazilianPortuguese	0x0416
# define	rtfLangBulgarian		0x0402
# define	rtfLangCatalan			0x0403
# define	rtfLangLatinCroatoSerbian	0x041a
# define	rtfLangCzech			0x0405
# define	rtfLangDanish			0x0406
# define	rtfLangDutch			0x0413
# define	rtfLangAustralianEnglish	0x0c09
# define	rtfLangUKEnglish		0x0809
# define	rtfLangUSEnglish		0x0409
# define	rtfLangFinnish			0x040b
# define	rtfLangFrench			0x040c
# define	rtfLangCanadianFrench		0x0c0c
# define	rtfLangGerman			0x0407
# define	rtfLangGreek			0x0408
# define	rtfLangHebrew			0x040d
# define	rtfLangHungarian		0x040e
# define	rtfLangIcelandic		0x040f
# define	rtfLangItalian			0x0410
# define	rtfLangJapanese			0x0411
# define	rtfLangKorean			0x0412
# define	rtfLangBokmalNorwegian		0x0414
# define	rtfLangNynorskNorwegian		0x0814
# define	rtfLangPolish			0x0415
# define	rtfLangPortuguese		0x0816
# define	rtfLangRhaetoRomanic		0x0417
# define	rtfLangRomanian			0x0418
# define	rtfLangRussian			0x0419
# define	rtfLangCyrillicSerboCroatian	0x081a
# define	rtfLangSimplifiedChinese	0x0804
# define	rtfLangSlovak			0x041b
# define	rtfLangCastilianSpanish		0x040a
# define	rtfLangMexicanSpanish		0x080a
# define	rtfLangSwedish			0x041d
# define	rtfLangSwissFrench		0x100c
# define	rtfLangSwissGerman		0x0807
# define	rtfLangSwissItalian		0x0810
# define	rtfLangThai			0x041e
# define	rtfLangTraditionalChinese	0x0404
# define	rtfLangTurkish			0x041f
# define	rtfLangUrdu			0x0420

/* -------------------------------------------------------- */

/*
 * Defines for standard character names
 */

# include	"rtfprep/rtf-namedef.h"

/* -------------------------------------------------------- */

/*
 * CharSet indices
 */

# define	rtfCSGeneral	0	/* general (default) charset */
# define	rtfCSSymbol	1	/* symbol charset */
/* The following CharSet indices added by Ujwal Sathyam for rtf2latex2e */
# define	rtfCS1250	2	/* code page 1250 (Eastern European) */
# define	rtfCS1252	3	/* code page 1252 (ANSI) */
# define	rtfCS1253	4	/* code page 1253 (Greek) */
# define	rtfCS1254	5	/* code page 1254 (Turkish) */
# define	rtfCSMac	6	/* mac character set */
# define	rtfCS437	7	/* code page 437 */
# define	rtfCS850	8	/* code page 850 */

# define	macCharSet	256
# define	ansiCharSet	0
# define	pcCharSet	437
# define	pcaCharSet	850

# define	cp1252Enc	0
# define	symbolEnc	2
/* # define	macEnc		77    wrong?? */
# define	cp1253Enc	161
# define	cp1254Enc	162
# define	cp1258Enc	163
# define	cp1255Enc	177
# define	cp1256Enc	178
# define	cp1257Enc	186
# define	cp1251Enc	204
# define	cp874Enc	222
# define	cp1250Enc	238

/* end additions by Ujwal Sathyam */

/*
 * Flags for auto-charset-processing.  Both are on by default.
 */

# define	rtfReadCharSet		0x01	/* auto-read charset files */
# define	rtfSwitchCharSet	0x02	/* auto-switch charset maps */

/*
 * Style types
 */

# define	rtfParStyle	0	/* the default */
# define	rtfCharStyle	1
# define	rtfSectStyle	2

/*
 * RTF font, color and style structures.  Used for font table,
 * color table, and stylesheet processing.
 */

typedef struct RTFFont		RTFFont;
typedef struct RTFColor		RTFColor;
typedef struct RTFStyle		RTFStyle;
typedef struct RTFStyleElt	RTFStyleElt;


struct RTFFont
{
	char	*rtfFName;		/* font name */
	char	*rtfFAltName;		/* font alternate name */
	short	rtfFNum;		/* font number */
	short	rtfFFamily;		/* font family */
	short	rtfFCharSet;		/* font charset */
	short	rtfFPitch;		/* font pitch */
	short	rtfFType;		/* font type */
	short	rtfFCodePage;		/* font code page */
	RTFFont	*rtfNextFont;		/* next font in list */
};


/*
 * Color values are -1 if the default color for the the color
 * number should be used.  The default color is writer-dependent.
 */

struct RTFColor
{
	short		rtfCNum;	/* color number */
	short		rtfCRed;	/* red value */
	short		rtfCGreen;	/* green value */
	short		rtfCBlue;	/* blue value */
	RTFColor	*rtfNextColor;	/* next color in list */
};


struct RTFStyle
{
	char		*rtfSName;	/* style name */
	short		rtfSType;	/* style type */
	short		rtfSAdditive;	/* whether or not style is additive */
	short		rtfSNum;	/* style number */
	short		rtfSBasedOn;	/* style this one's based on */
	short		rtfSNextPar;	/* style next paragraph style */
	RTFStyleElt	*rtfSSEList;	/* list of style words */
	short		rtfExpanding;	/* non-zero = being expanded */
	RTFStyle	*rtfNextStyle;	/* next style in style list */
};


struct RTFStyleElt
{
	short		rtfSEClass;	/* token class */
	short		rtfSEMajor;	/* token major number */
	short		rtfSEMinor;	/* token minor number */
	long		rtfSEParam;	/* control symbol parameter */
	char		*rtfSEText;	/* text of symbol */
	RTFStyleElt	*rtfNextSE;	/* next element in style */
};


typedef	void (*RTFFuncPtr) ();		/* generic function pointer */


/*
 * Public RTF reader routines
 */

void		RTFSimpleInit ();
void 		RTFSetDefaultFont (long fontNumber);
void		RTFInit ();
void		RTFSetStream ();
void		RTFSetInputName ();
char		*RTFGetInputName ();
void		RTFSetOutputName ();
char		*RTFGetOutputName ();
void		RTFSetClassCallback ();
RTFFuncPtr	RTFGetClassCallback ();
void		RTFSetDestinationCallback ();
RTFFuncPtr	RTFGetDestinationCallback ();
void		RTFRead ();
short		RTFGetToken ();	/* writer should rarely need this */
void		RTFUngetToken ();
short		RTFPeekToken ();
void		RTFSetToken ();
void		RTFSetReadHook ();
RTFFuncPtr	RTFGetReadHook ();
void		RTFRouteToken ();
void		RTFSkipGroup ();
void		RTFExpandStyle ();
short		RTFCheckCM ();
short		RTFCheckCMM ();
short		RTFCheckMM ();
RTFFont		*RTFGetFont ();
RTFColor	*RTFGetColor ();
RTFStyle	*RTFGetStyle ();
# define	RTFAlloc(size)	_RTFAlloc ((long) (size))
char		*_RTFAlloc ();
char		*RTFStrSave ();
void		RTFFree ();
short		RTFCharToHex ();
short		RTFHexToChar ();
void		RTFSetMsgProc ();
void		RTFSetPanicProc ();
short		RTFPushedChar ();
void		RTFSetPushedChar (short lastChar);
void		ReadColorTbl (); /* made public and brought over from reader.c by Ujwal Sathyam */

/*
 * The following messing around is used to allow RTFMsg() and RTFPanic()
 * to be variable-argument functions that are declared publicly but
 * without generating prototype-mismatch errors on systems that have
 * stdarg.h.
 */

# ifndef rtfInternal
void		RTFMsg ();
void		RTFPanic ();
# else
# ifdef STDARG
void		RTFMsg (char *fmt, ...);
void		RTFPanic (char *fmt, ...);
# else
void		RTFMsg ();
void		RTFPanic ();
# endif	/* STDARG */
# endif /* rtfInternal */

short		RTFReadCharSetMap ();
void		RTFSetCharSetMap ();
short		RTFStdCharCode ();
char		*RTFStdCharName ();
short		RTFMapChar ();
short		RTFGetCharSet();
void		RTFSetCharSet();
short		RTFReadOutputMap (); /* added by Ujwal Sathyam. Forgotten by Paul du Bois? */
void 		RTFPushStack (void); /* added by Ujwal Sathyam */
void 		RTFPopStack (void);  /* added by Ujwal Sathyam */
void		RTFStoreStack (void); /* added by Ujwal Sathyam */
void		RTFRestoreStack (void); /* added by Ujwal Sathyam */

/*char		*RTFGetLibPrefix();*/
void	RTFSetOpenLibFileProc ();
FILE	*RTFOpenLibFile ();

#endif /* _RTF_H */