/*
| File    : zxs_bas.h
| Purpose : ZX-Spectrum BASIC detokenizer function.
| Author  : Martin Rizzo | <martinrizzo@gmail.com>
| Repo    : https://github.com/martin-rizzo/ZXTapInspector
| License : MIT
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|                               ZXTapInspector
|           A simple CLI tool for inspecting ZX-Spectrum TAP files
\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _*/
#ifndef ZXS_BAS_H
#define ZXS_BAS_H

#include <stdio.h>
#include <string.h>
#include "common.h"

/*
  Most of the info about ZX-Spectrum BASIC tokens is available at:
  - https://en.wikipedia.org/wiki/ZX_Spectrum_character_set#Character_set
*/

#define ZXS_COPYRIGHT_CHAR "{(C)}"
static const char* ZXS_CTRL_CHARS[] = {
/* 0x00 */   "{00}", "{01}", "{02}", "{03}", "{04}", "{05}",  "\t" , "{07}",
/* 0x08 */   "{08}", "{09}", "{0A}", "{0B}", "{0C}", "\n"  ,  ""   , "{0F}",
/* 0x10 */   "{INK %d}", "{PAPER %d}", "{FLASH %d}", "{BRIGHT %d}", "{INVERSE %d}", "{OVER %d}", "{AT %d,%d}", "{TAB %d,%d}",
/* 0x18 */   "{18}", "{19}", "{1A}", "{1B}", "{1C}", "{1D}", "{1E}", "{1F}"
};

#define            ZXS_GRAPH_CHARS_START 0x80
static const char* ZXS_GRAPH_CHARS[] = {
/* 0x80 */   "{-8}", "{-1}", "{-2}", "{-3}", "{-4}", "{-5}", "{-6}", "{-7}",
/* 0x88 */   "{+7}", "{+6}", "{+5}", "{+4}", "{+3}", "{+2}", "{+1}", "{+8}"
};

#define            ZXS_UDG_CHARS_START 0x90
static const char* ZXS_UDG_CHARS[] = {
/* 0x90 */   "{A}", "{B}", "{C}", "{D}", "{E}", "{F}", "{G}", "{H}",
/* 0x98 */   "{I}", "{J}", "{K}", "{L}", "{M}", "{N}", "{O}", "{P}",
/* 0xA0 */   "{Q}", "{R}", "{S}", "{T}", "{U}"
};

#define            ZXS_KEYWORDS_START 0xA3
static const char* ZXS_KEYWORDS[] = {
/* 0xA3 */                                         " SPECTRUM ", " PLAY "  , "RND"      , "INKEY$"  , "PI"      ,
/* 0xA8 */   "FN "       , "POINT "     , "SCREEN$ ", "ATTR "  , "AT "     , "TAB "     , "VAL$ "   , "CODE "   ,
/* 0xB0 */   "VAL "      , "LEN "       , "SIN "    , "COS "   , "TAN "    , "ASN "     , "ACS "    , "ATN "    ,
/* 0xB8 */   "LN "       , "EXP "       , "INT "    , "SQR "   , "SGN "    , "ABS "     , "PEEK "   , "IN "     ,
/* 0xC0 */   "USR "      , "STR$ "      , "CHR$ "   , "NOT "   , "BIN "    , " OR "     , " AND "   , "<="      ,
/* 0xC8 */   ">="        , "<>"         , " LINE "  , " THEN " , " TO "    , " STEP "   , " DEF FN ", " CAT "   ,
/* 0xD0 */   " FORMAT "  , " MOVE "     , " ERASE " , " OPEN #", " CLOSE #", " MERGE "  , " VERIFY ", " BEEP "  ,
/* 0xD8 */   " CIRCLE "  , " INK "      , " PAPER " , " FLASH ", " BRIGHT ", " INVERSE ", " OVER "  , " OUT "   ,
/* 0xE0 */   " LPRINT "  , " LLIST "    , " STOP "  , " READ " , " DATA "  , " RESTORE ", " NEW "   , " BORDER ",
/* 0xE8 */   " CONTINUE ", " DIM "      , " REM "   , " FOR "  , " GO TO " , " GO SUB " , " INPUT " , " LOAD "  ,
/* 0xF0 */   " LIST "    , " LET "      , " PAUSE " , " NEXT " , " POKE "  , " PRINT "  , " PLOT "  , " RUN "   ,
/* 0xF8 */   " SAVE "    , " RANDOMIZE ", " IF "    , " CLS "  , " DRAW "  , " CLEAR "  , " RETURN ", " COPY "
};


/**
 * Counts the number of '%' parameters in a string.
 * @param str The input string to analyze.
 * @return    The number of '%' characters found in the string.
 */
int zxs_count_parameters(const char* str) {
    int count = 0;
    for( ; *str ; ++str) {
        if (*str == '%') { count++; }
    }
    return count;
}

/**
 * Prints a ZX Spectrum BASIC line in human-readable format to a file.
 * @param file         FILE pointer to the output file.
 * @param data         Pointer to the byte array containing the BASIC line data.
 * @param datasize     Size of the data array in bytes.
 * @return             0 on success, or an error code indicating what went wrong,
 */
int zxs_fprint_basic_line(FILE* file, BYTE* data, unsigned datasize) {
    int i; BYTE byte; unsigned char last_char;
    const char *control_name; int param1, param2;
    const char *keyword;
    BOOL in_quotes, in_rem;
    int  err_code = 0;

    /* check parameters */
    if( file == NULL ) { err_code = 1; /* invalid parameter */ }
    if( data == NULL ) { datasize = 0; }

    /* iterates over the `data` buffer             */
    /* printing each byte in human-readable format */
    last_char = 0;
    in_quotes = in_rem = FALSE;
    for( i = 0 ; i < datasize && !err_code ; i++ )
    {
        keyword = control_name = NULL;
        byte    = data[i];

        /*== CONTROL CHARS ==*/
        if( byte < 0x20 ) {
            control_name = ZXS_CTRL_CHARS[byte];
            param1       = (i+1)<datasize ? data[i+1] : 0;
            param2       = (i+2)<datasize ? data[i+2] : 0;
            fprintf(file, control_name, param1, param2);

            /* 0E: marker for a 5-byte number (already printed in ASCII format), 
             * skip these bytes */
            if( byte == 0x0E ) { i += 5; }
            else               { i += zxs_count_parameters(control_name); }

        /*== ASCII CHARS ==*/
        } else if( byte < 0x80 ) {
            if( byte == 0x7F ) { fprintf(file, ZXS_COPYRIGHT_CHAR); }
            else               { fprintf(file, "%c", byte);         }

            /*== GRAPHICS CHARS ==*/
        } else if( byte < 0x90 ) {
            fprintf(file, "%s", ZXS_GRAPH_CHARS[byte - ZXS_GRAPH_CHARS_START]);

            /*== USER-DEFINED GRAPHICS CHARS (UDG) ==*/
        } else if( byte < (in_quotes ? 0xA5 : 0xA3) ) {
            fprintf(file, "%s", ZXS_UDG_CHARS[byte - ZXS_UDG_CHARS_START]);

        /*== KEYWORDS ==*/
        } else {
            keyword = ZXS_KEYWORDS[byte - ZXS_KEYWORDS_START];
            if( last_char == ' ' && keyword[0] == ' ' ) { ++keyword; }
            last_char = (keyword[ strlen(keyword)-1 ] == ' ');
            fprintf(file, "%s", keyword);
        }

        /* before closing the loop, */
        /* update the flags according to the processed byte */
        if( !keyword ) {  /* any non-keyword byte */
            last_char = byte;
        }
        if( byte == 0x22 ) { /* quote */
            if( !in_rem ) { in_quotes = !in_quotes; }
        }
        if( byte == 0xEA ) { /* REM */
            in_rem = TRUE;
        }
    }
    return err_code;
}

/**
 * Prints a ZX Spectrum BASIC program in human-readable format to a file.
 * @param file     FILE pointer to the output file.
 * @param data     Pointer to the byte array containing the tokenized BASIC program data.
 * @param datasize Size of the data array in bytes.
 * @return         0 on success, or an error code indicating what went wrong,
 */
int zxs_fprint_basic_program(FILE* file, BYTE* data, unsigned datasize) {
    const char BUFFER_READ_OVERFLOW_MSG[] = "Exceeding input buffer limit during detokenization";
    unsigned line_number, line_length;
    int err_code = 0;

    /* check parameters */
    if( file == NULL ) { err_code = 1; /* invalid parameter */ }
    if( data == NULL ) { datasize = 0; }

    /* process 'data' buffer (line by line) until all bytes have been consumed */
    while( datasize>0 && !err_code )
    {
        /* extract line number and length in the safest way possible */
        if( 2 > datasize ) { error(BUFFER_READ_OVERFLOW_MSG); return 1; }
        line_number = GET_BE_WORD(data, 0); data+=2; datasize-=2;
        if( line_number >= 16384 ) { return 0; }
        if( 2 > datasize ) { error(BUFFER_READ_OVERFLOW_MSG); return 1; }
        line_length = GET_LE_WORD(data, 0); data+=2; datasize-=2;
        if( line_length > datasize ) { error(BUFFER_READ_OVERFLOW_MSG); return 1; }
        
        /* process and print the BASIC line */
        printf("%5d", line_number);
        err_code = zxs_fprint_basic_line(file, data, line_length);
        if( err_code ) { return err_code; }
        /* printf("\n"); */

        data     += line_length;
        datasize -= line_length;
    }
    return err_code;
}

#endif /* ZXS_BAS_H */

