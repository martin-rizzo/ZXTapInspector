#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
typedef unsigned char BYTE;
typedef int           BOOL;
#define TRUE  1
#define FALSE 0

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
typedef struct _BinaryCode {
    unsigned int start;        /**< Start address of the code block in memory */
    unsigned int end;          /**< End address of the code block in memory */
    unsigned int entry_point;  /**< Entry point address of the code block in memory */
    unsigned int datasize;     /**< Size of the data array in bytes */
    char         data[1];      /**< Flexible array containing the actual binary code data */
} BinaryCode;


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
void disable_colors() {
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
    static char text[1024];  /**< static buffer to store formatted text */

    /* format the variable arguments into the text buffer and print the colored error */
    vsnprintf(text, sizeof(text), format, vlist);
    fprintf(stderr, "\n%s[%s%s%s]%s %s\n", bra_color, err_color, err_type, bra_color, tex_color, text);
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

void print_help(char *argv[]) {
    printf("Usage: %s [options] <filename>\n", argv[0]);
}

/*------------------------------ .TAP LOADER ------------------------------*/

/**
 * A ZX-Spectrum TAP file block
 */
typedef struct _ZXTapBlock {
    unsigned flag;         /**< Control byte indicating block type (00 for headers, FF for data blocks) */
    unsigned checksum;     /**< 8-bit checksum of the data block for error detection */
    unsigned datasize;     /**< Size of the data array in bytes */
    BYTE     data[1];      /**< Flexible array containing the actual block data */
} ZXTapBlock;

/**
 * Reads a ZX Spectrum TAP file block from a file
 * @param tap_file The file pointer to the ZX Spectrum TAP file
 * @return A pointer to the allocated ZXTapBlock structure on success, or NULL on failure
 * @note   The caller is responsible for freeing the allocated memory using free()
 */
ZXTapBlock* read_zx_tap_block(FILE* tap_file) {
    ZXTapBlock* block = NULL;
    unsigned block_length, datasize;
    BYTE length[2], flag, checksum;
    BOOL error = FALSE;
    
    /* read the length of the block (2 bytes) */
    error        = fread(length, sizeof(length), 1, tap_file) != 1;
    block_length = (length[1] << 8) | length[0];
    datasize     = block_length - 2;

    /* read the spectrum generated data (flag + data + checksum) */
    block = malloc(sizeof(ZXTapBlock) + datasize);
    error = error || block == NULL;
    error = error || fread(&flag      , sizeof(BYTE),        1, tap_file) != 1;
    error = error || fread(block->data, sizeof(BYTE), datasize, tap_file) != datasize;
    error = error || fread(&checksum  , sizeof(BYTE),        1, tap_file) != 1;

    /* if there was an error then return NULL */
    if( error ) { free(block); return NULL; }

    /* set the block properties and return it */
    block->flag     = flag;
    block->checksum = checksum;
    block->datasize = datasize;
    return block;
}

BinaryCode* load_zx_tap(FILE* tap_file) {
    BinaryCode *code = NULL;
    ZXTapBlock *header, *data_block;
    BOOL block_is_valid;

    block_is_valid = TRUE; while( block_is_valid ) {
        header         = read_zx_tap_block(tap_file);
        block_is_valid = header != NULL;
        printf("Header: flag=%02X, checksum=%02X, size=%u bytes\n", header->flag, header->checksum, header->datasize);
        free( header );
    }
    return code;
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
    int  show_help = 0;
    int  show_version = 0;
    char filename[256] = "";
    int  non_flag_count = 0;
    char *param;
    BinaryCode *code;

    /* check if at least one parameter is provided */
    if (argc < 2) {
        error("No parameters were provided.");
        print_help(argv);
        return 1;
    }

    /* process each argument */
    for(i = 1; i < argc; i++) {
        param = argv[i];
        if( param[0] == '-' && param[1] == '-' ) {
            if      (ARG_EQ(param, "-h", "--help"   )) { show_help    = 1;  }
            else if (ARG_EQ(param, "-v", "--version")) { show_version = 1;  }
            else {
                fatal_error( "Unknown flag '%s'", param );
            }
        }
        else {
            /* assume this parameter is the filename */
            strncpy(filename, param, sizeof(filename)-1);
            filename[sizeof(filename)-1] = '\0';
            non_flag_count++;
        }
    }

    /* process the flags if they are activated */
    if(show_help) {
        print_help(argv);
        return 0;
    }
    if(show_version) {
        printf("Version 0.1.0\n");
        return 0;
    }

    /* check that exactly one filename was provided */
    if( non_flag_count != 1 ) { fatal_error("Exactly one filename was expected", "TEST"); }

    printf("Provided file: %s\n", filename);

    /* open the file and read it's contents */
    FILE *file = fopen(filename, "rb");
    if( !file ) {
        fatal_error("Failed to open file '%s'", filename);
    }
    code = load_zx_tap(file);
    free(code);
    fclose(file);

    return 0;
}

