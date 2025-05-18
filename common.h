/*
| File    : common.h
| Purpose : Definitions, macros, and type aliases used throughout the project.
| Author  : Martin Rizzo | <martinrizzo@gmail.com>
| Repo    : https://github.com/martin-rizzo/ZXTapInspector
| License : MIT
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|                               ZXTapInspector
|           A simple CLI tool for inspecting ZX-Spectrum TAP files
\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _*/
#ifndef COMMON_H
#define COMMON_H


typedef unsigned char BYTE;
typedef int BOOL;
#define TRUE  1
#define FALSE 0

/**
 * @brief Macro to extract a 16-bit unsigned integer from a byte stream in little-endian format.
 * @param ptr    A pointer to the byte stream data.
 * @param index  The starting index (in bytes) from which to read the 16-bit word.
 * @return       A 16-bit unsigned integer formed by combining the two bytes in little-endian format.
 */
#define GET_LE_WORD(ptr, index) ( (((unsigned char *)ptr)[index + 1] << 8) | (((unsigned char *)ptr)[index]) )

/**
 * @brief Macro to extract a 16-bit unsigned integer from a byte stream in big-endian format.
 * @param ptr    A pointer to the byte stream data.
 * @param index  The starting index (in bytes) from which to read the 16-bit word.
 * @return       A 16-bit unsigned integer formed by combining the two bytes in big-endian format.
 */
#define GET_BE_WORD(ptr, index) ( (((unsigned char *)ptr)[index] << 8) | (((unsigned char *)ptr)[index + 1]) )

/**
 * Displays a warning message to stderr with color formatting
 * @param message The main warning message
 * @param ... Optional additional informational messages
 */
void warning(const char *message, ...);

/**
 * Displays an error message to stderr with color formatting
 * @param message The main error message
 * @param ...     Optional additional informational messages
 */
void error(const char *message, ...);

/**
 * Displays a fatal error message to stderr with color formatting and exits
 * @param message The main fatal error message
 * @param ... Optional additional informational messages
 */
void fatal_error(const char *message, ...);


#endif /* COMMON_H */
