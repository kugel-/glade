/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Tristan Van Berkom.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors:
 *   Tristan Van Berkom <tvb@gnome.org>
 */
#ifndef _GLADE_MODEL_DATA_TREE_H_
#define _STV_CAP_H_

#include <glib.h>

G_BEGIN_DECLS

struct _GladeModelData
{
	GValue    value;
	gchar    *name;
	
	gboolean  i18n_translatable;
	gchar    *i18n_context;
	gchar    *i18n_comment;
};

typedef struct _GladeModelData         GladeModelData;
typedef struct _GladeParamModelData    GladeParamModelData;


#define	GLADE_TYPE_MODEL_DATA_TREE  (glade_model_data_tree_get_type())
#define	GLADE_TYPE_PARAM_MODEL_DATA (glade_param_model_data_get_type())
#define GLADE_TYPE_EPROP_MODEL_DATA (glade_eprop_model_data_get_type())

#define GLADE_IS_PARAM_SPEC_MODEL_DATA(pspec) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GLADE_TYPE_PARAM_MODEL_DATA))
#define GLADE_PARAM_SPEC_MODEL_DATA(pspec)    \
	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), GLADE_TYPE_PARAM_MODEL_DATA, GladeParamSpecModelDataTree))

GType           glade_model_data_tree_get_type     (void) G_GNUC_CONST;
GType           glade_param_model_data_get_type    (void) G_GNUC_CONST;
GType           glade_eprop_model_data_get_type    (void) G_GNUC_CONST;

GParamSpec     *glade_standard_model_data_spec     (void);


GladeModelData *glade_model_data_new               (GType           type);
GladeModelData *glade_model_data_copy              (GladeModelData *data);
void            glade_model_data_free              (GladeModelData *data);

GNode          *glade_model_data_tree_copy         (GNode          *node);
void            glade_model_data_tree_free         (GNode          *node);

void            glade_model_data_insert_column     (GNode          *node,
						    GType           type,
						    gint            nth);
void            glade_model_data_remove_column     (GNode          *node,
						    GType           type,
						    gint            nth);
void            glade_model_data_reorder_column    (GNode          *node,
						    gint            column,
						    gint            nth);


G_END_DECLS

#endif /* _GLADE_MODEL_DATA_H_ */