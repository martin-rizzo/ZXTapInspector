#include <stdio.h>

#if !defined(TRUE) && !defined(FALSE)
#   define TRUE  1
#   define FALSE 0
#   define BOOL int
#   define BYTE unsigned char
#endif


int detokenize_zx_basic_program(BYTE* data, unsigned length) {
    /* check if data is valid */
    if (data == NULL || length < 6) {
        printf("Error: Invalid data or insufficient length.\n");
        return -1;
    }

    /* print first 6 bytes in hexadecimal format */
    printf("%02x %02x %02x %02x %02x %02x\n", 
           data[0], data[1], data[2], data[3], data[4], data[5]);

    return 0;
}

