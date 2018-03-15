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

#ifndef __EGG_MARKDOWN_H
#define __EGG_MARKDOWN_H

#include <glib-object.h>

G_BEGIN_DECLS

#define EGG_TYPE_MARKDOWN		(egg_markdown_get_type ())
#define EGG_MARKDOWN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), EGG_TYPE_MARKDOWN, EggMarkdown))
#define EGG_MARKDOWN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), EGG_TYPE_MARKDOWN, EggMarkdownClass))
#define EGG_IS_MARKDOWN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), EGG_TYPE_MARKDOWN))
#define EGG_IS_MARKDOWN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), EGG_TYPE_MARKDOWN))
#define EGG_MARKDOWN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), EGG_TYPE_MARKDOWN, EggMarkdownClass))
#define EGG_MARKDOWN_ERROR		(egg_markdown_error_quark ())
#define EGG_MARKDOWN_TYPE_ERROR	(egg_markdown_error_get_type ())

#define EGG_MARKDOWN_MAX_LINE_LENGTH	2048

typedef struct EggMarkdownPrivate EggMarkdownPrivate;

typedef struct
{
	GObject		 parent;
	EggMarkdownPrivate	*priv;
} EggMarkdown;

typedef struct
{
	GObjectClass parent_class;
	void (* active_changed)	(EggMarkdown *self, gboolean active);
} EggMarkdownClass;

typedef enum {
	EGG_MARKDOWN_OUTPUT_TTY,	
	EGG_MARKDOWN_OUTPUT_TEXT,
	EGG_MARKDOWN_OUTPUT_PANGO,
	EGG_MARKDOWN_OUTPUT_HTML,
	EGG_MARKDOWN_OUTPUT_UNKNOWN
} EggMarkdownOutput;

typedef enum {
	EGG_MARKDOWN_EXTENSION_NONE = 0,
	EGG_MARKDOWN_EXTENSION_SHEBANG = 1 << 0,
	EGG_MARKDOWN_EXTENSION_GITHUB  = 1 << 1
} EggMarkdownExtensions;

GType		 egg_markdown_get_type (void);
EggMarkdown	*egg_markdown_new (void);
gboolean	 egg_markdown_set_output        (EggMarkdown *self, EggMarkdownOutput output);
gboolean	 egg_markdown_set_max_lines     (EggMarkdown *self, gint max_lines);
gboolean	 egg_markdown_set_smart_quoting (EggMarkdown *self, gboolean smart_quoting);
gboolean	 egg_markdown_set_escape        (EggMarkdown *self, gboolean escape);
gboolean	 egg_markdown_set_autocode      (EggMarkdown *self, gboolean autocode);
gboolean	 egg_markdown_set_exec          (EggMarkdown *self, gboolean exec);
gboolean	 egg_markdown_set_pot_output    (EggMarkdown *self, FILE *f, gchar* input_filename);
gboolean	 egg_markdown_set_use_gettext   (EggMarkdown *self, gboolean use_gettext);
gboolean     egg_markdown_set_extensions    (EggMarkdown *self, const EggMarkdownExtensions ext);
gchar		*egg_markdown_parse             (EggMarkdown *self, const gchar *text);
void		 egg_markdown_clear             (EggMarkdown *self);
gchar 		*egg_markdown_get_link_uri      (EggMarkdown *self, const gint link_id);

#define TEXTDOMAIN_DIRECTIVE "%%textdomain "
#define EXEC_DIRECTIVE "%%exec "
#define NOPOT_DIRECTIVE "%%nopot "

G_END_DECLS

#endif /* __EGG_MARKDOWN_H */

