/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * glade-attributes.c - Editing support for pango attributes
 *
 * Copyright (C) 2008 Tristan Van Berkom
 *
 * Author(s):
 *      Tristan Van Berkom <tvb@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gladeui/glade.h>
#include <glib/gi18n-lib.h>

#include "glade-attributes.h"


#define GLADE_RESPONSE_CLEAR 42


struct _GladeParamSpecAttributes {
	GParamSpec    parent_instance;
};


static GList *
glade_attr_list_copy (GList *attrs)
{
	GList          *ret = NULL, *list;
	GladeAttribute *attr, *dup_attr;

	for (list = attrs; list; list = list->next)
	{
		attr = list->data;

		dup_attr            = g_new0 (GladeAttribute, 1);
		dup_attr->type      = attr->type;
		dup_attr->start     = attr->start;
		dup_attr->end       = attr->end;
		g_value_init (&(dup_attr->value), G_VALUE_TYPE (&(attr->value)));
		g_value_copy (&(attr->value), &(dup_attr->value));

		ret = g_list_prepend (ret, dup_attr);
	}

	return g_list_reverse (ret);
}

void
glade_attr_list_free (GList *attrs)
{
	GList          *list;
	GladeAttribute *attr;

	for (list = attrs; list; list = list->next)
	{
		attr = list->data;

		g_value_unset (&(attr->value));
		g_free (attr);
	}
	g_list_free (attrs);
}

GType
glade_attr_glist_get_type (void)
{
	static GType type_id = 0;

	if (!type_id)
		type_id = g_boxed_type_register_static
			("GladeAttrGList", 
			 (GBoxedCopyFunc) glade_attr_list_copy,
			 (GBoxedFreeFunc) glade_attr_list_free);
	return type_id;
}

static void
param_attributes_init (GParamSpec *pspec)
{
}

static void
param_attributes_set_default (GParamSpec *pspec,
			      GValue     *value)
{
	if (value->data[0].v_pointer != NULL)
	{
		glade_attr_list_free (value->data[0].v_pointer);
	}
	value->data[0].v_pointer = NULL;
}

static gboolean
param_attributes_validate (GParamSpec *pspec,
			   GValue     *value)
{
	return TRUE;
}

static gint
param_attributes_values_cmp (GParamSpec   *pspec,
			  const GValue *value1,
			  const GValue *value2)
{
  guint8 *p1 = value1->data[0].v_pointer;
  guint8 *p2 = value2->data[0].v_pointer;

  /* not much to compare here, try to at least provide stable lesser/greater result */

  return p1 < p2 ? -1 : p1 > p2;
}

GType
glade_param_attributes_get_type (void)
{
	static GType attributes_type = 0;

	if (attributes_type == 0)
	{
		static /* const */ GParamSpecTypeInfo pspec_info = {
			sizeof (GladeParamSpecAttributes),  /* instance_size */
			16,                         /* n_preallocs */
			param_attributes_init,         /* instance_init */
			0xdeadbeef,                 /* value_type, assigned further down */
			NULL,                       /* finalize */
			param_attributes_set_default,  /* value_set_default */
			param_attributes_validate,     /* value_validate */
			param_attributes_values_cmp,   /* values_cmp */
		};
		pspec_info.value_type = GLADE_TYPE_ATTR_GLIST;

		attributes_type = g_param_type_register_static
			("GladeParamAttributes", &pspec_info);
	}
	return attributes_type;
}

GParamSpec *
glade_param_spec_attributes (const gchar   *name,
			     const gchar   *nick,
			     const gchar   *blurb,
			     GParamFlags    flags)
{
	GladeParamSpecAttributes *pspec;

	pspec = g_param_spec_internal (GLADE_TYPE_PARAM_ATTRIBUTES,
				       name, nick, blurb, flags);
  
	return G_PARAM_SPEC (pspec);
}

GParamSpec *
glade_gtk_attributes_spec (void)
{
	return glade_param_spec_attributes ("attributes", _("Attributes"), 
					    _("A list of attributes"),
					    G_PARAM_READWRITE);
}




/**************************************************************
 *              GladeEditorProperty stuff here
 **************************************************************/
typedef struct {
	GladeEditorProperty parent_instance;

	GtkTreeModel *model;
	
} GladeEPropAttrs;

GLADE_MAKE_EPROP (GladeEPropAttrs, glade_eprop_attrs)
#define GLADE_EPROP_ATTRS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GLADE_TYPE_EPROP_ATTRS, GladeEPropAttrs))
#define GLADE_EPROP_ATTRS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GLADE_TYPE_EPROP_ATTRS, GladeEPropAttrsClass))
#define GLADE_IS_EPROP_ATTRS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GLADE_TYPE_EPROP_ATTRS))
#define GLADE_IS_EPROP_ATTRS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GLADE_TYPE_EPROP_ATTRS))
#define GLADE_EPROP_ATTRS_GET_CLASS(o)    (G_TYPE_INSTANCE_GET_CLASS ((o), GLADE_EPROP_ATTRS, GladeEPropAttrsClass))

enum {

	/* Main Data */
	COLUMN_NAME,          /* The title string for PangoAttrType */
	COLUMN_NAME_WEIGHT,   /* For bold names */
	COLUMN_TYPE,          /* The PangoAttrType */
	COLUMN_EDIT_TYPE,     /* The AttrEditType (below) */
	COLUMN_VALUE,         /* The value */
	COLUMN_START,         /* attribute start value */
	COLUMN_END,           /* attribute end value */

	/* Editor renderer related */
	COLUMN_TOGGLE_ACTIVE, /* whether the toggle renderer is being used */
	COLUMN_TOGGLE_DOWN,   /* whether the toggle should be displayed in "downstate" */

	COLUMN_TEXT_ACTIVE,   /* whether the bare naked text renderer is being used (to launch dialogs) */
	COLUMN_TEXT,          /* text attribute value for all text derived renderers */
	COLUMN_TEXT_STYLE,    /* whether to make italic */
	COLUMN_TEXT_FG,       /* forground colour of the text */

	COLUMN_COMBO_ACTIVE,  /* whether the combobox renderer is being used */
	COLUMN_COMBO_MODEL,   /* the model for the dropdown list */

	COLUMN_SPIN_ACTIVE,   /* whether the spin renderer is being used */
	COLUMN_SPIN_DIGITS,   /* How many decimal points to show (used to edit float values) */

	NUM_COLUMNS
};


typedef enum {
	EDIT_TOGGLE = 0,
	EDIT_COMBO,
	EDIT_SPIN,
	EDIT_COLOR,
	EDIT_INVALID
} AttrEditType;

#define ACTIVATE_COLUMN_FROM_TYPE(type)                 \
	((type) == EDIT_TOGGLE ? COLUMN_TOGGLE_ACTIVE : \
	 (type) == EDIT_SPIN ? COLUMN_SPIN_ACTIVE :     \
	 (type) == EDIT_COMBO ? COLUMN_COMBO_ACTIVE: COLUMN_TEXT_ACTIVE)

static GtkListStore *
make_model_from_enum_type (GType enum_type)
{
	GtkListStore        *store;
	GtkTreeIter          iter;
	GEnumClass          *eclass;
	guint                i;

	eclass = g_type_class_ref (enum_type);

	store = gtk_list_store_new (1, G_TYPE_STRING);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, _("Unset"), 
			    -1);
	
	for (i = 0; i < eclass->n_values; i++)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    0, eclass->values[i].value_nick, 
				    -1);
	}

	g_type_class_unref (eclass);

	return store;
}

static GtkListStore *
get_enum_model_for_combo (PangoAttrType type)
{
	static GtkListStore *style_store = NULL, 
		*weight_store = NULL, *variant_store = NULL,
		*stretch_store = NULL, *gravity_store = NULL,
		*gravity_hint_store = NULL, *default_store = NULL;

	switch (type) 
	{
	case PANGO_ATTR_STYLE:
		if (!style_store)
			style_store = make_model_from_enum_type (PANGO_TYPE_STYLE);
		return style_store;

	case PANGO_ATTR_WEIGHT:
		if (!weight_store)
			weight_store = make_model_from_enum_type (PANGO_TYPE_WEIGHT);
		return weight_store;

	case PANGO_ATTR_VARIANT:
		if (!variant_store)
			variant_store = make_model_from_enum_type (PANGO_TYPE_VARIANT);
		return variant_store;

	case PANGO_ATTR_STRETCH:
		if (!stretch_store)
			stretch_store = make_model_from_enum_type (PANGO_TYPE_STRETCH);
		return stretch_store;

	case PANGO_ATTR_GRAVITY:	
		if (!gravity_store)
			gravity_store = make_model_from_enum_type (PANGO_TYPE_GRAVITY);
		return gravity_store;

	case PANGO_ATTR_GRAVITY_HINT:
		if (!gravity_hint_store)
			gravity_hint_store = make_model_from_enum_type (PANGO_TYPE_GRAVITY_HINT);
		return gravity_hint_store;

	default:
		if (!default_store)
			default_store = gtk_list_store_new (1, G_TYPE_STRING);
		return default_store;
	}
}

static GtkTreeIter *
get_last_iter (GtkTreeModel  *model, 
	       PangoAttrType  type)
{
	GtkTreeIter *last = NULL, iter;
	PangoAttrType  iter_type;

	if (!gtk_tree_model_iter_children (model, &iter, NULL))
		return NULL;

	do 
	{
		gtk_tree_model_get (model, &iter,
				    COLUMN_TYPE, &iter_type,
				    -1);
		if (iter_type == type)
		{
			if (last)
				gtk_tree_iter_free (last);

			last = gtk_tree_iter_copy (&iter);
		}
	} 
	while (gtk_tree_model_iter_next (model, &iter));

	return last;
}

static gboolean
append_empty_row (GtkTreeStore   *store,
		  PangoAttrType   type,
		  GtkTreeIter    *place,
		  GtkTreeIter   **new_row)
{
	gchar        *name = NULL;
	GtkListStore *model = get_enum_model_for_combo (type);
	GtkTreeIter   iter, *last;
	AttrEditType  edit_type = EDIT_INVALID;

	switch (type) 
	{
		/* PangoAttrLanguage */
	case PANGO_ATTR_LANGUAGE:
		
		break;
		/* PangoAttrInt */
	case PANGO_ATTR_STYLE:	
		edit_type = EDIT_COMBO;
		name = _("Style");
		break;
	case PANGO_ATTR_WEIGHT:
		edit_type = EDIT_COMBO;
		name = _("Weight");
		break;
	case PANGO_ATTR_VARIANT:
		edit_type = EDIT_COMBO;
		name = _("Variant");
		break;
	case PANGO_ATTR_STRETCH:
		edit_type = EDIT_COMBO;
		name = _("Stretch");
		break;
	case PANGO_ATTR_UNDERLINE:
		edit_type = EDIT_TOGGLE;
		name = _("Underline");
		break;
	case PANGO_ATTR_STRIKETHROUGH:	
		edit_type = EDIT_TOGGLE;
		name = _("Strikethrough");
		break;
	case PANGO_ATTR_GRAVITY:
		edit_type = EDIT_COMBO;
		name = _("Gravity");
		break;
	case PANGO_ATTR_GRAVITY_HINT:
		edit_type = EDIT_COMBO;
		name = _("Gravity Hint");
		break;
		
		/* PangoAttrString */	  
	case PANGO_ATTR_FAMILY:
		/* Use a simple editable text renderer ? */
		break;
		
		/* PangoAttrSize */	  
	case PANGO_ATTR_SIZE:
		edit_type = EDIT_SPIN;
		name = _("Size");
		break;
	case PANGO_ATTR_ABSOLUTE_SIZE:
		edit_type = EDIT_SPIN;
		name = _("Absolute Size");
		break;
					
		/* PangoAttrColor */
		/* Colours need editors... */
	case PANGO_ATTR_FOREGROUND:
		edit_type = EDIT_COLOR;
		name = _("Foreground Color");
		break;
	case PANGO_ATTR_BACKGROUND: 
		edit_type = EDIT_COLOR;
		name = _("Background Color");
		break;
	case PANGO_ATTR_UNDERLINE_COLOR:
		edit_type = EDIT_COLOR;
		name = _("Underline Color");
		break;
	case PANGO_ATTR_STRIKETHROUGH_COLOR:
		edit_type = EDIT_COLOR;
		name = _("Strikethrough Color");
		break;
		
		/* PangoAttrShape */
	case PANGO_ATTR_SHAPE:
		/* Unsupported for now */
		break;
		/* PangoAttrFloat */
	case PANGO_ATTR_SCALE:
		edit_type = EDIT_SPIN;
		name = _("Scale");	
		break;
		
	case PANGO_ATTR_INVALID:
	case PANGO_ATTR_LETTER_SPACING:
	case PANGO_ATTR_RISE:
	case PANGO_ATTR_FALLBACK:
	case PANGO_ATTR_FONT_DESC:
	default:
		break;
	}

	if (name)
	{
		if (place)
		{
			gtk_tree_store_insert_before (store, &iter, NULL, place);
		}
		else if ((last = get_last_iter (GTK_TREE_MODEL (store), type)) != NULL)
		{
			gtk_tree_store_insert_after (store, &iter, NULL, last);
			gtk_tree_iter_free (last);
		}
		else
			gtk_tree_store_append (store, &iter, NULL);

		gtk_tree_store_set (store, &iter,
				    COLUMN_NAME, name,
				    COLUMN_TYPE, type,
				    COLUMN_EDIT_TYPE, edit_type,
				    COLUMN_NAME_WEIGHT, PANGO_WEIGHT_NORMAL,
				    COLUMN_TEXT, _("<Enter Value>"),
				    COLUMN_TEXT_STYLE, PANGO_STYLE_ITALIC,
				    COLUMN_TEXT_FG, "Grey",
				    ACTIVATE_COLUMN_FROM_TYPE (edit_type), TRUE,
				    COLUMN_COMBO_MODEL, model,
				    -1);

		if (new_row)
			*new_row = gtk_tree_iter_copy (&iter);

		return TRUE;
	}

	return FALSE;
}

static gboolean
is_empty_row (GtkTreeModel  *model,
	      GtkTreeIter   *iter)
{

	PangoAttrType   attr_type;
	AttrEditType    edit_type;
	gboolean        bval;
	gchar          *strval = NULL;
	gboolean        empty_row = FALSE;

	/* First get the basic values */
	gtk_tree_model_get (model, iter,
			    COLUMN_TYPE, &attr_type,
			    COLUMN_EDIT_TYPE, &edit_type,
			    COLUMN_TOGGLE_DOWN, &bval,
			    COLUMN_TEXT, &strval,
			    -1);

	/* Ignore all other types */
	switch (edit_type)
	{
	case EDIT_TOGGLE:
		if (!bval)
			empty_row = TRUE;
		break;
	case EDIT_COMBO:
		if (!strval || !strcmp (strval, _("Unset")) || !strcmp (strval, _("<Enter Value>")))
			empty_row = TRUE;
		break;
	case EDIT_SPIN:
		/* XXX Interesting... can we get the defaults ? what can we do to let the user
		 * unset the value ?? 
		 */
		if (!strval || !strcmp (strval, "0") || !strcmp (strval, _("<Enter Value>")))
			empty_row = TRUE;
		break;
	case EDIT_COLOR:
		if (!strval || strval[0] == '\0' || !strcmp (strval, _("<Enter Value>")))
			empty_row = TRUE;
		break;
	case EDIT_INVALID:
	default:
			break;
	}
	g_free (strval);

	return empty_row;
}

static GtkTreeIter *
remove_empty_rows (GtkTreeModel *model,
		   AttrEditType  edit_type,
		   PangoAttrType type)
{
 	GtkTreeIter  iter, *ret = NULL, *this_iter = NULL;
	AttrEditType iter_type;
	gboolean     valid;

	valid = gtk_tree_model_iter_children (model, &iter, NULL);

	while (valid)
	{
		if (this_iter) this_iter = (gtk_tree_iter_free (this_iter), NULL);
		this_iter = gtk_tree_iter_copy (&iter);

		gtk_tree_model_get (model, this_iter,
				    COLUMN_TYPE, &iter_type,
				    -1);

		valid = gtk_tree_model_iter_next (model, &iter);

		if (iter_type == type && is_empty_row (model, this_iter))
		{
			if (valid)
			{
				if (ret) ret = (gtk_tree_iter_free (ret), NULL);
				ret = gtk_tree_iter_copy (&iter);
			}
			gtk_tree_store_remove (GTK_TREE_STORE (model), this_iter);
		}
	} 

	if (this_iter) gtk_tree_iter_free (this_iter);

	return ret;
}

static void
ensure_empty_row (GtkTreeStore   *store,
		  AttrEditType    edit_type,
		  PangoAttrType   type)
{
	GtkTreeIter *place;

	place = remove_empty_rows (GTK_TREE_MODEL (store), edit_type, type);
	append_empty_row (store, type, place, NULL);
	if (place)
		gtk_tree_iter_free (place);
}

static GType
type_from_attr_type (PangoAttrType type)
{
	GType gtype = 0;

	switch (type)
	{
	case PANGO_ATTR_LANGUAGE:
	case PANGO_ATTR_FAMILY:
		return G_TYPE_STRING;
		
	case PANGO_ATTR_STYLE:        return PANGO_TYPE_STYLE;
	case PANGO_ATTR_WEIGHT:       return PANGO_TYPE_WEIGHT;
	case PANGO_ATTR_VARIANT:      return PANGO_TYPE_VARIANT;
	case PANGO_ATTR_STRETCH:      return PANGO_TYPE_STRETCH;
	case PANGO_ATTR_GRAVITY:      return PANGO_TYPE_GRAVITY;
	case PANGO_ATTR_GRAVITY_HINT: return PANGO_TYPE_GRAVITY_HINT;

	case PANGO_ATTR_UNDERLINE:
	case PANGO_ATTR_STRIKETHROUGH:	
		return G_TYPE_BOOLEAN;
		
	case PANGO_ATTR_SIZE:
	case PANGO_ATTR_ABSOLUTE_SIZE:
		return G_TYPE_INT;

	case PANGO_ATTR_SCALE:
		return G_TYPE_DOUBLE;
		
		/* PangoAttrColor */
	case PANGO_ATTR_FOREGROUND:
	case PANGO_ATTR_BACKGROUND: 
	case PANGO_ATTR_UNDERLINE_COLOR:
	case PANGO_ATTR_STRIKETHROUGH_COLOR:
		/* boxed colours */
		return GDK_TYPE_COLOR;
		
		/* PangoAttrShape */
	case PANGO_ATTR_SHAPE:
		/* Unsupported for now */
		break;
		
	case PANGO_ATTR_INVALID:
	case PANGO_ATTR_LETTER_SPACING:
	case PANGO_ATTR_RISE:
	case PANGO_ATTR_FALLBACK:
	case PANGO_ATTR_FONT_DESC:
	default:
		break;
	}


	return gtype;
}

gchar *
glade_gtk_string_from_attr (GladeAttribute *gattr)
{
	gchar    *ret = NULL;
	gint      ival;
	gdouble   fval;
	GdkColor *color;

	switch (gattr->type)
	{
	case PANGO_ATTR_LANGUAGE:
	case PANGO_ATTR_FAMILY:
		ret = g_value_dup_string (&(gattr->value));
		break;

	case PANGO_ATTR_STYLE:
	case PANGO_ATTR_WEIGHT:
	case PANGO_ATTR_VARIANT:
	case PANGO_ATTR_STRETCH:
	case PANGO_ATTR_GRAVITY:
	case PANGO_ATTR_GRAVITY_HINT:

		/* Enums ... */
		ival = g_value_get_enum (&(gattr->value));
		ret = glade_utils_enum_string_from_value (G_VALUE_TYPE (&(gattr->value)), ival);
		break;


	case PANGO_ATTR_UNDERLINE:
	case PANGO_ATTR_STRIKETHROUGH:	
		/* Booleans */
		if (g_value_get_boolean (&(gattr->value)))
			ret = g_strdup_printf ("True");
		else
			ret = g_strdup_printf ("False");
		break;
		
		/* PangoAttrSize */	  
	case PANGO_ATTR_SIZE:
	case PANGO_ATTR_ABSOLUTE_SIZE:
		/* ints */
		ival = g_value_get_int (&(gattr->value));
		ret = g_strdup_printf ("%d", ival);
		break;

		/* PangoAttrFloat */
	case PANGO_ATTR_SCALE:
		/* doubles */
		fval = g_value_get_double (&(gattr->value));
		ret = g_strdup_printf ("%f", fval);
		break;
		
		/* PangoAttrColor */
	case PANGO_ATTR_FOREGROUND:
	case PANGO_ATTR_BACKGROUND: 
	case PANGO_ATTR_UNDERLINE_COLOR:
	case PANGO_ATTR_STRIKETHROUGH_COLOR:
		/* boxed colours */
		color = g_value_get_boxed (&(gattr->value));
		ret = gdk_color_to_string (color);
		break;
		
		/* PangoAttrShape */
	case PANGO_ATTR_SHAPE:
		/* Unsupported for now */
		break;
		
	case PANGO_ATTR_INVALID:
	case PANGO_ATTR_LETTER_SPACING:
	case PANGO_ATTR_RISE:
	case PANGO_ATTR_FALLBACK:
	case PANGO_ATTR_FONT_DESC:
	default:
		break;
	}

	return ret;
}

static gint
enum_value_from_string (PangoAttrType type, const gchar *strval)
{
	GEnumClass *enum_class;
	GEnumValue *enum_value;
	gint        value = 0;

	enum_class = g_type_class_ref (type_from_attr_type (type));
	if ((enum_value = g_enum_get_value_by_nick (enum_class, strval)) != NULL)
		value = enum_value->value;
	else
		g_critical ("Couldnt find enum value for %s, type %s", 
			    strval, g_type_name (type_from_attr_type (type)));
	
	g_type_class_unref (enum_class);
	
	return value;
}

GladeAttribute *
glade_gtk_attribute_from_string (PangoAttrType  type,
				 const gchar   *strval)
{
	GladeAttribute    *gattr;
	GdkColor           color;

	gattr        = g_new0 (GladeAttribute, 1);
	gattr->type  = type;
	gattr->start = 0;
	gattr->end   = G_MAXUINT;

	switch (type)
	{
	case PANGO_ATTR_LANGUAGE:
	case PANGO_ATTR_FAMILY:
	case PANGO_ATTR_FONT_DESC:
		g_value_init (&(gattr->value), G_TYPE_STRING);
		g_value_set_string (&(gattr->value), strval);
		break;

	case PANGO_ATTR_STYLE:
	case PANGO_ATTR_WEIGHT:
	case PANGO_ATTR_VARIANT:
	case PANGO_ATTR_STRETCH:
	case PANGO_ATTR_GRAVITY:
	case PANGO_ATTR_GRAVITY_HINT:

		/* Enums ... */
		g_value_init (&(gattr->value), type_from_attr_type (type));
		g_value_set_enum (&(gattr->value), enum_value_from_string (type, strval));
		break;


	case PANGO_ATTR_UNDERLINE:
	case PANGO_ATTR_STRIKETHROUGH:	
		/* Booleans */
		g_value_init (&(gattr->value), G_TYPE_BOOLEAN);
		g_value_set_boolean (&(gattr->value), TRUE);
		break;
		
		/* PangoAttrSize */	  
	case PANGO_ATTR_SIZE:
	case PANGO_ATTR_ABSOLUTE_SIZE:
		/* ints */
		g_value_init (&(gattr->value), G_TYPE_INT);
		g_value_set_int (&(gattr->value), strtol (strval, NULL, 10));
		break;

		/* PangoAttrFloat */
	case PANGO_ATTR_SCALE:
		/* doubles */
		g_value_init (&(gattr->value), G_TYPE_DOUBLE);
		g_value_set_double (&(gattr->value), strtod (strval, NULL));
		break;
		
		/* PangoAttrColor */
	case PANGO_ATTR_FOREGROUND:
	case PANGO_ATTR_BACKGROUND: 
	case PANGO_ATTR_UNDERLINE_COLOR:
	case PANGO_ATTR_STRIKETHROUGH_COLOR:
		/* boxed colours */
		if (gdk_color_parse (strval, &color))
		{
			g_value_init (&(gattr->value), GDK_TYPE_COLOR);
			g_value_set_boxed (&(gattr->value), &color);
		}
		else
			g_critical ("Unable to parse color attribute '%s'", strval);

		break;
		
		/* PangoAttrShape */
	case PANGO_ATTR_SHAPE:
		/* Unsupported for now */
		break;
		
	case PANGO_ATTR_INVALID:
	case PANGO_ATTR_LETTER_SPACING:
	case PANGO_ATTR_RISE:
	case PANGO_ATTR_FALLBACK:
	default:
		break;
	}

	return gattr;
}

static void
sync_object (GladeEPropAttrs *eprop_attrs,
	     gboolean         use_command)
{
	GList          *attributes = NULL;
	GladeAttribute *gattr;
 	GtkTreeIter     iter;
	PangoAttrType   type;
	AttrEditType    edit_type;
	gchar          *strval = NULL;
	gboolean        valid;

	valid = gtk_tree_model_iter_children (eprop_attrs->model, &iter, NULL);

	while (valid)
	{

		if (!is_empty_row (eprop_attrs->model, &iter))
		{
			gtk_tree_model_get (eprop_attrs->model, &iter,
					    COLUMN_TYPE, &type,
					    COLUMN_EDIT_TYPE, &edit_type,
					    COLUMN_TEXT, &strval,
					    -1);
			
			g_print ("Feeding attributes with a type %d value '%s'\n", type, strval);


			gattr = glade_gtk_attribute_from_string (type, (edit_type == EDIT_TOGGLE) ? "" : strval);
			strval = (g_free (strval), NULL);

			attributes = g_list_prepend (attributes, gattr);

		}
		valid = gtk_tree_model_iter_next (eprop_attrs->model, &iter);
	} 

	/* XXX Probably leaks attributes list ?? XXX */
	if (use_command)
		glade_command_set_property (GLADE_EDITOR_PROPERTY (eprop_attrs)->property,
					    g_list_reverse (attributes));
	else
		glade_property_set (GLADE_EDITOR_PROPERTY (eprop_attrs)->property, 
				    g_list_reverse (attributes));
	
}

static void
value_text_editing_started (GtkCellRenderer *renderer,
			    GtkCellEditable *editable,
			    gchar           *path,
			    GladeEPropAttrs *eprop_attrs)
{
	GtkWidget       *dialog;
	GtkTreeIter      iter;
	PangoAttrType    type;
	AttrEditType     edit_type;
	GdkColor         color;
	gchar           *text = NULL, *new_text;

	/* stop emission */
	g_signal_stop_emission_by_name (G_OBJECT (renderer), "editing-started");

	/* Find type etc */
	if (!gtk_tree_model_get_iter_from_string (eprop_attrs->model, &iter, path))
		return;

	gtk_tree_model_get (eprop_attrs->model, &iter,
			    COLUMN_TEXT, &text,
			    COLUMN_TYPE, &type,
			    COLUMN_EDIT_TYPE, &edit_type,
			    -1);

	/* Launch dialog etc. */
	switch (edit_type) 
	{
	case EDIT_COLOR:
		dialog = gtk_color_selection_dialog_new (_("Select a color"));

		/* Get response etc... */
		if (text && gdk_color_parse (text, &color))
			gtk_color_selection_set_current_color 
				(GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel), &color);

		gtk_dialog_run (GTK_DIALOG (dialog));

		gtk_color_selection_get_current_color
			(GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel), &color);

		new_text = gdk_color_to_string (&color);

		g_print ("Setting color in store to '%s'\n", new_text);

		gtk_tree_store_set (GTK_TREE_STORE (eprop_attrs->model), &iter,
				    COLUMN_TEXT, new_text,
				    COLUMN_NAME_WEIGHT, PANGO_WEIGHT_BOLD,
				    COLUMN_TEXT_STYLE, PANGO_STYLE_NORMAL,
				    COLUMN_TEXT_FG, "Black",
				    -1);
		g_free (new_text);

		gtk_widget_destroy (dialog);
		break;
	default:
		break;
	}

	ensure_empty_row (GTK_TREE_STORE (eprop_attrs->model), edit_type, type);

	sync_object (eprop_attrs, FALSE);


	g_free (text);
}

static void
value_toggled (GtkCellRendererToggle *cell_renderer,
	       gchar                 *path,
	       GladeEPropAttrs       *eprop_attrs)
{
	gboolean         active;
	GtkTreeIter      iter;
	PangoAttrType    type;

	if (!gtk_tree_model_get_iter_from_string (eprop_attrs->model, &iter, path))
		return;

	gtk_tree_model_get (eprop_attrs->model, &iter,
			    COLUMN_TOGGLE_DOWN, &active,
			    COLUMN_TYPE, &type,
			    -1);

	gtk_tree_store_set (GTK_TREE_STORE (eprop_attrs->model), &iter,
			    COLUMN_NAME_WEIGHT, PANGO_WEIGHT_BOLD,
			    COLUMN_TOGGLE_DOWN, !active,
			    -1);

	ensure_empty_row (GTK_TREE_STORE (eprop_attrs->model), EDIT_TOGGLE, type);

	sync_object (eprop_attrs, FALSE);
}

static void
value_combo_spin_edited (GtkCellRendererText *cell,
			 const gchar         *path,
			 const gchar         *new_text,
			 GladeEPropAttrs     *eprop_attrs)
{
	GtkTreeIter      iter;
	PangoAttrType    type;

	if (!gtk_tree_model_get_iter_from_string (eprop_attrs->model, &iter, path))
		return;

	gtk_tree_model_get (eprop_attrs->model, &iter,
			    COLUMN_TYPE, &type,
			    -1);

	gtk_tree_store_set (GTK_TREE_STORE (eprop_attrs->model), &iter,
			    COLUMN_TEXT, new_text,
			    COLUMN_NAME_WEIGHT, PANGO_WEIGHT_BOLD,
			    COLUMN_TEXT_STYLE, PANGO_STYLE_NORMAL,
			    COLUMN_TEXT_FG, "Black",
			    -1);

	ensure_empty_row (GTK_TREE_STORE (eprop_attrs->model), 
			  (type == PANGO_ATTR_SIZE ||
			   type == PANGO_ATTR_ABSOLUTE_SIZE ||
			   type == PANGO_ATTR_SCALE) ? EDIT_SPIN : EDIT_COMBO, type);

	sync_object (eprop_attrs, FALSE);

}

static GtkWidget *
glade_eprop_attrs_view (GladeEditorProperty *eprop)
{
	GladeEPropAttrs   *eprop_attrs = GLADE_EPROP_ATTRS (eprop);
	GtkWidget         *view_widget;
 	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GtkAdjustment     *adjustment;

	eprop_attrs->model = (GtkTreeModel *)gtk_tree_store_new
		(NUM_COLUMNS,
		 /* Main Data */
		 G_TYPE_STRING,  // COLUMN_NAME
		 G_TYPE_INT,     // COLUMN_NAME_WEIGHT
		 G_TYPE_INT,     // COLUMN_TYPE
		 G_TYPE_INT,     // COLUMN_EDIT_TYPE
		 G_TYPE_POINTER, // COLUMN_VALUE
		 G_TYPE_UINT,    // COLUMN_START
		 G_TYPE_UINT,    // COLUMN_END
		 /* Editor renderer related */
		 G_TYPE_BOOLEAN, // COLUMN_TOGGLE_ACTIVE
		 G_TYPE_BOOLEAN, // COLUMN_TOGGLE_DOWN
		 G_TYPE_BOOLEAN, // COLUMN_TEXT_ACTIVE
		 G_TYPE_STRING,  // COLUMN_TEXT
		 G_TYPE_INT,     // COLUMN_TEXT_STYLE
		 G_TYPE_STRING,  // COLUMN_TEXT_FG
		 G_TYPE_BOOLEAN, // COLUMN_COMBO_ACTIVE
		 GTK_TYPE_TREE_MODEL, // COLUMN_COMBO_MODEL
		 G_TYPE_BOOLEAN, // COLUMN_SPIN_ACTIVE
		 G_TYPE_UINT);   // COLUMN_SPIN_DIGITS

	view_widget = gtk_tree_view_new_with_model (eprop_attrs->model);
	gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (view_widget), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (view_widget), FALSE);

	/********************* attribute name column *********************/
 	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "editable", FALSE, NULL);
	column = gtk_tree_view_column_new_with_attributes
		(_("Attribute"),  renderer, 
		 "text", COLUMN_NAME, 
		 "weight", COLUMN_NAME_WEIGHT,
		 NULL);

	gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
 	gtk_tree_view_append_column (GTK_TREE_VIEW (view_widget), column);

	/********************* attribute value column *********************/
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Value"));



	/* Toggle renderer */
 	renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (G_OBJECT (renderer), "activatable", TRUE, NULL);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer, 
					     "visible", COLUMN_TOGGLE_ACTIVE,
					     "active", COLUMN_TOGGLE_DOWN,
					     NULL);
	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (value_toggled), eprop);

	/* Text renderer */
 	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer, 
					     "visible", COLUMN_TEXT_ACTIVE,
					     "text", COLUMN_TEXT,
					     "style", COLUMN_TEXT_STYLE,
					     "foreground", COLUMN_TEXT_FG,
					     NULL);
	g_signal_connect (G_OBJECT (renderer), "editing-started",
			  G_CALLBACK (value_text_editing_started), eprop);

	/* Combo renderer */
 	renderer = gtk_cell_renderer_combo_new ();
	g_object_set (G_OBJECT (renderer), 
		      "editable", TRUE, 
		      "text-column", 0,
		      "has-entry", FALSE,
		      NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer, 
					     "visible", COLUMN_COMBO_ACTIVE,
					     "text", COLUMN_TEXT,
					     "style", COLUMN_TEXT_STYLE,
					     "foreground", COLUMN_TEXT_FG,
					     "model", COLUMN_COMBO_MODEL,
					     NULL);
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (value_combo_spin_edited), eprop);


	/* Spin renderer */
 	renderer = gtk_cell_renderer_spin_new ();
	adjustment = (GtkAdjustment *)gtk_adjustment_new (0, -G_MAXDOUBLE, G_MAXDOUBLE, 100, 100, 100);
	g_object_set (G_OBJECT (renderer), "editable", TRUE, "adjustment", adjustment, NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer, 
					     "visible", COLUMN_SPIN_ACTIVE,
					     "text", COLUMN_TEXT,
					     "style", COLUMN_TEXT_STYLE,
					     "foreground", COLUMN_TEXT_FG,
					     "digits", COLUMN_SPIN_DIGITS,
					     NULL);
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (value_combo_spin_edited), eprop);

	gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
 	gtk_tree_view_append_column (GTK_TREE_VIEW (view_widget), column);

	return view_widget;
}

static void
glade_eprop_attrs_populate_view (GladeEditorProperty *eprop, 
				 GtkTreeView         *view)
{
	GList              *attributes, *list;
	GtkTreeStore       *model = (GtkTreeStore *)gtk_tree_view_get_model (view);
	GtkTreeIter        *iter = NULL;
	GladeAttribute     *gattr;
	gchar              *text;

	attributes = g_value_get_boxed (eprop->property->value);

	append_empty_row (model, PANGO_ATTR_LANGUAGE, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_STYLE, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_WEIGHT, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_VARIANT, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_STRETCH, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_UNDERLINE, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_STRIKETHROUGH, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_GRAVITY, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_GRAVITY_HINT, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_FAMILY, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_SIZE, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_ABSOLUTE_SIZE, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_FOREGROUND, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_BACKGROUND, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_UNDERLINE_COLOR, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_STRIKETHROUGH_COLOR, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_SHAPE, NULL, NULL);
	append_empty_row (model, PANGO_ATTR_SCALE, NULL, NULL);


	/* XXX Populate here ...
	 */
	for (list = attributes; list; list = list->next)
	{
		gattr = list->data;

		if (append_empty_row (model, gattr->type, NULL, &iter))
		{
			text = glade_gtk_string_from_attr (gattr);

			gtk_tree_store_set (GTK_TREE_STORE (model), iter,
					    COLUMN_NAME_WEIGHT, PANGO_WEIGHT_BOLD,
					    COLUMN_TEXT, text,
					    COLUMN_TEXT_STYLE, PANGO_STYLE_NORMAL,
					    COLUMN_TEXT_FG, "Black",
					    -1);
			
			if (gattr->type == PANGO_ATTR_UNDERLINE ||
			    gattr->type == PANGO_ATTR_STRIKETHROUGH)
				gtk_tree_store_set (GTK_TREE_STORE (model), iter,
						    COLUMN_TOGGLE_DOWN, g_value_get_boolean (&(gattr->value)),
						    -1);


			g_free (text);
			gtk_tree_iter_free (iter);
		}

	}

}


static void
glade_eprop_attrs_show_dialog (GtkWidget           *dialog_button,
			       GladeEditorProperty *eprop)
{
	GladeEPropAttrs  *eprop_attrs = GLADE_EPROP_ATTRS (eprop);
	GtkWidget        *dialog, *parent, *vbox, *sw, *tree_view;
	GladeProject     *project;
	gint              res;
	
	project = glade_widget_get_project (eprop->property->widget);
	parent = gtk_widget_get_toplevel (GTK_WIDGET (eprop));

	dialog = gtk_dialog_new_with_buttons (_("Setup Text Attributes"),
					      GTK_WINDOW (parent),
					      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CLEAR, GLADE_RESPONSE_CLEAR,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK, GTK_RESPONSE_OK,
					      NULL);

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (sw);
	gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
	gtk_widget_set_size_request (sw, 400, 200);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);

	tree_view = glade_eprop_attrs_view (eprop);
	glade_eprop_attrs_populate_view (eprop, GTK_TREE_VIEW (tree_view));

	gtk_tree_view_expand_all (GTK_TREE_VIEW (tree_view));

	gtk_widget_show (tree_view);
	gtk_container_add (GTK_CONTAINER (sw), tree_view);

	/* Run the dialog */
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_OK) 
	{
/* 		gtk_tree_model_foreach */
/* 			(gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view)), */
/* 			 (GtkTreeModelForeachFunc) */
/* 			 glade_eprop_accel_accum_accelerators, &accelerators); */
		
/* 		value = g_new0 (GValue, 1); */
/* 		g_value_init (value, GLADE_TYPE_ACCEL_GLIST); */
/* 		g_value_take_boxed (value, accelerators); */

/* 		glade_editor_property_commit (eprop, value); */

/* 		g_value_unset (value); */
/* 		g_free (value); */
	} 
	else if (res == GLADE_RESPONSE_CLEAR)
	{
/* 		value = g_new0 (GValue, 1); */
/* 		g_value_init (value, GLADE_TYPE_ACCEL_GLIST); */
/* 		g_value_set_boxed (value, NULL); */

/* 		glade_editor_property_commit (eprop, value); */

/* 		g_value_unset (value); */
/* 		g_free (value); */
	}

	/* Clean up ...
	 */
	gtk_widget_destroy (dialog);

	g_object_unref (G_OBJECT (eprop_attrs->model));
	eprop_attrs->model = NULL;

}


static void
glade_eprop_attrs_finalize (GObject *object)
{
	/* Chain up */
	GObjectClass *parent_class = g_type_class_peek_parent (G_OBJECT_GET_CLASS (object));

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
glade_eprop_attrs_load (GladeEditorProperty *eprop, 
			GladeProperty       *property)
{
	GladeEditorPropertyClass *parent_class = 
		g_type_class_peek_parent (GLADE_EDITOR_PROPERTY_GET_CLASS (eprop));
	GladeEPropAttrs *eprop_attrs = GLADE_EPROP_ATTRS (eprop);

	/* Chain up first */
	parent_class->load (eprop, property);

	if (property == NULL) return;

}

static GtkWidget *
glade_eprop_attrs_create_input (GladeEditorProperty *eprop)
{
	GladeEPropAttrs *eprop_attrs = GLADE_EPROP_ATTRS (eprop);
	GtkWidget        *hbox;
	GtkWidget        *button;

	hbox               = gtk_hbox_new (FALSE, 0);
/* 	eprop_attrs->entry = gtk_entry_new (); */
/* 	gtk_entry_set_editable (GTK_ENTRY (eprop_attrs->entry), FALSE); */
/* 	gtk_widget_show (eprop_attrs->entry); */
/* 	gtk_box_pack_start (GTK_BOX (hbox), eprop_attrs->entry, TRUE, TRUE, 0); */

	button = gtk_button_new_with_label (_("Edit Attributes"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox), button,  TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (glade_eprop_attrs_show_dialog),
			  eprop);

	return hbox;
}