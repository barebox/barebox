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

#ifdef __cplusplus
}
#endif

#endif /* JSMN_H */
