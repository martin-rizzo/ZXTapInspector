/*
| File    : file_dir.h
| Purpose : Helper functions for file and directory operations.
| Author  : Martin Rizzo | <martinrizzo@gmail.com>
| Repo    : https://github.com/martin-rizzo/ZXTapInspector
| License : MIT
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|                               ZXTapInspector
|           A simple CLI tool for inspecting ZX-Spectrum TAP files
\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _*/
#ifndef FILE_DIR_H
#define FILE_DIR_H
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#   include <sys/stat.h>
#endif

char* strdup_(const char* str) {
    char *allocated_str;
    return str && (allocated_str=malloc(strlen(str)+2)) ? strcpy(allocated_str, str) : NULL;
}

/**
 * Extracts the filename from a given path.
 * @param path A string representing the file path.
 * @return
 *     A pointer to the start of the filename in the path.
 */
const char* get_filename(const char* path) {
    int i, last_sep = -1;
    for( i = 0 ; path[i] != '\0' ; i++ ) {
        if( path[i]=='/' || path[i]=='\\' ) { last_sep = i; }
    }
    return path + (last_sep + 1);
}

/**
 * Allocates memory for a filename without its extension.
 * @param path A string representing the full file path.
 * @return
 *    The allocated filename without the extension or NULL on failure.
 *    The caller is responsible for freeing this memory.
 */
char* alloc_name(const char* path) {
    char* name = strdup_( get_filename( path ) );
    char* dot  = strrchr(name, '.');
    if( dot ) { *dot = '\0'; }
    return name;
}

/**
 * Concatenates five strings into a single allocated buffer.
 * 
 * @param str1 Optional first string to concatenate (may be NULL)
 * @param str2 Optional second string to concatenate (may be NULL)
 * @param str3 Optional third string to concatenate (may be NULL)
 * @param str4 Optional fourth string to concatenate (may be NULL)
 * @param str5 Optional fifth string to concatenate (may be NULL)
 * @return
 *    A pointer to the allocated buffer containing the concatenated strings.
 *    The caller is responsible for freeing this memory.
 */
char* alloc_concat5(const char *str1, const char *str2, const char *str3, const char *str4, const char *str5) {
    int    len1, len2, len3, len4, len5;
    size_t total_length;
    char   *output, *ptr;

    /* calculate the total length of all strings including the null terminator */
    total_length = 0;
    if( str1 ) { total_length += (len1=strlen(str1)); }
    if( str2 ) { total_length += (len2=strlen(str2)); }
    if( str3 ) { total_length += (len3=strlen(str3)); }
    if( str4 ) { total_length += (len4=strlen(str4)); }
    if( str5 ) { total_length += (len5=strlen(str5)); }
    total_length += 2;

    /* copy each string into the output buffer */
    ptr = output = (char*)calloc(total_length, sizeof(char));
    if( str1 ) { strcpy(ptr, str1); ptr += len1; }
    if( str2 ) { strcpy(ptr, str2); ptr += len2; }
    if( str3 ) { strcpy(ptr, str3); ptr += len3; }
    if( str4 ) { strcpy(ptr, str4); ptr += len4; }
    if( str5 ) { strcpy(ptr, str5); ptr += len5; }

    /* add a double null terminator */
    ptr[0] = ptr[1] = '\0';
    return output;
}

#if defined(_WIN32)
/**
 * Converts a UTF-8 string to a wide string (UTF-16) for Windows.
 * 
 * @param utf8_str A UTF-8 string.
 * @return
 *    A pointer to the allocated wide string (UTF-16), or NULL if the input is invalid.
 *    The caller is responsible for freeing this memory.
 */
wchar_t* alloc_wide_string(const char* utf8_str) {
    wchar_t* wide_str;
    size_t utf8_length, wide_length;
    if( utf8_str==NULL)  { return NULL; }

    /* calculate required buffer size */
    size_t utf8_length = strlen(utf8_str);
    wide_length = MultiByteToWideChar(CP_UTF8, 0, utf8_str, (int)utf8_length, NULL, 0);
    if( wide_length==0 ) { return NULL; }

    /* allocate memory for wide string */
    wide_str = (wchar_t*)malloc((wide_length + 1) * sizeof(wchar_t));
    if( wide_str==NULL ) { return NULL; }

    /* Perform the actual conversion */
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, (int)utf8_length, wide_str, (int)wide_length);
    return wide_str;
}
#endif

/**
 * Checks if a file or directory exists at the specified path.
 * 
 * @param path The file or directory path.
 * @return
 *    TRUE if the path exists, or FALSE if it does not.
 */
int path_exists(const char *path) {
    #ifdef _WIN32
        /* windows specific code */
        wchar_t* wide_path       = wide_string_from_utf8(path);
        DWORD    file_attributes = GetFileAttributesW(wide_path);
        free(wide_path);
        return (file_attributes != INVALID_FILE_ATTRIBUTES);
    #else
        return access(path, F_OK) == 0;
    #endif
}

/**
 * @brief Returns the first available path to not overwrite an existing file or dir.
 * 
 * @param dir      The directory path. (may be NULL or empty)
 * @param filename The base filename without extension, e.g., "file". (must be a valid string)
 * @param ext      The file extension including the dot, e.g., ".txt". (may be NULL or empty)
 * @return
 *    A dynamically allocated string containing the unique path.
 *    The caller is responsible for freeing this memory.
 */
char* alloc_unique_path(const char* dir, const char* filename, const char* ext) {
    char *path;
    int number; char number_str[16];
    BOOL unique;
    const char* dir_end = "/";
    char last_dir_char;
    assert( filename!=NULL );
    
    /* check the need for a directory separator at the end of the directory */
    last_dir_char = dir!=NULL && dir[0]!='\0' ? dir[ strlen(dir)-1 ] : '\0';
    if( last_dir_char == '\0' || last_dir_char=='/' || last_dir_char=='\\' ) {
        dir_end = NULL;
    }
    path   = alloc_concat5(dir, dir_end, filename, ext, NULL);
    unique = !path_exists(path);
    for( number = 2; !unique && number <= 9999; ++number ) {
        sprintf(number_str, "_%d_", number);
        free( path );
        path   = alloc_concat5(dir, dir_end, filename, number_str, ext);
        unique = !path_exists(path);
    }
    return path;
}

/**
 * Creates a directory at the specified path.
 * @param dir_path The directory path to create.
 * @return
 *   - TRUE if the directory was successfully created.
 *   - FALSE if the directory path is invalid or directory creation failed.
 */
BOOL create_directory(const char *path) {
    BOOL result = TRUE;

    assert( path!=NULL );
    if( !path || path[0]=='\0' )
    { error("Invalid directory path '%s'\n", path); return FALSE; }
#   ifdef _WIN32
        /* windows specific code */
        wide_path = wide_string_from_utf8(path);
        if( !CreateDirectoryW(wide_path, NULL) )
        { error("Failed to create directory '%s'\n", path); result=FALSE; }
        free(wide_dir_path);
#   else
        /* linux/mac specific code */
        if( mkdir(path, 0777) != 0 )
        { error("Failed to create directory '%s'.\n", path); result=FALSE; }
#   endif
    return result;
}


#endif /* FILE_DIR_H */
