// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2026 Tobias Waldekranz <tobias@waldekranz.com>

#include <stdio.h>
#include <string.h>
#include <xfuncs.h>

#include <linux/kstrtox.h>

#include "lvm-md.h"

struct lvm_md_parser {
	const char *text;
	size_t len;
	size_t pos;

	lvm_tok_t *toks;
	int next;

	int super;	/* Index of the current container (section/array) */
	int key;	/* Index of the most recent key in the current section */
	bool value;	/* An '=' was seen; the next scalar/array is its value */
};

static int lvm_md_tok_new(struct lvm_md_parser *p, enum lvm_tok_type type, int start)
{
	lvm_tok_t *t;

	p->toks = xrealloc(p->toks, (p->next + 1) * sizeof(*p->toks));

	t = &p->toks[p->next];
	t->type = type;
	t->start = start;
	t->end = -1;
	t->size = 0;
	t->parent = -1;
	return p->next++;
}

static void lvm_md_link(struct lvm_md_parser *p, int idx)
{
	lvm_tok_t *t = &p->toks[idx];

	if (p->super >= 0 && p->toks[p->super].type == LVM_TOK_ARRAY) {
		t->parent = p->super;
		p->toks[p->super].size++;
	} else if (p->value) {
		t->parent = p->key;
		p->value = false;
		p->key = -1;
	} else {
		t->parent = p->super;
		if (p->super >= 0)
			p->toks[p->super].size++;
		p->key = idx;
	}
}

static int lvm_md_parse_string(struct lvm_md_parser *p)
{
	int idx, start;

	/* Skip opening quote */
	start = p->pos + 1;

	for (p->pos++; p->pos < p->len; p->pos++) {
		char c = p->text[p->pos];

		if (c == '\\' && p->pos + 1 < p->len) {
			p->pos++;
			continue;
		}
		if (c == '"') {
			idx = lvm_md_tok_new(p, LVM_TOK_STRING, start);
			p->toks[idx].end = p->pos;
			lvm_md_link(p, idx);
			return 0;
		}
	}

	/* Unterminated string */
	return -EINVAL;
}

static bool lvm_md_is_primchar(char c)
{
	switch (c) {
	case '0'...'9':
	case 'a'...'z':
	case 'A'...'Z':
	case '.':
	case '_':
	case '-':
	case '+':
		return true;
	}

	return false;
}

static int lvm_md_parse_primitive(struct lvm_md_parser *p)
{
	int start = p->pos;
	int idx;

	while (p->pos < p->len && lvm_md_is_primchar(p->text[p->pos]))
		p->pos++;

	idx = lvm_md_tok_new(p, LVM_TOK_PRIMITIVE, start);
	p->toks[idx].end = p->pos;
	lvm_md_link(p, idx);

	/* Reexamine the delimiter in the main loop */
	p->pos--;
	return 0;
}

static int lvm_md_parse(struct lvm_md_parser *p)
{
	int idx, s, par;
	int err;

	/* Implicit anonymous root section. */
	idx = lvm_md_tok_new(p, LVM_TOK_SECTION, 0);
	p->toks[idx].end = p->len;
	p->super = idx;
	p->key = -1;
	p->value = false;

	for (; p->pos < p->len; p->pos++) {
		char c = p->text[p->pos];

		switch (c) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case ',':
			break;
		case '#':
			while (p->pos < p->len && p->text[p->pos] != '\n')
				p->pos++;
			break;
		case '\0':
			/* A NUL terminates the document */
			p->pos = p->len;
			break;
		case '"':
			err = lvm_md_parse_string(p);
			if (err)
				return err;
			break;
		case '=':
			if (p->key < 0)
				return -EINVAL;
			p->value = true;
			break;
		case '{':
			/* The preceding key names this section. */
			if (p->key < 0)
				return -EINVAL;
			idx = lvm_md_tok_new(p, LVM_TOK_SECTION, p->pos);
			p->toks[idx].parent = p->key;
			p->super = idx;
			p->key = -1;
			p->value = false;
			break;
		case '[':
			idx = lvm_md_tok_new(p, LVM_TOK_ARRAY, p->pos);
			lvm_md_link(p, idx);
			p->super = idx;
			p->key = -1;
			p->value = false;
			break;
		case '}':
		case ']':
			s = p->super;
			if (s < 0 || p->value)
				return -EINVAL;
			if (p->toks[s].type !=
			    (c == '}' ? LVM_TOK_SECTION : LVM_TOK_ARRAY))
				return -EINVAL;
			p->toks[s].end = p->pos + 1;

			/* Pop back to the enclosing container. The token
			 * just closed hangs off either a key (the common
			 * case) or directly off an enclosing array.
			 */
			par = p->toks[s].parent;
			if (par >= 0 && p->toks[par].type == LVM_TOK_ARRAY)
				p->super = par;
			else
				p->super = (par >= 0) ? p->toks[par].parent : -1;
			p->key = -1;
			p->value = false;
			break;
		default:
			if (!lvm_md_is_primchar(c))
				return -EINVAL;

			err = lvm_md_parse_primitive(p);
			if (err)
				return err;
			break;
		}
	}

	if (p->super != 0 || p->value)
		return -EINVAL;	/* Unbalanced braces or dangling '=' */

	return p->next;
}

int lvm_md_parse_alloc(const char *text, size_t len, struct lvm_md **mdp)
{
	struct lvm_md_parser p = {
		.text = text,
		.len = len,
	};
	struct lvm_md *md;
	int ret;

	ret = lvm_md_parse(&p);
	if (ret < 0) {
		free(p.toks);
		return ret;
	}

	md = xzalloc(sizeof(*md));
	md->text = text;
	md->tokens = p.toks;
	md->num_tokens = p.next;

	*mdp = md;
	return 0;
}

void lvm_md_free(struct lvm_md *md)
{
	if (!md)
		return;

	free(md->tokens);
	free(md);
}

const lvm_tok_t *lvm_md_vgsect(const struct lvm_md *md, const lvm_tok_t **keyp)
{
	const lvm_tok_t *key, *val;

	lvm_md_for_each(md, key, &md->tokens[0]) {
		val = lvm_md_val(md, key);
		if (val && val->type == LVM_TOK_SECTION) {
			if (keyp)
				*keyp = key;
			return val;
		}
	}

	return NULL;
}

static bool lvm_md_tok_eq(const struct lvm_md *md, const lvm_tok_t *tok, const char *str)
{
	size_t len = tok->end - tok->start;

	return strlen(str) == len && !strncmp(md->text + tok->start, str, len);
}

static const lvm_tok_t *lvm_md_skip(const struct lvm_md *md, const lvm_tok_t *tok)
{
	const lvm_tok_t *end = md->tokens + md->num_tokens;
	int max = tok->end;

	do {
		tok++;
	} while (tok < end && tok->start < max);

	return (tok < end) ? tok : NULL;
}

const lvm_tok_t *lvm_md_first(const struct lvm_md *md, const lvm_tok_t *parent)
{
	if (!parent)
		return NULL;
	if (parent->type != LVM_TOK_SECTION && parent->type != LVM_TOK_ARRAY)
		return NULL;
	if (parent->size == 0)
		return NULL;
	if (parent + 1 >= md->tokens + md->num_tokens)
		return NULL;

	return parent + 1;
}

const lvm_tok_t *lvm_md_next(const struct lvm_md *md, const lvm_tok_t *parent,
			     const lvm_tok_t *child)
{
	const lvm_tok_t *value, *next;

	if (!parent || !child)
		return NULL;

	/* In a section a key is followed by its value subtree; in an
	 * array the element is itself the value.
	 */
	value = (parent->type == LVM_TOK_SECTION) ? child + 1 : child;
	if (value >= md->tokens + md->num_tokens)
		return NULL;

	next = lvm_md_skip(md, value);
	if (!next || next->start >= parent->end)
		return NULL;

	return next;
}

const lvm_tok_t *lvm_md_val(const struct lvm_md *md, const lvm_tok_t *key)
{
	if (!key || key + 1 >= md->tokens + md->num_tokens)
		return NULL;

	return key + 1;
}

const lvm_tok_t *lvm_md_find(const struct lvm_md *md, const lvm_tok_t *sec,
			     const char *key)
{
	const lvm_tok_t *k;

	if (!sec || sec->type != LVM_TOK_SECTION)
		return NULL;

	lvm_md_for_each(md, k, sec) {
		if (lvm_md_tok_eq(md, k, key))
			return lvm_md_val(md, k);
	}

	return NULL;
}

const lvm_tok_t *lvm_md_findf(const struct lvm_md *md, const lvm_tok_t *sec,
			      const char *keyfmt, ...)
{
	const lvm_tok_t *k;
	va_list ap;
	char *key;

	va_start(ap, keyfmt);
	key = xvasprintf(keyfmt, ap);
	va_end(ap);

	k = lvm_md_find(md, sec, key);
	free(key);
	return k;
}

static bool lvm_md_tok_is_str(const lvm_tok_t *tok)
{
	return tok && (tok->type == LVM_TOK_STRING ||
		       tok->type == LVM_TOK_PRIMITIVE);
}

static char *lvm_md_tok_strdup(const struct lvm_md *md, const lvm_tok_t *tok)
{
	int len;
	char *s;

	if (!lvm_md_tok_is_str(tok))
		return NULL;

	len = tok->end - tok->start;
	s = malloc(len + 1);
	if (!s)
		return NULL;

	memcpy(s, md->text + tok->start, len);
	s[len] = '\0';
	return s;
}

char *lvm_md_tok_xstrdup(const struct lvm_md *md, const lvm_tok_t *tok)
{
	char *cpy;

	if (!lvm_md_tok_is_str(tok))
		return NULL;

	cpy = lvm_md_tok_strdup(md, tok);
	if (!cpy)
		panic("lvm: out of memory");

	return cpy;
}

int lvm_md_tok_u64(const struct lvm_md *md, const lvm_tok_t *tok, u64 *out)
{
	char buf[32];
	int len;

	if (!lvm_md_tok_is_str(tok))
		return -EINVAL;

	len = tok->end - tok->start;
	if (len <= 0 || len >= (int)sizeof(buf))
		return -EINVAL;

	memcpy(buf, md->text + tok->start, len);
	buf[len] = '\0';
	return kstrtou64(buf, 0, out);
}

char *lvm_md_strdup(const struct lvm_md *md, const lvm_tok_t *sec,
		    const char *key)
{
	return lvm_md_tok_strdup(md, lvm_md_find(md, sec, key));
}

int lvm_md_u64(const struct lvm_md *md, const lvm_tok_t *sec,
	       const char *key, u64 *out)
{
	return lvm_md_tok_u64(md, lvm_md_find(md, sec, key), out);
}
