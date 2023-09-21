// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2010 Serge Zaitsev
 */
#ifndef __JSMN_H_
#define __JSMN_H_

#define JSMN_STRICT
#define JSMN_PARENT_LINKS

#include <stddef.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef JSMN_STATIC
#define JSMN_API static
#else
#define JSMN_API extern
#endif

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
	JSMN_UNDEFINED = 0,
	JSMN_OBJECT = 1 << 0,
	JSMN_ARRAY = 1 << 1,
	JSMN_STRING = 1 << 2,
	JSMN_PRIMITIVE = 1 << 3
} jsmntype_t;

enum jsmnerr {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -ENOMEM,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -EINVAL,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -EMSGSIZE
};

/**
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 */
typedef struct jsmntok {
	jsmntype_t type;
	int start;
	int end;
	int size;
#ifdef JSMN_PARENT_LINKS
	int parent;
#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string.
 */
typedef struct jsmn_parser {
	unsigned int pos;     /* offset in the JSON string */
	unsigned int toknext; /* next token to allocate */
	int toksuper;         /* superior token node, e.g. parent object or array */
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
JSMN_API void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each
 * describing
 * a single JSON object.
 */
JSMN_API int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
			jsmntok_t *tokens, const unsigned int num_tokens);

/**
 * Like jsmn_parse, but allocates tokens dynamically.
 */
JSMN_API jsmntok_t *jsmn_parse_alloc(const char *js, const size_t len,
				     unsigned int *num_tokens);

static inline int jsmn_token_size(const jsmntok_t *token)
{
	return token->end - token->start;
}

/** Returns `true` if `token` is a string and equal to `str`. */
JSMN_API bool jsmn_str_eq(const char *str, const char *json, const jsmntok_t *token);

/** Returns `true` if `token` is to `str`. */
JSMN_API bool jsmn_eq(const char *val, const char *json, const jsmntok_t *token);

/** Returns the token after the value at `tokens[0]`. */
JSMN_API const jsmntok_t *jsmn_skip_value(const jsmntok_t *tokens);

/**
 * Returns a pointer to the value associated with `key` inside `json` starting
 * at `tokens[0]`, which is expected to be an object, or returns `NULL` if `key`
 * cannot be found.
 */
JSMN_API const jsmntok_t *jsmn_find_value(const char *key, const char *json,
					  const jsmntok_t *tokens);

/**
 * Locate the token at `path` inside `json` in a manner similar to JSONPath.
 *
 * Example:
 * \code
 * {
 *   "date": "...",
 *   "product": {
 *	 "serial": "1234",
 *   }
 * }
 * \endcode
 *
 * To locate the serial number in the JSON object above, you would use the
 * JSONPath expression "$.product.serial". The same thing can be accomplished
 * with this call:
 *
 * \code
 * const char *path[] = {"product", "serial", NULL};
 * const jsmntok_t *token = jsmn_locate(path, json, tokens);
 * \endcode
 *
 * \remark This function cannot search inside arrays.
 *
 * @param path		Null-terminated list of path elements.
 * @param json		JSON string to search in.
 * @param tokens	Tokens for `json`.
 *
 * @return Pointer to the value token or `NULL` if the token could not be found.
 */
JSMN_API const jsmntok_t *jsmn_locate(const char *path[], const char *json,
				      const jsmntok_t *tokens);

/**
 * Similar to `jsmn_locate` but returns a copy of the value or `NULL` if the
 * value does not exist or is not a string. The caller takes ownership of the
 * pointer returned.
 */
JSMN_API char *jsmn_strcpy(const char *path[], const char *json, const jsmntok_t *tokens);

#ifdef __cplusplus
}
#endif

#endif /* JSMN_H */
