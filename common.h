/*
| File    : common.h
| Purpose : Definitions, macros, and type aliases used throughout the project.
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
