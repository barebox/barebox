/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2026 Tobias Waldekranz <tobias@waldekranz.com> */

#ifndef _LVM_MD_H
#define _LVM_MD_H

/*
 * LVM metadata parser, modeled on the jsmn JSON tokenizer.
 *
 * The text metadata is a sequence of statements. A statement is
 * either an assignment:
 *
 *	key = value
 *
 * where the value is a string ("..."), a primitive (a bare integer or
 * identifier) or an array ([v0, v1, ...]); or a named section:
 *
 *	key { ... }
 *
 * whose body is itself a sequence of statements. Sections nest. The
 * document as a whole is treated as the body of an implicit,
 * anonymous root section, which is always tokens[0].
 */
enum lvm_tok_type {
	LVM_TOK_UNDEFINED = 0,
	LVM_TOK_SECTION,
	LVM_TOK_ARRAY,
	LVM_TOK_STRING,
	LVM_TOK_PRIMITIVE,
};

typedef struct lvm_tok {
	enum lvm_tok_type type;
	int start;		/* Offset of first byte in text */
	int end;		/* Offset one past the last byte in text */
	int size;		/* # of child keys (section) or elements (array) */
	int parent;		/* Index of parent token, -1 for the root */
} lvm_tok_t;

struct lvm_md {
	const char *text;
	lvm_tok_t *tokens;
	size_t num_tokens;
};

/* Tokenize text into a dynamically allocated context. As tokens are
 * represented as spans, text must remain valid for the lifetime of
 * the returned context.
 */
int lvm_md_parse_alloc(const char *text, size_t len, struct lvm_md **md);
void lvm_md_free(struct lvm_md *md);

/* Return the only named section in the root, which describes the
 * VG. The key token, holding the VG name, is returned in keyp.
 */
const lvm_tok_t *lvm_md_vgsect(const struct lvm_md *md, const lvm_tok_t **keyp);

/* Return the value token belonging to key. */
const lvm_tok_t *lvm_md_val(const struct lvm_md *md, const lvm_tok_t *key);

/* Look up key in the section sec and return its value token, or
 * NULL.
 */
const lvm_tok_t *lvm_md_find(const struct lvm_md *md, const lvm_tok_t *sec,
			     const char *key);
const lvm_tok_t *lvm_md_findf(const struct lvm_md *md, const lvm_tok_t *sec,
			      const char *keyfmt, ...) __printf(3, 4);

/* The first child of a section (its first key) or array (its first
 * element), or NULL if empty.
 */
const lvm_tok_t *lvm_md_first(const struct lvm_md *md, const lvm_tok_t *parent);

/* The child following child within parent, or NULL once exhausted.
 * For a section, children are the keys; for an array, the elements.
 */
const lvm_tok_t *lvm_md_next(const struct lvm_md *md, const lvm_tok_t *parent,
			     const lvm_tok_t *child);

/* Iterate the keys of a section or the elements of an array. */
#define lvm_md_for_each(_md, _child, _parent)				\
	for ((_child) = lvm_md_first((_md), (_parent));			\
	     (_child);							\
	     (_child) = lvm_md_next((_md), (_parent), (_child)))

/* Retrurn a copy of the text of a string/primitive value token, or
 * NULL on error.
 */
char *lvm_md_tok_xstrdup(const struct lvm_md *md, const lvm_tok_t *tok);

/* Parse a primitive (or string) value token as an unsigned
 * integer.
 */
int lvm_md_tok_u64(const struct lvm_md *md, const lvm_tok_t *tok, u64 *out);

/* Return a copy of key's value from sec, if available, otherwize
 * NULL.
 */
char *lvm_md_strdup(const struct lvm_md *md, const lvm_tok_t *sec,
		    const char *key);

/* Return key's numerical value from sec in out, if available and
 * properly formatted. Returns 0 on success, negative error code on
 * error.
 */
int lvm_md_u64(const struct lvm_md *md, const lvm_tok_t *sec,
	       const char *key, u64 *out);

#endif	/* _LVM_MD_H */
