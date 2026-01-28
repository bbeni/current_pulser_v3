// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
#define _GNU_SOURCE
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

#include "uti.h"


#ifdef _WIN32

// noisy header -> disable warnings
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
#include "dirent_win32ports.h"
// resotore warnings
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#else //_WIN32
#include "dirent.h"
#endif  //_WIN32

bool uti_read_entire_file(const char *path, char** content, size_t* out_size) {
    char* mem_block = NULL;
    FILE *file = fopen(path, "rb");
    if (!file) goto error;
    if (fseek(file, 0, SEEK_END) < 0) goto error;

    long long length = 0;
#ifndef _WIN32
    length = ftell(file);
#else
    length = _ftelli64(file);
#endif

    if (length < 0) goto error;
    if (fseek(file, 0, SEEK_SET) < 0) goto error;

    mem_block = (char*)malloc(length+1);
    if (mem_block == NULL) {
        printf("ERROR: malloc failed in %s:%d\n", __FILE__, __LINE__);
        goto error;
    }

    mem_block[length] = '\0'; // for good measure

    fread(mem_block, length, 1, file);
    if (ferror(file)) goto error;

    fclose(file);
    // if all is fine, set it for output
    *content = mem_block;
    *out_size = length;
    return true;

error:
    printf("ERROR: Could not read file %s: %s\n", path, strerror(errno));
    if (file) fclose(file);
    if (mem_block) free(mem_block);
    return false;
}


bool uti_read_entire_dir(const char *parent_dir, char*** children, size_t *children_count) {
    DIR* d;
	d = opendir(parent_dir);
	if (!d) {
        printf("ERROR: Could not open directory %s: %s\n", parent_dir, strerror(errno));
        return false;
    }

    #define CHILDREN_DATA_SIZE_INITIAL 4096*4096*8
    #define CHILDREN_COUNT_INITIAL 8
    char* data = malloc( sizeof(char) * CHILDREN_DATA_SIZE_INITIAL);
    char** childs = malloc( sizeof(char *) * CHILDREN_COUNT_INITIAL);
    childs[0] = data;

    size_t data_capacity = CHILDREN_DATA_SIZE_INITIAL;
    size_t childs_capacity = CHILDREN_COUNT_INITIAL;
    size_t child_index = 0;

    struct dirent* entry = NULL;
	while ((entry = readdir(d)) != NULL) {
        // check for enough space
        size_t new_length = strlen(entry->d_name);
        size_t new_size = childs[child_index] - childs[0] + new_length + 1;
        if (new_size >= data_capacity) {
            printf("ERROR: space (%zu > %zu) exeeded inital capacity of %d Mbytes in %s:%d.\nIncrease CHILDREN_DATA_SIZE_INITIAL and recomplile\n", new_size, data_capacity, CHILDREN_DATA_SIZE_INITIAL / (1024 * 1024), __FILE__, __LINE__);
            exit(1);
        }

        char* p = mempcpy(childs[child_index], entry->d_name, new_length);
        *p = '\0';
        child_index++;

        // check for enough space and double if needed
        if (child_index >= childs_capacity) {
            childs_capacity *= 2;
            childs = realloc(childs, sizeof(char *) * childs_capacity);
            if (childs == NULL) {
                printf("ERROR: realloc failed in %s:%d\n", __FILE__, __LINE__);
                goto error;
            }
        }

        childs[child_index] = p + 1;
    }
    closedir(d);

    *children = childs;
    *children_count = child_index;

    return true;

error:
    if (d) closedir(d);
    if (data) free(data);
    if (childs) free(childs);
    return false;
}

// Adopted from nob.h
// TEMP buffer because we need copy strings to null terminated. Raylib MeaserTexteEx, etc.. uses only null terminated
static char temp_buffer_internal[TEMP_BUFFER_CAP_INTERNAL];
static size_t temp_size_internal = 0;
void uti_temp_reset(void){ temp_size_internal = 0; }
void *uti_temp_alloc(size_t requested_size) {
    size_t word_size = sizeof(uintptr_t);
    size_t size = (requested_size + word_size - 1)/word_size*word_size;
    if (temp_size_internal + size > TEMP_BUFFER_CAP_INTERNAL) return NULL;
    void *result = &temp_buffer_internal[temp_size_internal];
    temp_size_internal += size;
    return result;
}
char *uti_temp_strndup(const char *s, size_t n) {
    char *r = uti_temp_alloc(n + 1);
    assert(r != NULL && "Extend the size of the temporary allocator in uti.c");
    memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

// end temp allocator

// Adopted too

struct Uti_String_View uti_sv_chop_by_delim(struct Uti_String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->length && sv->text[i] != delim) {
        i += 1;
    }

    struct Uti_String_View result = uti_sv_from_parts(sv->text, i);

    if (i < sv->length) {
        sv->length -= i + 1;
        sv->text  += i + 1;
    } else {
        sv->length -= i;
        sv->text  += i;
    }

    return result;
}

struct Uti_String_View uti_sv_chop_left(struct Uti_String_View *sv, size_t n)
{
    if (n > sv->length) {
        n = sv->length;
    }

    struct Uti_String_View result = uti_sv_from_parts(sv->text, n);

    sv->text  += n;
    sv->length -= n;

    return result;
}

struct Uti_String_View uti_sv_from_parts(const char *text, size_t length)
{
    struct Uti_String_View sv;
    sv.length = length;
    sv.text = text;
    return sv;
}

struct Uti_String_View uti_sv_trim_left(struct Uti_String_View sv)
{
    size_t i = 0;
    while (i < sv.length && isspace(sv.text[i])) {
        i += 1;
    }

    return uti_sv_from_parts(sv.text + i, sv.length - i);
}

struct Uti_String_View uti_sv_trim_right(struct Uti_String_View sv)
{
    size_t i = 0;
    while (i < sv.length && isspace(sv.text[sv.length - 1 - i])) {
        i += 1;
    }

    return uti_sv_from_parts(sv.text, sv.length - i);
}

struct Uti_String_View uti_sv_trim(struct Uti_String_View sv)
{
    return uti_sv_trim_right(uti_sv_trim_left(sv));
}

struct Uti_String_View uti_sv_from_cstr(const char *cstr)
{
    return uti_sv_from_parts(cstr, strlen(cstr));
}

bool uti_sv_eq(struct Uti_String_View a, struct Uti_String_View b)
{
    if (a.length != b.length) {
        return false;
    } else {
        return memcmp(a.text, b.text, a.length) == 0;
    }
}

bool uti_sv_end_with(struct Uti_String_View sv, const char *cstr)
{
    size_t cstr_length = strlen(cstr);
    if (sv.length >= cstr_length) {
        size_t ending_start = sv.length - cstr_length;
        struct Uti_String_View sv_ending = uti_sv_from_parts(sv.text + ending_start, cstr_length);
        return uti_sv_eq(sv_ending, uti_sv_from_cstr(cstr));
    }
    return false;
}


bool uti_sv_starts_with(struct Uti_String_View sv, struct Uti_String_View expected_prefix)
{
    if (expected_prefix.length <= sv.length) {
        struct Uti_String_View actual_prefix = uti_sv_from_parts(sv.text, expected_prefix.length);
        return uti_sv_eq(expected_prefix, actual_prefix);
    }

    return false;
}

