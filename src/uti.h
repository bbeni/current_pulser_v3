// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
#ifndef UTI_H_
#define UTI_H_

#include "stdbool.h"
#include "stddef.h"

// allocate space for file content + \0 terminator and read into it. out size is without \0 terminator.
bool uti_read_entire_file(const char *path, char** content, size_t* out_size);
bool uti_read_entire_dir(const char *parent_dir, char*** children, size_t *children_count);

// Adopted from nob.h:
// TEMP buffer because we need copy strings to null terminated. Raylib MeaserTexteEx, etc.. uses only null terminated
#define TEMP_BUFFER_CAP_INTERNAL 4096*4096
void uti_temp_reset(void);
void *uti_temp_alloc(size_t requested_size);
char *uti_temp_strndup(const char *s, size_t n);
// end temp allocator stuff


struct Uti_String_View {
    const char* text;
    size_t length;
};

struct Uti_String_View uti_sv_chop_by_delim(struct Uti_String_View *sv, char delim);
struct Uti_String_View uti_sv_chop_left(struct Uti_String_View *sv, size_t n);
struct Uti_String_View uti_sv_from_parts(const char *text, size_t length);
struct Uti_String_View uti_sv_trim_left(struct Uti_String_View sv);
struct Uti_String_View uti_sv_trim_right(struct Uti_String_View sv);
struct Uti_String_View uti_sv_trim(struct Uti_String_View sv);
struct Uti_String_View uti_sv_from_cstr(const char *cstr);
bool uti_sv_eq(struct Uti_String_View a, struct Uti_String_View b);
bool uti_sv_end_with(struct Uti_String_View sv, const char *cstr);
bool uti_sv_starts_with(struct Uti_String_View sv, struct Uti_String_View expected_prefix);

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#endif //UTI_H_