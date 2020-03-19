#pragma once

#include <string.h>

typedef struct str_dict str_dict;

/**
 * create a new string dict object
 */
str_dict *new_dict(size_t startsize);

/**
 * add a string to the dict object, associating a pointer to the string.
 * returns the "same" object, maybe moved/resized
 */
str_dict *add_str(str_dict *d, const char *str, size_t len, const void *val);

/**
 * packs and sorts the dict object
 * returns the "same" object, maybe moved/resized.
 * This function should be called before query_str_dict().
 * If new strings are added after, call pack_dict() again.
 */
str_dict *pack_dict(str_dict *d);

/**
 * query a dict object, returns the associated pointer or NULL if not found.
 */
const void *query_str_dict(const str_dict *d, const char *str, size_t len);
