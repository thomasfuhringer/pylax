/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2009 Leandro Pereira <leandro@hardinfo.org>
 * Copyright (C) 2015 James B
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <libintl.h>

//#define DEBUG g_print
#define DEBUG

#include "egg-markdown.h"

/*******************************************************************************
 *
 * This is a simple Markdown parser.
 * It can output to Pango, HTML or plain text. The following limitations are
 * already known, and properly deliberate:
 *
 * - No ordered list support
 * - No blockquote section support
 * - No backslash escapes support
 * - No HTML escaping support
 * - Auto-escapes certain word patterns, like http://
 *
 * It does support the rest of the standard pretty well, although it's not
 * been run against any conformance tests. The parsing is single pass, with
 * a simple enumerated intepretor mode and a single line back-memory.
 *
 ******************************************************************************/

static void     egg_markdown_finalize	(GObject		*object);

#define EGG_MARKDOWN_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), EGG_TYPE_MARKDOWN, EggMarkdownPrivate))

typedef gchar *(EggMarkdownLinkBuilder)(gchar *text, gchar *uri, gchar *title, gint link_id);
typedef gchar *(EggMarkdownImageBuilder)(gchar *alt_text, gchar *path, gint link_id);

typedef enum {
	EGG_MARKDOWN_MODE_BLANK,
	EGG_MARKDOWN_MODE_RULE,
	EGG_MARKDOWN_MODE_BULLETT,
	EGG_MARKDOWN_MODE_PARA,
	EGG_MARKDOWN_MODE_CODEBLOCK,
	EGG_MARKDOWN_MODE_H1,
	EGG_MARKDOWN_MODE_H2,
	EGG_MARKDOWN_MODE_H3,
	EGG_MARKDOWN_MODE_UNKNOWN
} EggMarkdownMode;

typedef enum {
	EGG_MARKDOWN_CODEBLOCK_NONE,
	EGG_MARKDOWN_CODEBLOCK_STD,
	EGG_MARKDOWN_CODEBLOCK_SHEBANG,
	EGG_MARKDOWN_CODEBLOCK_GITHUB
} EggMarkdownCodeblockMode;

typedef enum {
	EGG_MARKDOWN_CODEBLOCK_SYNTAX_NONE,
	EGG_MARKDOWN_CODEBLOCK_SYNTAX_GETTEXT
} EggMarkdownCodeblockSyntax;

typedef struct {
	const gchar *em_start;
	const gchar *em_end;
	const gchar *strong_start;
	const gchar *strong_end;
	const gchar *strong_em_start;
	const gchar *strong_em_end;
	const gchar *code_start;
	const gchar *code_end;
	const gchar *codeblock_start;
	const gchar *codeblock_end;
	const gchar *strikethrough_start;
	const gchar *strikethrough_end;
	const gchar *h1_start;
	const gchar *h1_end;
	const gchar *h2_start;
	const gchar *h2_end;
	const gchar *h3_start;
	const gchar *h3_end;
	const gchar *bullett_start;
	const gchar *bullett_end;
	const gchar *rule;
	const gchar *para_start;
	const gchar *para_end;

	EggMarkdownLinkBuilder *link_builder;
	EggMarkdownImageBuilder *image_builder;
} EggMarkdownTags;

struct EggMarkdownPrivate
{
	EggMarkdownExtensions    extensions;
	EggMarkdownMode		     mode;
	EggMarkdownCodeblockMode codeblock_mode;
	EggMarkdownCodeblockSyntax codeblock_syntax;
	EggMarkdownTags		     tags;
	EggMarkdownOutput	 output;
	gint			max_lines;
	guint			line_count;   // count of output lines
	gboolean		smart_quoting;
	gboolean		escape;
	gboolean		autocode;
	gboolean		autocode_shebang;  // autocode entire file that has shebang
	gboolean		exec;         // run executable code
	gboolean        global_exec;
	gboolean		use_gettext;  // translate content via gettext
	GString			*pending;
	GString			*processed;
	GArray			*link_table;

	// for pot_output
	gchar*          input_filename;
	guint           line_number;  // source line number
	FILE*			pot_output;   // where to send pot output
	gboolean        temporary_disable_pot;

	// code blocks mode save
	gboolean        codeblock_prev_exec;
	gboolean        codeblock_prev_pot;
};

G_DEFINE_TYPE (EggMarkdown, egg_markdown, G_TYPE_OBJECT)

/**
 * egg_markdown_to_text_line_is_rule:
 *
 * Horizontal rules are created by placing three or more hyphens, asterisks,
 * or underscores on a line by themselves.
 * You may use spaces between the hyphens or asterisks.
 **/
static gboolean
egg_markdown_to_text_line_is_rule (const gchar *line)
{
	guint i;
	guint len;
	guint count = 0;
	gchar *copy = NULL;
	gboolean ret = FALSE;

	len = strnlen (line, EGG_MARKDOWN_MAX_LINE_LENGTH);
	if (len == 0)
		goto out;

	/* replace non-rule chars with ~ */
	copy = g_strdup (line);
	g_strcanon (copy, "-*_ ", '~');
	for (i=0; i<len; i++) {
		if (copy[i] == '~')
			goto out;
		if (copy[i] != ' ')
			count++;
	}

	/* if we matched, return true */
	if (count >= 3)
		ret = TRUE;
out:
	g_free (copy);
	return ret;
}

/**
 * egg_markdown_to_text_line_is_bullett:
 **/
static gboolean
egg_markdown_to_text_line_is_bullett (const gchar *line)
{
	return (g_str_has_prefix (line, "- ") ||
		g_str_has_prefix (line, "* ") ||
		g_str_has_prefix (line, "+ ") ||
		g_str_has_prefix (line, " - ") ||
		g_str_has_prefix (line, " * ") ||
		g_str_has_prefix (line, " + "));
}

/**
 * egg_markdown_to_text_line_is_header1:
 **/
static gboolean
egg_markdown_to_text_line_is_header1 (const gchar *line)
{
	return g_str_has_prefix (line, "# ");
}

/**
 * egg_markdown_to_text_line_is_header2:
 **/
static gboolean
egg_markdown_to_text_line_is_header2 (const gchar *line)
{
	return g_str_has_prefix (line, "## ");
}

/**
 * egg_markdown_to_text_line_is_header3:
 **/
static gboolean
egg_markdown_to_text_line_is_header3 (const gchar *line)
{
	return g_str_has_prefix (line, "### ");
}


/**
 * egg_markdown_to_text_line_is_header1_type2:
 **/
static gboolean
egg_markdown_to_text_line_is_header1_type2 (const gchar *line)
{
	return g_str_has_prefix (line, "===");
}

/**
 * egg_markdown_to_text_line_is_header2_type2:
 **/
static gboolean
egg_markdown_to_text_line_is_header2_type2 (const gchar *line)
{
	return g_str_has_prefix (line, "---");
}

/**
 * egg_markdown_to_text_line_is_textdomain:
 **/
static gboolean
egg_markdown_to_text_line_is_textdomain (const gchar *line)
{
	return g_str_has_prefix (line, TEXTDOMAIN_DIRECTIVE);
}

/**
 * egg_markdown_to_text_line_is_nopot:
 **/
static gboolean
egg_markdown_to_text_line_is_nopot (const gchar *line)
{
	return g_str_has_prefix (line, NOPOT_DIRECTIVE);
}

/**
 * egg_markdown_to_text_line_is_exec:
 **/
static gboolean
egg_markdown_to_text_line_is_exec (const gchar *line)
{
	return g_str_has_prefix (line, EXEC_DIRECTIVE);
}

/**
 * egg_markdown_to_text_line_is_shebang:
 **/
static gboolean
egg_markdown_to_text_line_is_shebang (const gchar *line)
{
	return g_str_has_prefix (line, "#!");
}

/**
 * egg_markdown_to_text_line_is_code:
 **/
static gboolean
egg_markdown_to_text_line_is_code (const gchar *line)
{
	return (g_str_has_prefix (line, "    ") || g_str_has_prefix (line, "\t"));
}

static gboolean
egg_markdown_to_text_line_is_github_code (const gchar *line, EggMarkdownCodeblockSyntax *syntax)
{
	if( g_str_has_prefix (line, "```") ) {
		if( strncmp(line + 3, "gettext", 7)==0 )
			*syntax = EGG_MARKDOWN_CODEBLOCK_SYNTAX_GETTEXT;
		else
			*syntax = EGG_MARKDOWN_CODEBLOCK_SYNTAX_NONE;
		return TRUE;
	}
	return FALSE;
}

static gboolean
egg_markdown_extension_enabled(EggMarkdown *self, const EggMarkdownExtensions ext) {
	return (self->priv->extensions & ext);
}

#if 0
/**
 * egg_markdown_to_text_line_is_blockquote:
 **/
static gboolean
egg_markdown_to_text_line_is_blockquote (const gchar *line)
{
	return (g_str_has_prefix (line, "> "));
}
#endif

/**
 * egg_markdown_to_text_line_is_blank:
 **/
static gboolean
egg_markdown_to_text_line_is_blank (const gchar *line)
{
	guint i;
	guint len;
	gboolean ret = FALSE;

	len = strnlen (line, EGG_MARKDOWN_MAX_LINE_LENGTH);

	/* a line with no characters is blank by definition */
	if (len == 0) {
		ret = TRUE;
		goto out;
	}

	/* find if there are only space chars */
	for (i=0; i<len; i++) {
		if (line[i] != ' ' && line[i] != '\t')
			goto out;
	}

	/* if we matched, return true */
	ret = TRUE;
out:
	return ret;
}

/**
 * egg_markdown_replace:
 **/
static gchar *
egg_markdown_replace (const gchar *haystack, const gchar *needle, const gchar *replace)
{
	gchar *new;
	gchar **split;

	split = g_strsplit (haystack, needle, -1);
	new = g_strjoinv (replace, split);
	g_strfreev (split);

	return new;
}

/**
 * egg_markdown_strstr_spaces:
 **/
static gchar *
egg_markdown_strstr_spaces (const gchar *haystack, const gchar *needle)
{
	gchar *found;
	const gchar *haystack_new = haystack;

retry:
	/* don't find if surrounded by spaces or preceeded by backslash */
	found = strstr (haystack_new, needle);
	if (found == NULL)
		return NULL;

	/* start of the string, always valid */
	if (found == haystack)
		return found;

	/* end of the string, always valid */
	if (( *(found-1) == ' ' && *(found+1) == ' ') ||
	      *(found-1) == '\\' )
	{
		haystack_new = found+1;
		goto retry;
	}
	return found;
}


/**
 * egg_markdown_to_text_line_formatter:
 **/
static gchar *
egg_markdown_to_text_line_formatter (const gchar *line, const gchar *formatter, const gchar *left, const gchar *right)
{
	guint len;
	gchar *str1;
	gchar *str2;
	gchar *start = NULL;
	gchar *middle = NULL;
	gchar *end = NULL;
	gchar *copy = NULL;
	gchar *data = NULL;
	gchar *temp;

	/* needed to know for shifts */
	len = strnlen (formatter, EGG_MARKDOWN_MAX_LINE_LENGTH);
	if (len == 0)
		goto out;

	/* find sections */
	copy = g_strdup (line);
	str1 = egg_markdown_strstr_spaces (copy, formatter);
	if (str1 != NULL) {
		*str1 = '\0';
		str2 = egg_markdown_strstr_spaces (str1+len, formatter);
		if (str2 != NULL) {
			*str2 = '\0';
			middle = str1 + len;
			start = copy;
			end = str2 + len;
		}
	}

	/* if we found, replace and keep looking for the same string */
	if (start != NULL && middle != NULL && end != NULL) {
		temp = g_strdup_printf ("%s%s%s%s%s", start, left, middle, right, end);
		/* recursive */
		data = egg_markdown_to_text_line_formatter (temp, formatter, left, right);
		g_free (temp);
	} else {
		/* not found, keep return as-is */
		data = g_strdup (line);
	}
out:
	g_free (copy);
	return data;
}

static gchar *
egg_markdown_to_text_line_formatter_image (EggMarkdown *self, const gchar *line)
{
	const guint len = 2;	/* needed to know for shifts */
	gchar *str1;
	gchar *str2;
	gchar *start = NULL;
	gchar *path = NULL;
	gchar *alt_text = NULL;
	gchar *end = NULL;
	gchar *copy = NULL;
	gchar *data = NULL;
	gchar *temp;

	/* find sections */
	copy = g_strdup (line);
	str1 = egg_markdown_strstr_spaces (copy, "![");
	if (str1 != NULL) {
		*str1 = '\0';
		str2 = egg_markdown_strstr_spaces (str1+len, "]");
		if (str2 != NULL) {
			*str2 = '\0';
			start = copy;
			alt_text = str1 + len;

			str2 = strstr (str2 + 1, "(");
			if (str2 != NULL) {
				*str2 = '\0';

				str1 = strstr (str2 + 1, ")");
				if (str1 != NULL) {
					 *str1 = '\0';
					 path = str2 + 1;
					 end = str1 + 1;
				}
			}
		}
	}

	/* if we found, replace and keep looking for the same string */
	if (start && (path && *path) && alt_text && end) {
		gchar *formatted_img;
		gchar *path_copy = g_strdup(path);

		g_array_append_val(self->priv->link_table, path_copy);

		formatted_img = self->priv->tags.image_builder(alt_text,
					   path, self->priv->link_table->len - 1);

		temp = g_strdup_printf ("%s%s%s", start, formatted_img, end);
		g_free(formatted_img);
		data = egg_markdown_to_text_line_formatter_image (self, temp);
		g_free(temp);

	} else {
		/* not found, keep return as-is */
		data = g_strdup (line);
	}

	g_free (copy);
	return data;
}


/**
 * egg_markdown_to_text_line_formatter_link:
 **/
static gchar *
egg_markdown_to_text_line_formatter_link (EggMarkdown *self, const gchar *line)
{
	const guint len = 1;	/* needed to know for shifts */
	gchar *str1;
	gchar *str2;
	gchar *start = NULL;
	gchar *link = NULL;
	gchar *link_text = NULL;
	gchar *link_title = NULL;
	gchar *end = NULL;
	gchar *copy = NULL;
	gchar *data = NULL;
	gchar *temp;

	/* find sections */
	copy = g_strdup (line);
	str1 = egg_markdown_strstr_spaces (copy, "[");
	if (str1 != NULL) {
		*str1 = '\0';
		str2 = egg_markdown_strstr_spaces (str1+len, "]");
		if (str2 != NULL) {
			*str2 = '\0';
			start = copy;
			link_text = str1 + len;
			str1 = str2 + 1;
			str2 = strstr (str1, "(");
			if (str2 != NULL) {
				*str2 = '\0';
				if (str2 == str1) {
					str1 = strstr (str2 + 1, ")");
					if (str1 != NULL) {
						*str1 = '\0';
						link = str2 + 1;
						end = str1 + 1;

						str2 = strstr (link, " ");
						if (str2 != NULL) {
							*str2 = '\0';
							if (!self->priv->smart_quoting) {
								link_title = str2+2; // skip quote
								*(str1-1) = 0;       // delete quote
							} else {
								link_title = str2+1+sizeof("“")-1;
								*(str1-(sizeof("”")-1)) = 0;
							}
						}
					}
				} else {
					/* we found faux link "start[link_text]str1(", save literals and go past looking for a link */
					gchar *literal, *past;
					literal = g_strdup_printf ("%s[%s]", start, link_text);
					temp = g_strdup_printf("%s(%s", str1, str2 + 1);
					past = egg_markdown_to_text_line_formatter_link (self, temp);
					g_free(temp);
					data = g_strdup_printf("%s%s", literal, past);
					g_free(literal);
					g_free(past);
					g_free(copy);
					return data;
				}
			}
		}
	}

	/* if we found a link replace it and keep looking for another link */
	if (start && (link && *link) && link_text && end) {
		gchar *formatted_link;
		gchar *link_copy = g_strdup(link);

		g_array_append_val(self->priv->link_table, link_copy);

		formatted_link = self->priv->tags.link_builder(link_text,
						link, link_title, self->priv->link_table->len - 1);

		temp = g_strdup_printf ("%s%s%s", start, formatted_link, end);
		g_free(formatted_link);
		data = egg_markdown_to_text_line_formatter_link (self, temp);
		g_free(temp);

	} else {
		/* not found, keep return as-is */
		data = g_strdup (line);
	}

	g_free (copy);
	return data;
}

/**
 * egg_markdown_to_text_line_formatter_exec:
 **/
static gchar *
egg_markdown_to_text_line_formatter_exec (EggMarkdown *self, const gchar *line)
{
	gchar *str1;
	gchar *str2;
	gchar *start = NULL;
	gchar *cmd = NULL;
	gchar *end = NULL;
	gchar *copy = NULL;
	gchar *data = NULL;

	// a silly state-machine to "parse" quoting
	#define NQ  0
	#define SQ  1
	#define DQ  2
	int next[3][3] = { // state, input-token
		{NQ, SQ, DQ},  // no-quote with input of no-quote, single, double-quotes
		{SQ, NQ, SQ},  // single-quote with input of no-quote, single, double-quotes
		{DQ, DQ, NQ}   // double-quote with input of no-quote, single, double-quotes
	};
	int quote_allowed[3] = { // state
		1, 0, 1  // quote only allowed during no-quote and double-quotes
	};
	int state; // as #defined above: 0=no quote, 1=in-single-quote, 2=in-double-quote
	int next_token_is_never_quote;
	int tok;

	/* find sections */
	copy = g_strdup (line);
	str1 = egg_markdown_strstr_spaces (copy, "$(");
	if (str1 != NULL) {
		*str1 = '\0';
		state = next_token_is_never_quote = 0;
		for (str2=str1+2; *str2; str2++) {
			tok = NQ;
			if (!next_token_is_never_quote) {
				switch (*str2) {
					case ')' : if (state==NQ) goto sm_out; else break;
					case '\'' : tok=SQ; break;
					case '\"' : tok=DQ; break;
					case '\\' : next_token_is_never_quote = quote_allowed[state];
					break;
				}
			} else next_token_is_never_quote=0; //reset for next
			state = next[state][tok];
		}
sm_out:
		if (state==NQ) {
			*str2 = '\0';
			start = copy;
			cmd = str1 + 2;
			end = str2 + 1;
		}
	}

	/* if we found, replace and keep looking for the same string */
	if (start && (cmd && *cmd) && end) {
		GString *temp = g_string_sized_new(EGG_MARKDOWN_MAX_LINE_LENGTH);
		g_string_assign (temp, start);

		FILE *p = popen(cmd,"r");
		char *line = g_malloc0 (EGG_MARKDOWN_MAX_LINE_LENGTH);
		int linelen;
		while (fgets(line, EGG_MARKDOWN_MAX_LINE_LENGTH, p)) {
			linelen = strlen(line);
			if (linelen > 0) {
				if (line[linelen-1]=='\n') line[linelen-1]=0;
				g_string_append (temp, line);
			}
		}
		pclose(p);
		g_free (line);

		g_string_append (temp, end);
		data = egg_markdown_to_text_line_formatter_exec (self, temp->str);
		g_string_free (temp, TRUE);
	} else {
		/* not found, keep return as-is */
		data = g_strdup (line);
	}

	g_free (copy);
	return data;
}

static gchar *
egg_markdown_escape_c_str(const gchar *line)
{
	gchar *out;
	gchar c;
	GString *s = g_string_sized_new (strlen(line)*2);

	for (; (c = *line); line++) {
		switch(c) {
			case '"':
			case '\\':
				g_string_append_c (s,'\\');
			default:
				g_string_append_c (s,c);
		}
	}

	out = s->str;
	g_string_free (s, FALSE);
	return out;
}

/**
 * egg_markdown_to_text_line_formatter_gettext:
 **/
static gchar *
egg_markdown_to_text_line_formatter_gettext (EggMarkdown *self, const gchar *line)
{
	if (self->priv->pot_output)
	{
		if (!self->priv->temporary_disable_pot) {
			gchar *escaped = egg_markdown_escape_c_str(line);
			fprintf(self->priv->pot_output,
			"#: %s:%u\n"
			"msgid \"%s\"\nmsgstr \"\"\n\n",
			self->priv->input_filename, self->priv->line_number, escaped);
			g_free (escaped);
			return g_strdup(line);
		} else {
			return g_strdup("");
		}
	} else if (self->priv->use_gettext)
	{
		if (!self->priv->temporary_disable_pot) {
			return g_strdup(gettext(line));
		} else {
			return g_strdup(line);
		}
	}
}

/**
 * egg_markdown_to_text_line_formatter_remove_backslash:
 **/
static gchar *
egg_markdown_to_text_line_formatter_remove_backslash (gchar *line)
{
	gchar *out;
	gchar c;
	gboolean quoted;
	GString *s = g_string_sized_new (strlen(line)*2);

	for (quoted=FALSE; (c = *line); line++) {
		if (quoted) {
			switch(c) {
				case '\\': case '`':
				case '*': case '_' :
				case '{': case '}':
				case '[': case ']':
				case '(': case ')':
				case '#':
				case '+': case '-':
				case '.': case '!':
					// remove backslash
					break;
				default:
					// all other - insert the backslash again
					g_string_append_c (s,'\\');
			}
			g_string_append_c (s,c);
			quoted = FALSE;
		} else {
			if (c=='\\') quoted=TRUE;
			else
				g_string_append_c (s,c);
		}
	}

	out = s->str;
	g_string_free (s, FALSE);
	return out;
}

void
egg_markdown_clear(EggMarkdown *self)
{
	int i;

	for (i = 0; i < self->priv->link_table->len; i++) {
		g_free(g_array_index(self->priv->link_table, gchar *, i));
	}

	g_array_free(self->priv->link_table, TRUE);
	self->priv->link_table = g_array_new(FALSE, FALSE, sizeof(gchar *));
}

gchar *
egg_markdown_get_link_uri(EggMarkdown *self, const gint link_id)
{
	g_return_val_if_fail(link_id < self->priv->link_table->len, NULL);
	return g_strdup(g_array_index(self->priv->link_table, gchar *, link_id));
}

/**
 * egg_markdown_to_text_line_format_sections:
 **/
static gchar *
egg_markdown_to_text_line_format_sections (EggMarkdown *self, const gchar *line)
{
	gchar *data = g_strdup (line);
	gchar *temp;

	/* smart quoting */
	if (self->priv->smart_quoting) {
		if (self->priv->escape) {
			temp = data;
			data = egg_markdown_to_text_line_formatter (temp, "&quot;", "“", "”");
			g_free (temp);

			temp = data;
			data = egg_markdown_to_text_line_formatter (temp, "&apos;", "‘", "’");
			g_free (temp);
		} else {
			temp = data;
			data = egg_markdown_to_text_line_formatter (temp, "\"", "“", "”");
			g_free (temp);

			temp = data;
			data = egg_markdown_to_text_line_formatter (temp, "'", "‘", "’");
			g_free (temp);
		}
	}

	/* image */
	temp = data;
	data = egg_markdown_to_text_line_formatter_image (self, temp);
	g_free(temp);

	/* link */
	temp = data;
	data = egg_markdown_to_text_line_formatter_link (self, temp);
	g_free(temp);

	/* bold-italic1 - this combo must be detected before the individual bold/italic */
	temp = data;
	data = egg_markdown_to_text_line_formatter (temp, "***", self->priv->tags.strong_em_start, self->priv->tags.strong_em_end);
	g_free (temp);

	/* bold-italic2 - this combo must be detected before the individual bold/italic */
	temp = data;
	data = egg_markdown_to_text_line_formatter (temp, "___", self->priv->tags.strong_em_start, self->priv->tags.strong_em_end);
	g_free (temp);

	/* bold1 */
	temp = data;
	data = egg_markdown_to_text_line_formatter (temp, "**", self->priv->tags.strong_start, self->priv->tags.strong_end);
	g_free (temp);

	/* bold2 */
	temp = data;
	data = egg_markdown_to_text_line_formatter (temp, "__", self->priv->tags.strong_start, self->priv->tags.strong_end);
	g_free (temp);

	/* italic1 */
	temp = data;
	data = egg_markdown_to_text_line_formatter (temp, "*", self->priv->tags.em_start, self->priv->tags.em_end);
	g_free (temp);

	/* italic2 */
	temp = data;
	data = egg_markdown_to_text_line_formatter (temp, "_", self->priv->tags.em_start, self->priv->tags.em_end);
	g_free (temp);

	/* strikethrough */
	temp = data;
	data = egg_markdown_to_text_line_formatter (temp, "---", self->priv->tags.strikethrough_start, self->priv->tags.strikethrough_end);
	g_free (temp);

	/* em-dash */
	temp = data;
	data = egg_markdown_replace (temp, " -- ", " — ");
	g_free (temp);

	/* remove redundant backslash */
	temp = data;
	data = egg_markdown_to_text_line_formatter_remove_backslash (temp);
	g_free (temp);

	return data;
}

/**
 * egg_markdown_to_text_line_format:
 **/
static gchar *
egg_markdown_to_text_line_format (EggMarkdown *self, const gchar *line)
{
	guint i;
	gchar *text;
	gboolean mode = FALSE;
	gchar **codes;
	GString *string;

	/* optimise the trivial case where we don't have any code tags */
	text = strstr (line, "`");
	if (text == NULL) {
		text = egg_markdown_to_text_line_format_sections (self, line);
		goto out;
	}

	/* we want to parse the code sections without formatting */
	codes = g_strsplit (line, "`", -1);
	string = g_string_new ("");
	for (i=0; codes[i] != NULL; i++) {
		if (!mode) {
			text = egg_markdown_to_text_line_format_sections (self, codes[i]);
			g_string_append (string, text);
			g_free (text);
			mode = TRUE;
		} else {
			/* just append without formatting */
			g_string_append (string, self->priv->tags.code_start);
			g_string_append (string, codes[i]);
			g_string_append (string, self->priv->tags.code_end);
			mode = FALSE;
		}
	}
	text = g_string_free (string, FALSE);
out:
	return text;
}

/**
 * egg_markdown_add_pending:
 **/
static gboolean
egg_markdown_add_pending (EggMarkdown *self, const gchar *line)
{
	gchar *copy;

	/* would put us over the limit */
	if (self->priv->line_count >= self->priv->max_lines)
		return FALSE;

	copy = g_strdup (line);

	/* strip leading and trailing spaces unless in codeblock mode */
	if (self->priv->mode != EGG_MARKDOWN_MODE_CODEBLOCK)
		g_strstrip (copy);

	/* append */
	g_string_append_printf (self->priv->pending, "%s ", copy);

	g_free (copy);
	return TRUE;
}

/**
 * egg_markdown_add_pending_header:
 **/
static gboolean
egg_markdown_add_pending_header (EggMarkdown *self, const gchar *line)
{
	gchar *copy;
	gboolean ret;

	/* strip trailing # */
	copy = g_strdup (line);
	g_strdelimit (copy, "#", ' ');
	ret = egg_markdown_add_pending (self, copy);
	g_free (copy);
	return ret;
}

/**
 * egg_markdown_count_chars_in_word:
 **/
static guint
egg_markdown_count_chars_in_word (const gchar *text, gchar find)
{
	guint i;
	guint len;
	guint count = 0;

	/* get length */
	len = strnlen (text, EGG_MARKDOWN_MAX_LINE_LENGTH);
	if (len == 0)
		goto out;

	/* find matching chars */
	for (i=0; i<len; i++) {
		if (text[i] == find)
			count++;
	}
out:
	return count;
}

/**
 * egg_markdown_word_is_code:
 **/
static gboolean
egg_markdown_word_is_code (const gchar *text)
{
	/* already code */
	if (g_str_has_prefix (text, "`"))
		return FALSE;
	if (g_str_has_suffix (text, "`"))
		return FALSE;

	/* paths */
	if (g_str_has_prefix (text, "/"))
		return TRUE;

	/* bugzillas */
	if (g_str_has_prefix (text, "#"))
		return TRUE;

	/* uri's */
	if (g_str_has_prefix (text, "http://"))
		return TRUE;
	if (g_str_has_prefix (text, "https://"))
		return TRUE;
	if (g_str_has_prefix (text, "ftp://"))
		return TRUE;

	/* patch files */
	if (g_strrstr (text, ".patch") != NULL)
		return TRUE;
	if (g_strrstr (text, ".diff") != NULL)
		return TRUE;

	/* function names */
	if (g_strrstr (text, "()") != NULL)
		return TRUE;

	/* email addresses */
	if (g_strrstr (text, "@") != NULL)
		return TRUE;

	/* compiler defines */
	if (text[0] != '_' && text[0] != '*' &&
	    egg_markdown_count_chars_in_word (text, '_') > 1)
		return TRUE;

	/* nothing special */
	return FALSE;
}

/**
 * egg_markdown_word_auto_format_code:
 **/
static gchar *
egg_markdown_word_auto_format_code (const gchar *text)
{
	guint i;
	gchar *temp;
	gchar **words;
	gboolean ret = FALSE;

	/* split sentence up with space */
	words = g_strsplit (text, " ", -1);

	/* search each word */
	for (i=0; words[i] != NULL; i++) {
		if (egg_markdown_word_is_code (words[i])) {
			temp = g_strdup_printf ("`%s`", words[i]);
			g_free (words[i]);
			words[i] = temp;
			ret = TRUE;
		}
	}

	/* no replacements, so just return a copy */
	if (!ret) {
		temp = g_strdup (text);
		goto out;
	}

	/* join the array back into a string */
	temp = g_strjoinv (" ", words);
out:
	g_strfreev (words);
	return temp;
}

/**
 * egg_markdown_flush_pending:
 **/
static void
egg_markdown_flush_pending (EggMarkdown *self)
{
	gchar *copy;
	gchar *temp;

	/* no data yet */
	if (self->priv->mode == EGG_MARKDOWN_MODE_UNKNOWN)
		return;

	/* remove trailing spaces */
	while (g_str_has_suffix (self->priv->pending->str, " "))
		g_string_set_size (self->priv->pending, self->priv->pending->len - 1);

	/* check for translation first */
	copy = g_strdup (self->priv->pending->str);
	if ((self->priv->pot_output || self->priv->use_gettext) && copy[0]) {
		temp = copy;
		copy = egg_markdown_to_text_line_formatter_gettext (self, temp);
		g_free(temp);
	}

	/* then check for exec, before anything, and so its output can be interpreted by others */
	if (self->priv->exec) {
		temp = copy;
		copy = egg_markdown_to_text_line_formatter_exec (self, temp);
		g_free(temp);
	}

	/* pango requires escaping */
	if (!self->priv->escape && self->priv->output == EGG_MARKDOWN_OUTPUT_PANGO) {
		g_strdelimit (copy, "<", '(');
		g_strdelimit (copy, ">", ')');
	}

	/* check words for code */
	if (self->priv->autocode &&
	    (self->priv->mode == EGG_MARKDOWN_MODE_PARA ||
	     self->priv->mode == EGG_MARKDOWN_MODE_BULLETT)) {
		temp = egg_markdown_word_auto_format_code (copy);
		g_free (copy);
		copy = temp;
	}

	/* escape */
	if (self->priv->escape) {
		temp = g_markup_escape_text (copy, -1);
		g_free (copy);
		copy = temp;
	}

	/* do formatting */
	if (self->priv->mode != EGG_MARKDOWN_MODE_CODEBLOCK)
		temp = egg_markdown_to_text_line_format (self, copy);
	else temp = copy;

	if (self->priv->mode == EGG_MARKDOWN_MODE_BULLETT) {
		g_string_append_printf (self->priv->processed, "%s%s%s\n", self->priv->tags.bullett_start, temp, self->priv->tags.bullett_end);
		self->priv->line_count++;
	} else if (self->priv->mode == EGG_MARKDOWN_MODE_H1) {
		g_string_append_printf (self->priv->processed, "%s%s%s\n", self->priv->tags.h1_start, temp, self->priv->tags.h1_end);
	} else if (self->priv->mode == EGG_MARKDOWN_MODE_H2) {
		g_string_append_printf (self->priv->processed, "%s%s%s\n", self->priv->tags.h2_start, temp, self->priv->tags.h2_end);
	} else if (self->priv->mode == EGG_MARKDOWN_MODE_H3) {
		g_string_append_printf (self->priv->processed, "%s%s%s\n", self->priv->tags.h3_start, temp, self->priv->tags.h3_end);
	} else if (self->priv->mode == EGG_MARKDOWN_MODE_CODEBLOCK) {
		g_string_append_printf (self->priv->processed, "%s%s%s\n", self->priv->tags.codeblock_start, temp, self->priv->tags.codeblock_end);
		self->priv->line_count++;
	} else if (self->priv->mode == EGG_MARKDOWN_MODE_PARA) {
		g_string_append_printf (self->priv->processed, "%s%s%s\n", self->priv->tags.para_start, temp, self->priv->tags.para_end);
		self->priv->line_count++;
	} else if (self->priv->mode == EGG_MARKDOWN_MODE_RULE) {
		g_string_append_printf (self->priv->processed, "%s\n", temp);
		self->priv->line_count++;
	}

	DEBUG ("adding '%s'", temp);

	/* clear */
	g_string_truncate (self->priv->pending, 0);
	g_free (copy);
	if (temp!=copy) g_free (temp);
}

/**
 * egg_markdown_to_text_line_process:
 **/
static gboolean
egg_markdown_to_text_line_process (EggMarkdown *self, const gchar *line)
{
	gboolean ret;

	/* extension: shebang - do it here so we can trap it before others */
	if (egg_markdown_extension_enabled (self, EGG_MARKDOWN_EXTENSION_SHEBANG)) {
		if (self->priv->autocode_shebang || // reset on (re-)parsing rendered file
			( (self->priv->line_number == 0) && // only look for shebang on first line
			  egg_markdown_to_text_line_is_shebang (line) )
			) {
			ret = self->priv->autocode_shebang = TRUE;
			self->priv->mode = EGG_MARKDOWN_MODE_CODEBLOCK;
			self->priv->codeblock_mode = EGG_MARKDOWN_CODEBLOCK_SHEBANG;

			/* render codeblock */
			egg_markdown_add_pending (self, line);
			egg_markdown_flush_pending (self); // end the line
			goto out;
		}
	}

	/* extension: github codeblock */
	if (egg_markdown_extension_enabled (self, EGG_MARKDOWN_EXTENSION_GITHUB)) {
		if (ret = egg_markdown_to_text_line_is_github_code (line, &self->priv->codeblock_syntax) ) {
			if (self->priv->codeblock_mode != EGG_MARKDOWN_CODEBLOCK_GITHUB) {
				// begin code block
				egg_markdown_flush_pending (self);
				self->priv->mode = EGG_MARKDOWN_MODE_CODEBLOCK;
				self->priv->codeblock_mode = EGG_MARKDOWN_CODEBLOCK_GITHUB;
				self->priv->codeblock_prev_exec = self->priv->exec;
				self->priv->exec = FALSE;
				if (self->priv->codeblock_syntax != EGG_MARKDOWN_CODEBLOCK_SYNTAX_GETTEXT) {
					self->priv->codeblock_prev_pot = self->priv->temporary_disable_pot;
					self->priv->temporary_disable_pot = TRUE;
				}
				goto out;
			} else {
				// end code block
				self->priv->mode = EGG_MARKDOWN_MODE_UNKNOWN;
				self->priv->codeblock_mode = EGG_MARKDOWN_CODEBLOCK_NONE;
				self->priv->exec = self->priv->codeblock_prev_exec;
				if (self->priv->codeblock_syntax != EGG_MARKDOWN_CODEBLOCK_SYNTAX_GETTEXT)
					self->priv->temporary_disable_pot = self->priv->codeblock_prev_pot;
				self->priv->codeblock_syntax = EGG_MARKDOWN_CODEBLOCK_SYNTAX_NONE;
				goto out;
			}
		}
		if (self->priv->mode == EGG_MARKDOWN_MODE_CODEBLOCK &&
			self->priv->codeblock_mode == EGG_MARKDOWN_CODEBLOCK_GITHUB)
		{
			ret = TRUE;
			egg_markdown_add_pending (self, line);
			egg_markdown_flush_pending (self); // end the line
			goto out;
		}
	}

	/* code - do it here so we can trap it before others */
	ret = egg_markdown_to_text_line_is_code (line);
	if (ret) {
		const gchar *start;
		if (line[0]=='\t') start=&line[1];
		else start=&line[4];

		if (self->priv->mode != EGG_MARKDOWN_MODE_CODEBLOCK) {
			// begin code block
			egg_markdown_flush_pending (self);
			self->priv->mode = EGG_MARKDOWN_MODE_CODEBLOCK;
			self->priv->codeblock_mode = EGG_MARKDOWN_CODEBLOCK_STD;
			self->priv->codeblock_prev_exec = self->priv->exec;
			self->priv->codeblock_prev_pot = self->priv->temporary_disable_pot;
			self->priv->exec = FALSE;
			self->priv->temporary_disable_pot = TRUE;
		}

		// continue code block
		egg_markdown_add_pending (self, start);
		egg_markdown_flush_pending (self); // end the line
		goto out;

	} else {
		if (self->priv->mode == EGG_MARKDOWN_MODE_CODEBLOCK) {
			// end code block
			self->priv->mode = EGG_MARKDOWN_MODE_UNKNOWN;
			self->priv->codeblock_mode = EGG_MARKDOWN_CODEBLOCK_NONE;
			self->priv->exec = self->priv->codeblock_prev_exec;
			self->priv->temporary_disable_pot = self->priv->codeblock_prev_pot;
		}
	}

	/* blank */
	ret = egg_markdown_to_text_line_is_blank (line);
	if (ret) {
		DEBUG ("blank: '%s'", line);
		egg_markdown_flush_pending (self);
		/* a new line after a list is the end of list, not a gap */
		if (self->priv->mode != EGG_MARKDOWN_MODE_BULLETT)
			ret = egg_markdown_add_pending (self, "\n");
		self->priv->mode = EGG_MARKDOWN_MODE_BLANK;
		goto out;
	}

	/* header1_type2 */
	ret = egg_markdown_to_text_line_is_header1_type2 (line);
	if (ret && self->priv->mode == EGG_MARKDOWN_MODE_PARA) {
		DEBUG ("header1_type2: '%s'", line);
		self->priv->mode = EGG_MARKDOWN_MODE_H1;
		goto out;
	}

	/* header2_type2 */
	ret = egg_markdown_to_text_line_is_header2_type2 (line);
	if (ret && self->priv->mode == EGG_MARKDOWN_MODE_PARA) {
		DEBUG ("header2_type2: '%s'", line);
		self->priv->mode = EGG_MARKDOWN_MODE_H2;
		goto out;
	}

	/* rule */
	ret = egg_markdown_to_text_line_is_rule (line);
	if (ret) {
		DEBUG ("rule: '%s'", line);
		egg_markdown_flush_pending (self);
		self->priv->mode = EGG_MARKDOWN_MODE_RULE;
		ret = egg_markdown_add_pending (self, self->priv->tags.rule);
		goto out;
	}

	/* bullett */
	ret = egg_markdown_to_text_line_is_bullett (line);
	if (ret) {
		DEBUG ("bullett: '%s'", line);
		egg_markdown_flush_pending (self);
		self->priv->mode = EGG_MARKDOWN_MODE_BULLETT;
		ret = egg_markdown_add_pending (self, &line[2]);
		goto out;
	}

	/* header1 */
	ret = egg_markdown_to_text_line_is_header1 (line);
	if (ret) {
		DEBUG ("header1: '%s'", line);
		egg_markdown_flush_pending (self);
		self->priv->mode = EGG_MARKDOWN_MODE_H1;
		ret = egg_markdown_add_pending_header (self, &line[2]);
		goto out;
	}

	/* header2 */
	ret = egg_markdown_to_text_line_is_header2 (line);
	if (ret) {
		DEBUG ("header2: '%s'", line);
		egg_markdown_flush_pending (self);
		self->priv->mode = EGG_MARKDOWN_MODE_H2;
		ret = egg_markdown_add_pending_header (self, &line[3]);
		goto out;
	}

	/* header3 */
	ret = egg_markdown_to_text_line_is_header3 (line);
	if (ret) {
		g_debug ("header3: '%s'", line);
		egg_markdown_flush_pending (self);
		self->priv->mode = EGG_MARKDOWN_MODE_H3;
		ret = egg_markdown_add_pending_header (self, &line[4]);
		goto out;
	}

	/* directives */
	ret = egg_markdown_to_text_line_is_textdomain (line);
	if (ret) {
		egg_markdown_flush_pending (self); // end line
		gchar *domain = g_strdup(&line[sizeof(TEXTDOMAIN_DIRECTIVE)-1]);
		g_strstrip (domain);
		if (domain[0]) {
			g_debug ("text_domain: '%s'\n", domain);
			gchar *dir = bindtextdomain(textdomain(NULL),NULL);
			textdomain(domain);
			bindtextdomain(textdomain(NULL), dir);
		}
		g_free (domain);
		goto out;
	}

	ret = egg_markdown_to_text_line_is_exec (line);
	if (ret) {
		egg_markdown_flush_pending (self); // end line
		gchar *tmp = g_strdup(&line[sizeof(EXEC_DIRECTIVE)-1]);
		g_strstrip (tmp);
		if (tmp[0]) {
			g_debug ("exec: '%s'\n", tmp);
			if (g_str_has_prefix(tmp, "yes") && self->priv->global_exec)
				self->priv->exec = TRUE;
			else
				self->priv->exec = FALSE;
		}
		g_free (tmp);
		goto out;
	}

	ret = egg_markdown_to_text_line_is_nopot (line);
	if (ret) {
		egg_markdown_flush_pending (self); // end line
		gchar *tmp = g_strdup(&line[sizeof(NOPOT_DIRECTIVE)-1]);
		g_strstrip (tmp);
		if (tmp[0]) {
			g_debug ("nopot: '%s'\n", tmp);
			if (g_str_has_prefix(tmp, "yes"))
				self->priv->temporary_disable_pot = TRUE;
			else
				self->priv->temporary_disable_pot = FALSE;
		}
		g_free (tmp);
		goto out;
	}


	/* paragraph */
	if (self->priv->mode == EGG_MARKDOWN_MODE_BLANK || self->priv->mode == EGG_MARKDOWN_MODE_UNKNOWN) {
		egg_markdown_flush_pending (self);
		self->priv->mode = EGG_MARKDOWN_MODE_PARA;
	}

	/* add to pending */
	DEBUG ("continue: '%s'", line);
	ret = egg_markdown_add_pending (self, line);
out:
	/* if we failed to add, we don't know the mode */
	if (!ret)
		self->priv->mode = EGG_MARKDOWN_MODE_UNKNOWN;
	return ret;
}

/**
 * egg_markdown_linkbuilder_pango:
 **/
static gchar *
egg_markdown_linkbuilder_pango (gchar *text, gchar *uri, gchar *title, gint link_id)
{
	/* FIXME: This is a nasty hack, since extending Pango markup to allow new tags
	*        is too complicated. We use the language code as a link index
	*        since it won't allow anything besides letters or numbers.
	*        To obtain the link URI, use egg_markdown_get_link_uri(). */
	return g_strdup_printf("<span lang=\"%d\" foreground=\"blue\"><u>%s</u></span>",
		  link_id, text);
}

/**
 * egg_markdown_linkbuilder_html
 **/
static gchar *
egg_markdown_linkbuilder_html (gchar *text, gchar *uri, gchar *title, gint link_id)
{
	if (title)
		return g_strdup_printf("<a href=\"%s\" title=\"%s\">%s</a>", uri, title, text);
	else
		return g_strdup_printf("<a href=\"%s\">%s</a>", uri, text);
}

/**
 * egg_markdown_linkbuilder_text
 **/
static gchar *
egg_markdown_linkbuilder_text (gchar *text, gchar *uri, gchar *title, gint link_id)
{
	return g_strdup_printf("%s (%s)", text, uri);
}

/**
 * egg_markdown_imagebuilder_pango:
 **/
static gchar *
egg_markdown_imagebuilder_pango (gchar *alt_text, gchar *uri, gint link_id)
{
	/* FIXME See egg_markdown_linkbuilder_pango() */
	return g_strdup_printf("<span lang=\"%d\" underline=\"double\">%s</span>",
						  link_id, alt_text);
}

/**
 * egg_markdown_imagebuilder_html
 **/
static gchar *
egg_markdown_imagebuilder_html (gchar *alt_text, gchar *uri, gint link_id)
{
	return g_strdup_printf("<img src=\"%s\" alt=\"%s\">", uri, alt_text);
}

/**
 * egg_markdown_imagebuilder_text
 **/
static gchar *
egg_markdown_imagebuilder_text (gchar *alt_text, gchar *uri, gint link_id)
{
	return g_strdup(alt_text);
}

/**
 * egg_markdown_set_output:
 **/
gboolean
egg_markdown_set_output (EggMarkdown *self, EggMarkdownOutput output)
{
	gboolean ret = TRUE;
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);

	/* PangoMarkup */
	if (output == EGG_MARKDOWN_OUTPUT_PANGO) {
		self->priv->tags.em_start = "<i>";
		self->priv->tags.em_end = "</i>";
		self->priv->tags.strong_start = "<b>";
		self->priv->tags.strong_end = "</b>";
		self->priv->tags.strong_em_start = "<b><i>";
		self->priv->tags.strong_em_end = "</i></b>";
		self->priv->tags.code_start = "<tt><span bgcolor=\"#eee\">";
		self->priv->tags.code_end = "</span></tt>";
		self->priv->tags.codeblock_start = "<tt><span bgcolor=\"#eee\">";
		self->priv->tags.codeblock_end = "</span></tt>";
		self->priv->tags.strikethrough_start = "<s>";
		self->priv->tags.strikethrough_end = "</s>";
		self->priv->tags.h1_start = "<span color=\"#444\" size=\"xx-large\"><b>";
		self->priv->tags.h1_end = "</b></span>";
		self->priv->tags.h2_start = "<big><big><b>";
		self->priv->tags.h2_end = "</b></big></big>";
		self->priv->tags.h3_start = "<big><span variant=\"smallcaps\"><b>";
		self->priv->tags.h3_end = "</b></span></big>";
		self->priv->tags.bullett_start = "  • ";
		self->priv->tags.bullett_end = "";
		self->priv->tags.rule = "⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯\n";
		self->priv->tags.para_start = "";
		self->priv->tags.para_end = "";
		self->priv->tags.link_builder = egg_markdown_linkbuilder_pango;
		self->priv->tags.image_builder = egg_markdown_imagebuilder_pango;

	/* XHTML */
	} else if (output == EGG_MARKDOWN_OUTPUT_HTML) {
		self->priv->tags.em_start = "<em>";
		self->priv->tags.em_end = "</em>";
		self->priv->tags.strong_start = "<strong>";
		self->priv->tags.strong_end = "</strong>";
		self->priv->tags.strong_em_start = "<strong><em>";
		self->priv->tags.strong_em_end = "</em></strong>";
		self->priv->tags.code_start = "<code>";
		self->priv->tags.code_end = "</code>";
		self->priv->tags.codeblock_start = "<pre><code>";
		self->priv->tags.codeblock_end = "</code></pre>";
		self->priv->tags.strikethrough_start = "<strike>";
		self->priv->tags.strikethrough_end = "</strike>";
		self->priv->tags.h1_start = "<h1>";
		self->priv->tags.h1_end = "</h1>";
		self->priv->tags.h2_start = "<h2>";
		self->priv->tags.h2_end = "</h2>";
		self->priv->tags.h3_start = "<h3>";
		self->priv->tags.h3_end = "</h3>";
		self->priv->tags.bullett_start = "<li>";
		self->priv->tags.bullett_end = "</li>";
		self->priv->tags.rule = "<hr>";
		self->priv->tags.para_start = "<p>";
		self->priv->tags.para_end = "</p>";
		self->priv->tags.link_builder = egg_markdown_linkbuilder_html;
		self->priv->tags.image_builder = egg_markdown_imagebuilder_html;

	/* tty - use Linux/vt100 codes */
	} else if (output == EGG_MARKDOWN_OUTPUT_TTY) {
#define ESCP "\033"
		self->priv->tags.em_start = ESCP "[4m";
		self->priv->tags.em_end = ESCP "[24m";
		self->priv->tags.strong_start = ESCP "[1m";
		self->priv->tags.strong_end = ESCP "[21m";
		self->priv->tags.strong_em_start = ESCP "[1m" ESCP "[4m";
		self->priv->tags.strong_em_end = ESCP "[24m" ESCP "[21m";
		self->priv->tags.code_start = ESCP "[36m";
		self->priv->tags.code_end = ESCP "[39m";
		self->priv->tags.codeblock_start = ESCP "[36m";
		self->priv->tags.codeblock_end = ESCP "[39m";
		self->priv->tags.strikethrough_start = "--";
		self->priv->tags.strikethrough_end = "--";
		self->priv->tags.h1_start = ESCP "[7;1m";
		self->priv->tags.h1_end = ESCP "[27;21m";
		self->priv->tags.h2_start = ESCP "[7m";
		self->priv->tags.h2_end = ESCP "[27m";
		self->priv->tags.h3_start = ESCP "[1m"; // same as bold
		self->priv->tags.h3_end = ESCP "[21m";
		self->priv->tags.bullett_start = "* ";
		self->priv->tags.bullett_end = "";
		self->priv->tags.rule = " ----- \n";
		self->priv->tags.para_start = "";
		self->priv->tags.para_end = "";
		self->priv->tags.link_builder = egg_markdown_linkbuilder_text;
		self->priv->tags.image_builder = egg_markdown_imagebuilder_text;

	/* plain text */
	} else if (output == EGG_MARKDOWN_OUTPUT_TEXT) {
		self->priv->tags.em_start = "";
		self->priv->tags.em_end = "";
		self->priv->tags.strong_start = "";
		self->priv->tags.strong_end = "";
		self->priv->tags.code_start = "";
		self->priv->tags.code_end = "";
		self->priv->tags.codeblock_start = "";
		self->priv->tags.codeblock_end = "";
		self->priv->tags.strikethrough_start = "";
		self->priv->tags.strikethrough_end = "";
		self->priv->tags.h1_start = "[";
		self->priv->tags.h1_end = "]";
		self->priv->tags.h2_start = "-";
		self->priv->tags.h2_end = "-";
		self->priv->tags.h3_start = "~";
		self->priv->tags.h3_end = "~";
		self->priv->tags.bullett_start = "* ";
		self->priv->tags.bullett_end = "";
		self->priv->tags.rule = " ----- \n";
		self->priv->tags.para_start = "";
		self->priv->tags.para_end = "";
		self->priv->tags.link_builder = egg_markdown_linkbuilder_text;
		self->priv->tags.image_builder = egg_markdown_imagebuilder_text;

	/* unknown */
	} else {
		g_warning ("unknown output enum");
		ret = FALSE;
	}

	/* save if valid */
	if (ret)
		self->priv->output = output;
	return ret;
}

/**
 * egg_markdown_set_max_lines:
 **/
gboolean
egg_markdown_set_max_lines (EggMarkdown *self, gint max_lines)
{
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);
	self->priv->max_lines = max_lines;
	return TRUE;
}

/**
 * egg_markdown_set_smart_quoting:
 **/
gboolean
egg_markdown_set_smart_quoting (EggMarkdown *self, gboolean smart_quoting)
{
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);
	self->priv->smart_quoting = smart_quoting;
	return TRUE;
}

/**
 * egg_markdown_set_escape:
 **/
gboolean
egg_markdown_set_escape (EggMarkdown *self, gboolean escape)
{
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);
	self->priv->escape = escape;
	return TRUE;
}

/**
 * egg_markdown_set_autocode:
 **/
gboolean
egg_markdown_set_autocode (EggMarkdown *self, gboolean autocode)
{
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);
	self->priv->autocode = autocode;
	return TRUE;
}

/**
 * egg_markdown_set_exec:
 **/
gboolean
egg_markdown_set_exec (EggMarkdown *self, gboolean exec)
{
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);
	self->priv->exec = self->priv->global_exec = exec;
	return TRUE;
}

/**
 * egg_markdown_set_pot_output:
 **/
gboolean
egg_markdown_set_pot_output (EggMarkdown *self, FILE *f, gchar *input_filename)
{
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);
	self->priv->pot_output = f;
	self->priv->input_filename = g_strdup(input_filename);
	return TRUE;
}

/**
 * egg_markdown_set_use_gettext:
 **/
gboolean
egg_markdown_set_use_gettext (EggMarkdown *self, gboolean use_gettext)
{
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);
	self->priv->use_gettext = use_gettext;
	return TRUE;
}

/**
 * egg_markdown_set_extensions:
 **/
gboolean
egg_markdown_set_extensions (EggMarkdown *self, const EggMarkdownExtensions ext)
{
	g_return_val_if_fail (EGG_IS_MARKDOWN (self), FALSE);
	self->priv->extensions = ext;
	return TRUE;
}

/**
 * egg_markdown_parse:
 **/
gchar *
egg_markdown_parse (EggMarkdown *self, const gchar *markdown)
{
	gchar **lines;
	guint i;
	guint len;
	gchar *temp;
	gboolean ret;

	g_return_val_if_fail (EGG_IS_MARKDOWN (self), NULL);
	g_return_val_if_fail (self->priv->output != EGG_MARKDOWN_OUTPUT_UNKNOWN, NULL);

	DEBUG ("input='%s'", markdown);

	/* process */
	self->priv->mode = EGG_MARKDOWN_MODE_UNKNOWN;
	self->priv->codeblock_mode = EGG_MARKDOWN_CODEBLOCK_NONE;
	self->priv->codeblock_syntax = EGG_MARKDOWN_CODEBLOCK_SYNTAX_NONE;
	self->priv->line_count = 0;
	self->priv->line_number = 0;
	g_string_truncate (self->priv->pending, 0);
	g_string_truncate (self->priv->processed, 0);
	lines = g_strsplit (markdown, "\n", -1);
	len = g_strv_length (lines);
	self->priv->autocode_shebang = FALSE;

	/* process each line */
	for (i=0; i<len; (self->priv->line_number = ++i) ) {
		ret = egg_markdown_to_text_line_process (self, lines[i]);
		if (!ret)
			break;
	}
	g_strfreev (lines);
	egg_markdown_flush_pending (self);

	/* remove trailing \n */
	while (g_str_has_suffix (self->priv->processed->str, "\n"))
		g_string_set_size (self->priv->processed, self->priv->processed->len - 1);

	/* get a copy */
	temp = g_strdup (self->priv->processed->str);
	g_string_truncate (self->priv->pending, 0);
	g_string_truncate (self->priv->processed, 0);

	DEBUG ("output='%s'", temp);

	return temp;
}

/**
 * egg_markdown_class_init:
 * @klass: The EggMarkdownClass
 **/
static void
egg_markdown_class_init (EggMarkdownClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = egg_markdown_finalize;
	g_type_class_add_private (klass, sizeof (EggMarkdownPrivate));
}

/**
 * egg_markdown_init:
 **/
static void
egg_markdown_init (EggMarkdown *self)
{
	self->priv = EGG_MARKDOWN_GET_PRIVATE (self);

	self->priv->extensions = 0xffff; // enable all extensions
	self->priv->mode = EGG_MARKDOWN_MODE_UNKNOWN;
	self->priv->codeblock_mode = EGG_MARKDOWN_CODEBLOCK_NONE;
	self->priv->codeblock_syntax = EGG_MARKDOWN_CODEBLOCK_SYNTAX_NONE;
	self->priv->output = EGG_MARKDOWN_OUTPUT_UNKNOWN;
	self->priv->pending = g_string_new ("");
	self->priv->processed = g_string_new ("");
	self->priv->link_table = g_array_new(FALSE, FALSE, sizeof(gchar *));
	self->priv->max_lines = -1;
	self->priv->smart_quoting = FALSE;
	self->priv->escape = FALSE;
	self->priv->autocode = FALSE;
	self->priv->autocode_shebang = FALSE;
	self->priv->exec = self->priv->global_exec = FALSE;
	self->priv->pot_output = NULL;
	self->priv->temporary_disable_pot = FALSE;
	self->priv->use_gettext = FALSE;
	self->priv->pot_output = NULL;
	self->priv->input_filename = NULL;
}

/**
 * egg_markdown_finalize:
 * @object: The object to finalize
 **/
static void
egg_markdown_finalize (GObject *object)
{
	EggMarkdown *self;
	int i;

	g_return_if_fail (EGG_IS_MARKDOWN (object));

	self = EGG_MARKDOWN (object);

	g_return_if_fail (self->priv != NULL);
	g_string_free (self->priv->pending, TRUE);
	g_string_free (self->priv->processed, TRUE);

	for (i = 0; i < self->priv->link_table->len; i++) {
		g_free(g_array_index(self->priv->link_table, gchar *, i));
	}
	g_array_free (self->priv->link_table, TRUE);
	g_free (self->priv->input_filename);

	G_OBJECT_CLASS (egg_markdown_parent_class)->finalize (object);
}

/**
 * egg_markdown_new:
 *
 * Return value: a new EggMarkdown object.
 **/
EggMarkdown *
egg_markdown_new (void)
{
	EggMarkdown *self;
	self = g_object_new (EGG_TYPE_MARKDOWN, NULL);
	return EGG_MARKDOWN (self);
}

/***************************************************************************
 ***                          MAKE CHECK TESTS                           ***
 ***************************************************************************/
#ifdef EGG_TEST
#include "egg-test.h"

void
egg_markdown_test (EggTest *test)
{
	EggMarkdown *self;
	gchar *text;
	gboolean ret;
	const gchar *markdown;
	const gchar *markdown_expected;

	if (!egg_test_start (test, "EggMarkdown"))
		return;

	/************************************************************
	 ****************        line_is_rule          **************
	 ************************************************************/
	ret = egg_markdown_to_text_line_is_rule ("* * *");
	egg_test_title_assert (test, "is rule (1)", ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_rule ("***");
	egg_test_title_assert (test, "is rule (2)", ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_rule ("*****");
	egg_test_title_assert (test, "is rule (3)", ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_rule ("- - -");
	egg_test_title_assert (test, "is rule (4)", ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_rule ("---------------------------------------");
	egg_test_title_assert (test, "is rule (5)", ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_rule ("");
	egg_test_title_assert (test, "is rule (blank)", !ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_rule ("richard hughes");
	egg_test_title_assert (test, "is rule (text)", !ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_rule ("- richard-hughes");
	egg_test_title_assert (test, "is rule (bullet)", !ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_blank ("");
	egg_test_title_assert (test, "is blank (blank)", ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_blank (" \t ");
	egg_test_title_assert (test, "is blank (mix)", ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_blank ("richard hughes");
	egg_test_title_assert (test, "is blank (name)", !ret);

	/************************************************************/
	ret = egg_markdown_to_text_line_is_blank ("ccccccccc");
	egg_test_title_assert (test, "is blank (full)", !ret);


	/************************************************************
	 ****************           replace            **************
	 ************************************************************/
	text = egg_markdown_replace ("summary -- really -- sure!", " -- ", " – ");
	egg_test_title (test, "replace (multiple)");
	if (g_str_equal (text, "summary – really – sure!"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************
	 ****************          formatter           **************
	 ************************************************************/
	text = egg_markdown_to_text_line_formatter ("**is important** text", "**", "<b>", "</b>");
	egg_test_title (test, "formatter (left)");
	if (g_str_equal (text, "<b>is important</b> text"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************/
	text = egg_markdown_to_text_line_formatter ("this is **important**", "**", "<b>", "</b>");
	egg_test_title (test, "formatter (right)");
	if (g_str_equal (text, "this is <b>important</b>"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************/
	text = egg_markdown_to_text_line_formatter ("**important**", "**", "<b>", "</b>");
	egg_test_title (test, "formatter (only)");
	if (g_str_equal (text, "<b>important</b>"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************/
	text = egg_markdown_to_text_line_formatter ("***important***", "**", "<b>", "</b>");
	egg_test_title (test, "formatter (only)");
	if (g_str_equal (text, "<b>*important</b>*"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************/
	text = egg_markdown_to_text_line_formatter ("I guess * this is * not bold", "*", "<i>", "</i>");
	egg_test_title (test, "formatter (with spaces)");
	if (g_str_equal (text, "I guess * this is * not bold"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************/
	text = egg_markdown_to_text_line_formatter ("this **is important** text in **several** places", "**", "<b>", "</b>");
	egg_test_title (test, "formatter (middle, multiple)");
	if (g_str_equal (text, "this <b>is important</b> text in <b>several</b> places"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************/
	text = egg_markdown_word_auto_format_code ("this is http://www.hughsie.com/with_spaces_in_url inline link");
	egg_test_title (test, "auto formatter (url)");
	if (g_str_equal (text, "this is `http://www.hughsie.com/with_spaces_in_url` inline link"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************/
	text = egg_markdown_to_text_line_formatter ("this was \"triffic\" it was", "\"", "“", "”");
	egg_test_title (test, "formatter (quotes)");
	if (g_str_equal (text, "this was “triffic” it was"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************/
	text = egg_markdown_to_text_line_formatter ("This isn't a present", "'", "‘", "’");
	egg_test_title (test, "formatter (one quote)");
	if (g_str_equal (text, "This isn't a present"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got %s", text);
	g_free (text);

	/************************************************************
	 ****************          markdown            **************
	 ************************************************************/
	egg_test_title (test, "get EggMarkdown object");
	self = egg_markdown_new ();
	egg_test_assert (test, self != NULL);

	/************************************************************/
	ret = egg_markdown_set_output (self, EGG_MARKDOWN_OUTPUT_PANGO);
	egg_test_title_assert (test, "set pango output", ret);

	/************************************************************/
	markdown = "OEMs\n"
		   "====\n"
		   " - Bullett\n";
	markdown_expected =
		   "<big>OEMs</big>\n"
		   "• Bullett";
	egg_test_title (test, "markdown (type2 header)");
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);


	/************************************************************/
	markdown = "this is http://www.hughsie.com/with_spaces_in_url inline link\n";
	markdown_expected = "this is <tt>http://www.hughsie.com/with_spaces_in_url</tt> inline link";
	egg_test_title (test, "markdown (autocode)");
	egg_markdown_set_autocode (self, TRUE);
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	/************************************************************/
	markdown = "*** This software is currently in alpha state ***\n";
	markdown_expected = "<b><i> This software is currently in alpha state </b></i>";
	egg_test_title (test, "markdown some invalid header");
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	/************************************************************/
	markdown = " - This is a *very*\n"
		   "   short paragraph\n"
		   "   that is not usual.\n"
		   " - Another";
	markdown_expected =
		   "• This is a <i>very</i> short paragraph that is not usual.\n"
		   "• Another";
	egg_test_title (test, "markdown (complex1)");
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	/************************************************************/
	markdown = "*  This is a *very*\n"
		   "   short paragraph\n"
		   "   that is not usual.\n"
		   "*  This is the second\n"
		   "   bullett point.\n"
		   "*  And the third.\n"
		   " \n"
		   "* * *\n"
		   " \n"
		   "Paragraph one\n"
		   "isn't __very__ long at all.\n"
		   "\n"
		   "Paragraph two\n"
		   "isn't much better.";
	markdown_expected =
		   "• This is a <i>very</i> short paragraph that is not usual.\n"
		   "• This is the second bullett point.\n"
		   "• And the third.\n"
		   "⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯\n"
		   "Paragraph one isn't <b>very</b> long at all.\n"
		   "Paragraph two isn't much better.";
	egg_test_title (test, "markdown (complex1)");
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	/************************************************************/
	markdown = "This is a spec file description or\n"
		   "an **update** description in bohdi.\n"
		   "\n"
		   "* * *\n"
		   "# Big title #\n"
		   "\n"
		   "The *following* things 'were' fixed:\n"
		   "- Fix `dave`\n"
		   "* Fubar update because of \"security\"\n";
	markdown_expected =
		   "This is a spec file description or an <b>update</b> description in bohdi.\n"
		   "⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯\n"
		   "<big>Big title</big>\n"
		   "The <i>following</i> things 'were' fixed:\n"
		   "• Fix <tt>dave</tt>\n"
		   "• Fubar update because of \"security\"";
	egg_test_title (test, "markdown (complex2)");
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	/************************************************************/
	markdown = "* list seporated with spaces -\n"
		   "  first item\n"
		   "\n"
		   "* second item\n"
		   "\n"
		   "* third item\n";
	markdown_expected =
		   "• list seporated with spaces - first item\n"
		   "• second item\n"
		   "• third item";
	egg_test_title (test, "markdown (list with spaces)");
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	/************************************************************/
	ret = egg_markdown_set_max_lines (self, 1);
	egg_test_title_assert (test, "set max_lines", ret);

	/************************************************************/
	markdown = "* list seporated with spaces -\n"
		   "  first item\n"
		   "* second item\n";
	markdown_expected =
		   "• list seporated with spaces - first item";
	egg_test_title (test, "markdown (one line limit)");
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	/************************************************************/
	ret = egg_markdown_set_max_lines (self, 1);
	egg_test_title_assert (test, "set max_lines", ret);

	/************************************************************/
	markdown = "* list & spaces";
	markdown_expected =
		   "• list & spaces";
	egg_test_title (test, "markdown (escaping)");
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	/************************************************************/
	egg_test_title (test, "markdown (free text)");
	text = egg_markdown_parse (self, "This isn't a present");
	if (g_str_equal (text, "This isn't a present"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s'", text);
	g_free (text);

	/************************************************************/
	egg_test_title (test, "markdown (autotext underscore)");
	text = egg_markdown_parse (self, "This isn't CONFIG_UEVENT_HELPER_PATH present");
	if (g_str_equal (text, "This isn't <tt>CONFIG_UEVENT_HELPER_PATH</tt> present"))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s'", text);
	g_free (text);

	/************************************************************/
	markdown = "*Thu Mar 12 12:00:00 2009* Dan Walsh <dwalsh@redhat.com> - 2.0.79-1\n"
		   "- Update to upstream \n"
		   " * Netlink socket handoff patch from Adam Jackson.\n"
		   " * AVC caching of compute_create results by Eric Paris.\n"
		   "\n"
		   "*Tue Mar 10 12:00:00 2009* Dan Walsh <dwalsh@redhat.com> - 2.0.78-5\n"
		   "- Add patch from ajax to accellerate X SELinux \n"
		   "- Update eparis patch\n";
	markdown_expected =
		   "<i>Thu Mar 12 12:00:00 2009</i> Dan Walsh <tt>&lt;dwalsh@redhat.com&gt;</tt> - 2.0.79-1\n"
		   "• Update to upstream\n"
		   "• Netlink socket handoff patch from Adam Jackson.\n"
		   "• AVC caching of compute_create results by Eric Paris.\n"
		   "<i>Tue Mar 10 12:00:00 2009</i> Dan Walsh <tt>&lt;dwalsh@redhat.com&gt;</tt> - 2.0.78-5\n"
		   "• Add patch from ajax to accellerate X SELinux\n"
		   "• Update eparis patch";
	egg_test_title (test, "markdown (end of bullett)");
	egg_markdown_set_escape (self, TRUE);
	ret = egg_markdown_set_max_lines (self, 1024);
	text = egg_markdown_parse (self, markdown);
	if (g_str_equal (text, markdown_expected))
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "failed, got '%s', expected '%s'", text, markdown_expected);
	g_free (text);

	g_object_unref (self);

	egg_test_end (test);
}
#endif

