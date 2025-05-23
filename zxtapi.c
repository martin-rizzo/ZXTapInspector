/*
| File    : zxtapi.c
| Purpose : Main file for the ZXTapInspector tool.
| Author  : Martin Rizzo | <martinrizzo@gmail.com>
| Repo    : https://github.com/martin-rizzo/ZXTapInspector
| License : MIT
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|                               ZXTapInspector
|    A simple CLI tool for inspecting and extracting ZX-Spectrum TAP files
|
|    Copyright (c) 2025 Martin Rizzo
|
|    Permission is hereby granted, free of charge, to any person obtaining
|    a copy of this software and associated documentation files (the
|    "Software"), to deal in the Software without restriction, including
|    without limitation the rights to use, copy, modify, merge, publish,
|    distribute, sublicense, and/or sell copies of the Software, and to
|    permit persons to whom the Software is furnished to do so, subject to
|    the following conditions:
|
|    The above copyright notice and this permission notice shall be
|    included in all copies or substantial portions of the Software.
|
|    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
|    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
|    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
|    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
|    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
|    TORT OR OTHERWISE, ARISING FROM,OUT OF OR IN CONNECTION WITH THE
|    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "common.h"
#include "file_dir.h"
#include "zxs_bas.h"
#include "zxs_tap.h"
#include "fmt_hex.h"
const char* HELP[] = {
"Usage: zxtapi [OPTIONS] FILE.tap"                                                       ,
""                                                                                       ,
"Description:"                                                                           ,
"  ZXTapInspector (zxtapi) is a command-line tool for inspecting ZX Spectrum .tap files.",
"  It enables you to list blocks, view detailed block information, extract BASIC code,"  ,
"  and convert tape data into usable file formats."                                      ,
""                                                                                       ,
"Options:"                                                                               ,
"  -l, --list"                                                                           ,
"        List all blocks contained in the .tap file."                                    ,
""                                                                                       ,
"  -p, --print <n>"                                                                      ,
"        Print the specified block. The parameter can be either:"                        ,
"          - A numeric index (e.g., \"1\" for the first block)"                          ,
"          - A block name prefixed with a colon (e.g., \":loader\")"                     ,
"        Depending on the block type, it is displayed in an appropriate format"          ,
""                                                                                       ,
"  -b, --basic"                                                                          ,
"        Output the first BASIC program found within the .tap file."                     ,
""                                                                                       ,
"  -c, --code"                                                                           ,
"        Output the first binary code found within the .tap file."                       ,
""                                                                                       ,
"  -x, --extract"                                                                        ,
"        Extract all blocks from the .tap file into separate files:"                     ,
"          - any BASIC program is saved as a .bas untokenized text file."                ,
"          - any binary code is saved as a Intel HEX (.hex) format."                     ,
"        The extracted files are placed in a folder named after the original tape file." ,
""                                                                                       ,
"  -h, --help"                                                                           ,
"        Show this help message and exit."                                               ,
""                                                                                       ,
"  -v, --version"                                                                        ,
"        Display version information."                                                   ,
""                                                                                       ,
"Examples:"                                                                              ,
"  zxtapi example.tap"                                                                   ,
"      List all blocks found within 'example.tap'."                                      ,
""                                                                                       ,
"  zxtapi -d example.tap"                                                                ,
"      Show detailed block information for 'example.tap'."                               ,
""                                                                                       ,
"  zxtapi -b example.tap"                                                                ,
"      Output the first BASIC code stored in 'example.tap'."                             ,
""                                                                                       ,
"  zxtapi -x example.tap"                                                                ,
"      Extract and convert all blocks from 'example.tap' into separate files."           ,
"", NULL
};

/* The index of the first header in a TAP file */
#define FIRST_HEADER_INDEX 1

/**
 * Macro to check the arguments passed by command line
 * @param arg   The argument to compare.
 * @param str1  The first string to compare against.
 * @param str2  The second string to compare against.
 * @return True if `arg` matches either `str1` or `str2`, otherwise false.
 */
#define ARG_EQ(arg, str1, str2) (strcmp(arg, str1) == 0 || strcmp(arg, str2) == 0)

/**
 * A binary code block in memory
 */
typedef struct BinaryCode {
    unsigned int start;        /**< Start address of the code block in memory */
    unsigned int end;          /**< End address of the code block in memory */
    unsigned int entry_point;  /**< Entry point address of the code block in memory */
    unsigned int datasize;     /**< Size of the data array in bytes */
    char         data[1];      /**< Flexible array containing the actual binary code data */
} BinaryCode;


const char *get_selected_name(const char* print_param) {
    if( !print_param ) { return ""; }
    if( print_param[0] == ':' ) { return &print_param[1]; }
    return NULL;
}

/*---------------------------- COLORED OUTPUT -----------------------------*/

/**
 * ANSI escape codes for colored terminal output
 */
char RED[]     = "\033[91m";
char GREEN[]   = "\033[92m";
char YELLOW[]  = "\033[93m";
char CYAN[]    = "\033[96m";
char DKGRAY[]  = "\033[90m";
char NOCOLOR[] = "\033[0m";

/**
 * Disables color output by resetting all color codes to empty strings
 */
void disable_colors(void) {
    RED[0]     = '\0';
    GREEN[0]   = '\0';
    YELLOW[0]  = '\0';
    CYAN[0]    = '\0';
    DKGRAY[0]  = '\0';
    NOCOLOR[0] = '\0';
}

/*---------------------------- ERROR MESSAGES -----------------------------*/

/**
 * Prints an error message with colored formatting.
 * @param err_type   The type of error (e.g., "Error", "Warning").
 * @param err_color  The color code for the error type.
 * @param bra_color  The color code for the brackets enclosing the error type.
 * @param tex_color  The color code for the text content.
 * @param format     The format string for the error message.
 * @param vlist      The variable arguments list.
 */
void print_colored_error(const char *err_type,
                         const char *err_color,
                         const char *bra_color,
                         const char *tex_color,
                         const char *format, va_list vlist) {
    fprintf (stderr, "\n%s[%s%s%s]%s ", bra_color, err_color, err_type, bra_color, tex_color);
    vfprintf(stderr, format, vlist);
    fprintf (stderr, "\n");
}

/**
 * Displays a warning message to stderr with color formatting
 * @param message The main warning message
 * @param ... Optional additional informational messages
 */
void warning(const char *message, ...) {
    char *info; va_list args;
    va_start(args, message);
    print_colored_error("WARNING", YELLOW, CYAN, NOCOLOR, message, args);
    va_end(args);
}

/**
 * Displays an error message to stderr with color formatting
 * @param message The main error message
 * @param ...     Optional additional informational messages
 */
void error(const char *message, ...) {
    char *info; va_list args;
    va_start(args, message);
    print_colored_error("ERROR", RED, CYAN, NOCOLOR, message, args);
    va_end(args);
}

/**
 * Displays a fatal error message to stderr with color formatting and exits
 * @param message The main fatal error message
 * @param ... Optional additional informational messages
 */
void fatal_error(const char *message, ...) {
    char *info; va_list args;
    va_start(args, message);
    print_colored_error("ERROR", RED, CYAN, NOCOLOR, message, args);
    va_end(args);
    exit(1);
}

/**
 * Displays help messages to stdout.
 * 
 * Although `argc` and `argv` are not used in this implementation, it allows
 * for potential future extensions where context-specific help could be
 * determined based on arguments.
 *
 * @param argc Number of command-line arguments (typically from `main()`).
 * @param argv Array of command-line arguments (typically from `main()`).
 */
void print_help(int argc, char *argv[]) {
    int i = 0; while( HELP[i] ) { printf("%s\n",HELP[i++]); }
}


/*---------------------- BLOCK HEADER/DATA FUNCTIONS ----------------------*/

/**
 * Searches for a specific header in a TAP file based on optional criteria.
 * 
 * The criteria can be based on name, index, or header type. If multiple criteria are provided,
 * the function prioritizes name checks first, then index checks.
 * 
 * @param header    Pointer to the ZXHeaderInfo where the matched header will be stored. 
 * @param tap_file  FILE pointer to the TAP file being processed.
 * @param name      Optional block name to match. If NULL, name-based filtering is skipped.
 * @param index     Optional block index to match. If -1, index-based filtering is skipped.
 * @param type      Header type to match. This parameter is only used if both name and index are omitted.
 * @return
 *    TRUE if a matching header is found, FALSE otherwise.
 */
BOOL find_zx_tap_header(ZXSHeader* header, FILE* tap_file, const char* name, int index, ZXS_DATATYPE type) {
    ZXSTapBlock *block;
    int          header_index;
    BOOL is_block_valid, found;

    /* loop through all TAP blocks until the target header is found */
    header_index   = FIRST_HEADER_INDEX;
    is_block_valid = TRUE; found = FALSE;
    while( is_block_valid && !found )
    {
        /* read next block from TAP file */
        block           = zxs_read_tap_block(tap_file);
        is_block_valid  = (block != NULL);
        if( is_block_valid && zxs_parse_header(header, block) ) {
            /* check if the current header matches the selected criteria */
            if( !found && name     ) { found = (0==strcmp(header->filename, name)); }
            if( !found && index>=0 ) { found = (header_index == index); }
            if( !name && index<0   ) {
                if( type == ZXS_DATATYPE_ANY ) { found = TRUE; }
                else                           { found = (header->datatype == type ); }
            }
            ++header_index;
        }
        free( block ); block = NULL;
    }
    return found;
}

/**
 * Prints data from a ZX TAP file block based on the header's data type.
 * 
 * This function reads the next data block in the TAP file and prints its
 * contents to the specified output. The function assumes the provided header
 * is valid and corresponds to the data block to be printed.
 * 
 * @param output     FILE pointer to the output file where data will be printed. (may be stdout)
 * @param tap_file   FILE pointer to the TAP file being processed.
 * @param header     Pointer to a ZXSHeader containing the block header information.
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_zx_tap_data(FILE* output, FILE* tap_file, const ZXSHeader* header) {
    ZXSTapBlock *block;
    int err_code = 0;

    /* read the data block, assuming that the header has already been read */
    block = zxs_read_tap_block(tap_file);
    switch( header->datatype ) {

        case ZXS_DATATYPE_BASIC:
            if( !err_code && block==NULL )
            { err_code = 1; error("Error reading BASIC program, no data block found"); }
            if( !err_code )
            { err_code = !err_code && zxs_fprint_basic_program(output, block->data, block->datasize); }
            break;

        case ZXS_DATATYPE_NUMBERS:
            err_code = 1; error("Number array blocks are not supported yet.");
            break;

        case ZXS_DATATYPE_STRINGS:
            err_code = 1; error("String array blocks are not supported yet.");
            break;

        case ZXS_DATATYPE_CODE:
            if( !err_code && block==NULL )
            { err_code = 1; error("Error reading binary code, no data block found"); }
            if( !err_code )
            { err_code = fprint_hex_data(output, header->param1, block->data, block->datasize); }
            break;

        default:
            err_code = 1; error("Unknown data type in header (%d).", header->datatype);
            break;
    }
    /* free the data block and return */
    free(block);
    return err_code;
}

/*------------------------------ SUB-COMMANDS ------------------------------*/

/**
 * Prints a formatted list of all TAP blocks in a TAP file.
 * @param output    FILE pointer to the output stream where the block list will be printed.
 * @param tap_file  FILE pointer to the TAP file being processed.
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_block_list(FILE* output, FILE* tap_file) {
    ZXSTapBlock *block;
    ZXSHeader    header;
    int          header_index, block_index;
    BOOL         is_block_valid;
    char buffer20[20];
    char datatype_name_buffer[32];
    int  err_code = 0;
    static const char THEADER[]=" IDX | name       | type          | Length | Param1 | Param2 |\n";
    static const char TLINE[]  ="-----|------------|---------------|--------|--------|--------|\n";
    static const BOOL padding = TRUE;

    /* loop through all TAP blocks */
    header_index   = FIRST_HEADER_INDEX;
    block_index    = 0;
    is_block_valid = TRUE;
    fprintf(output, "%s%s%s", padding ? "\n" : "", THEADER, TLINE);
    while( is_block_valid )
    {
        /* read next block from TAP file */
        block           = zxs_read_tap_block(tap_file);
        is_block_valid  = (block != NULL);
        if( is_block_valid ) {
            if( zxs_parse_header(&header, block) )
            {
                if( header_index != FIRST_HEADER_INDEX ) { fprintf(output, TLINE); }
                sprintf(buffer20, "\":%s\"", header.filename);
                fprintf(output, " %3d  :%-12s %-15s %6d   %6d   %6d\n",
                       header_index, header.filename,
                       zxs_get_datatype_name(header.datatype, datatype_name_buffer),
                       header.length, header.param1, header.param2
                       );
                ++header_index; block_index=0;
            }
            else {
                sprintf(buffer20, "\\data%d", block_index);
                fprintf(output, "       %-12s %-15s %6d\n", 
                       "", buffer20, block->datasize
                       );
            }
        }
        free( block ); block = NULL;
    }
    fprintf(output, "%s%s", TLINE, padding ?  "\n" : "");
    return err_code;
}

/**
 * Prints a detokenized ZX Spectrum BASIC program from a TAP file.
 * 
 * This function searches for a BASIC program block in a TAP file, reads its data,
 * and prints the detokenized BASIC program to the output file. It handles error cases
 * such as missing blocks or invalid block types.
 * 
 * @param output         File pointer to the output file where the BASIC program will be printed.
 * @param tap_file       File pointer to the TAP file containing the ZX Spectrum data.
 * @param selected_name  Optional block name to match. If NULL, name-based filtering is skipped.
 * @param selected_idx   Optional block index to match. If -1, index-based filtering is skipped.
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_zx_basic_program(FILE* output, FILE* tap_file, const char* selected_name, int selected_idx) {
    ZXSHeader header; BOOL found;
    int err_code = 0;

    /* search for the selected BASIC program header in the TAP file */
    found = find_zx_tap_header(&header, tap_file, selected_name, selected_idx, ZXS_DATATYPE_BASIC);

    /* handle error cases */
    if( !found  )
    { error("No BASIC program found"); return err_code=1; }
    if( found && header.datatype!=ZXS_DATATYPE_BASIC )
    { error("Selected block is not a BASIC program"); return err_code=1; }

    /* print the actual BASIC program */
    err_code = fprint_zx_tap_data(output, tap_file, &header);
    return err_code;
}

/**
 * Prints a binary code from a TAP file.
 * 
 * This function searches for a binary code block in a TAP file, reads its data, and prints
 * the content in Intel HEX format. It supports optional filtering by filename or index.
 * 
 * @param output         File pointer to the output file where the binary data will be printed.
 * @param tap_file       File pointer to the TAP file containing the ZX Spectrum data.
 * @param selected_name  Optional block name to match. If NULL, name-based filtering is skipped.
 * @param selected_idx   Optional block index to match. If -1, index-based filtering is skipped.
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_zx_binary_code(FILE* output, FILE* tap_file, const char* selected_name, int selected_idx) {
    ZXSHeader header; BOOL found;
    int err_code = 0;

    /* search for the selected binary code in the TAP file */
    found = find_zx_tap_header(&header, tap_file, selected_name, selected_idx, ZXS_DATATYPE_CODE);

    /* handle error cases */
    if( !found  )
    { error("No binary code found"); return err_code=1; }
    if( found && header.datatype!=ZXS_DATATYPE_CODE )
    { error("Selected block is not a binary code"); return err_code=1; }

    /* print the actual binary code */
    err_code = fprint_zx_tap_data(output, tap_file, &header);
    return err_code;
}

/**
 * Prints any type of ZX Spectrum block from a TAP file.
 * 
 * This function searches for a block in a TAP file matching the provided name or index,
 * then prints the block content based on its type. It supports BASIC programs, binary code,
 * and other block types, though some types may not be fully implemented.
 * 
 * @param output         File pointer to the output file where the block content will be printed.
 * @param tap_file       File pointer to the TAP file containing the ZX Spectrum data.
 * @param selected_name  Optional block name to match. If NULL, name-based filtering is skipped.
 * @param selected_idx   Optional block index to match. If -1, index-based filtering is skipped.
 * @return
 *    0 on success, or an error code indicating what went wrong
 */
int fprint_any_zx_block(FILE* output, FILE* tap_file, const char* selected_name, int selected_idx) {
    ZXSHeader header; ZXSTapBlock *block; BOOL found;
    int err_code = 0;
    
    /* search for the first header that matches the given name or index */
    /* and if it is found, print the next block content                 */
    found = find_zx_tap_header(&header, tap_file, selected_name, selected_idx, ZXS_DATATYPE_ANY);
    if( found ) {
        err_code = fprint_zx_tap_data(output, tap_file, &header);
    }
    else {
        /* handle the not-found error */
        if( selected_name   ) { err_code=1; error("No block found with name \"%s\"", selected_name); }
        if( selected_idx>=0 ) { err_code=1; error("No block at index %d", selected_idx); }
    }
    return err_code;
}

int extract_zx_block(const char* output_dir, const char* output_name, FILE* tap_file, const ZXSHeader *header) {
    char *output_ext=NULL, *output_path=NULL;
    FILE *output=NULL;
    int err_code = 0;

    switch( header->datatype ) {
        case ZXS_DATATYPE_BASIC:  output_ext = ".bas"; break;
        case ZXS_DATATYPE_CODE :  output_ext = ".hex"; break;
        default:
            output_ext = ".txt";
            break;
    }
    if( !err_code ) {
        output_path = alloc_unique_path(output_dir, output_name, output_ext);
        if( !output_path ) { err_code=1; error("Cannot allocate memory for output path"); }
    }
    if( !err_code ) {
        output = fopen(output_path, "wb");
        if( !output ) { err_code=1; error("Cannot open output file \"%s\"", output_path); }
    }
    if( !err_code ) {
        err_code = fprint_zx_tap_data(output, tap_file, header);
    }
    if( output     ) { fclose(output);    }
    if( output_path) { free(output_path); }
    return err_code;
}

int extract_all_zx_blocks(const char* dir_name, FILE* tap_file, const char* selected_name, int selected_idx) {
    ZXSTapBlock *block;
    ZXSHeader    header;
    int          header_index;
    BOOL is_block_valid, is_header_valid, found;
    char *output_dir, *output_name;
    int err_code = 0;

    if( dir_name==NULL || dir_name[0]=='\0' )  {
        dir_name = "output";
    }
    output_dir = alloc_unique_path(NULL, dir_name, NULL);
    if( !create_directory(output_dir) ) {
        error("Cannot create output directory \"%s\"", dir_name);
        err_code = 1;
    }
    
    /* loop through all TAP blocks extracting the selected ones */
    header_index   = FIRST_HEADER_INDEX;
    is_block_valid = TRUE;
    while( is_block_valid && !err_code )
    {
        /* read next block from TAP file */
        block           = zxs_read_tap_block(tap_file);
        is_block_valid  = (block != NULL);
        is_header_valid = zxs_parse_header(&header, block);
        if( is_header_valid )
        {
            /* check if the current block matches the selected criteria */
            found = FALSE;
            if( !found && selected_name   ) { found = strcmp(header.filename, selected_name)==0; }
            if( !found && selected_idx>=0 ) { found = header_index==selected_idx;                }
            if( !selected_name && selected_idx<0 ) { found = TRUE; }

            /* placeholder for block extraction */
            if( found ) {
                output_name = strlen(header.filename)>0 ? header.filename : "data";
                err_code = extract_zx_block(output_dir, output_name, tap_file, &header);
            }
            ++header_index;
        }
        free( block ); block = NULL;
    }
    free( output_dir );
    return err_code;
}


/**
 * Converts ZX-Spectrum TAP file to HEX Intel format
 * @param output_filename  The name of the output file where the converted HEX data will be written.
 *                         If the file already exists, it will be overwritten. 
 * @param tap_file         Pointer to the TAP file stream (must be opened in read mode).
 * @return 0 on successful completion. 
 */
int convert_zx_tap_to_hex(const char* output_filename, FILE* tap_file) {
    fatal_error( "Not implemented yet" );
    return 0;
}

/*===========================================================================
/////////////////////////////////// MAIN ////////////////////////////////////
===========================================================================*/

/**
 * Main function to process command-line arguments and handle file operations.
 * 
 * This function processes command-line arguments to check for help, version, and filename.
 * It validates the input and performs the necessary actions based on the provided flags.
 * 
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on success, 1 on error.
 */
int main(int argc, char *argv[]) {
    int  i;
    FILE *tap_file = NULL;
    char filename[1024] = "";
    int  non_flag_count = 0;
    char *arg;
    BinaryCode *code;
    int err_code = 0;
    char *dir_name = NULL;
    const char *selected_name  = NULL;
    int         selected_index = FIRST_HEADER_INDEX;
    enum { CMD_HELP, CMD_VERSION, CMD_LIST, CMD_DETAILS, CMD_PRINT, CMD_BASIC, CMD_BINARY, CMD_EXTRACT } cmd;

    /* check if at least one parameter is provided */
    if (argc < 2) {
        error("No parameters were provided.");
        print_help(argc,argv);
        return 1;
    }

    /* process each argument */
    cmd = CMD_LIST;
    for(i = 1; i < argc; i++) {
        arg = argv[i];
        if( arg[0] == '-' ) {
            if      (ARG_EQ(arg, "-l", "--list"   )) { cmd = CMD_LIST;    }
            else if (ARG_EQ(arg, "-d", "--detail" )) { cmd = CMD_DETAILS; }
            else if (ARG_EQ(arg, "-p", "--print"  )) { cmd = CMD_PRINT; ++i;
                if( i >= argc ) { fatal_error("Missing value for --print"); }
                selected_name  = get_selected_name(argv[i]);
                selected_index = selected_name ? -1 : atoi(argv[i]);
            }
            else if (ARG_EQ(arg, "-b", "--basic"  )) { cmd = CMD_BASIC;   }
            else if (ARG_EQ(arg, "-c", "--code"   )) { cmd = CMD_BINARY;  }
            else if (ARG_EQ(arg, "-x", "--extract")) { cmd = CMD_EXTRACT; }
            else if (ARG_EQ(arg, "-h", "--help"   )) { cmd = CMD_HELP;    }
            else if (ARG_EQ(arg, "-v", "--version")) { cmd = CMD_VERSION; }
            else {
                fatal_error( "Unknown flag '%s'", arg );
            }
        }
        else {
            /* assume this parameter is the filename */
            strncpy(filename, arg, sizeof(filename)-1);
            filename[sizeof(filename)-1] = '\0';
            non_flag_count++;
        }
    }

    /* handle help & version commands */
    switch( cmd ) {
        case CMD_HELP:
            print_help(argc,argv);
            return 0;
        case CMD_VERSION:
            printf("Version 0.1.0\n");
            return 0;
        default:
            break;
    }

    /* check that exactly one filename was provided */
    if( non_flag_count != 1 ) {
        fatal_error("Exactly one filename was expected");
    }

    /* proceed with file operations based on the selected command */
    err_code  = 0;
    tap_file = fopen(filename, "rb");
    if( !tap_file ) { fatal_error("Failed to open file '%s'", filename); }
    switch( cmd ) {
        case CMD_LIST:
            err_code = fprint_block_list(stdout, tap_file);
            break;
        case CMD_DETAILS:
            err_code = fprint_block_list(stdout, tap_file);
            break;
        case CMD_PRINT:
            err_code = fprint_any_zx_block(stdout, tap_file, selected_name, selected_index);
            break;
        case CMD_BASIC:
            err_code = fprint_zx_basic_program(stdout, tap_file, NULL, -1);
            break;
        case CMD_BINARY:
            err_code = fprint_zx_binary_code(stdout, tap_file, NULL, -1);
            break;
        case CMD_EXTRACT:
            dir_name = alloc_name(filename);
            err_code = extract_all_zx_blocks(dir_name, tap_file, NULL, -1);
            free(dir_name);
            break;
        default:
            fatal_error( "Unknown command '%d'", cmd );
    }
    fclose(tap_file);
    return err_code;
}

