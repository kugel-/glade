/*
 * glade-gtk-button.c - GladeWidgetAdaptor for GtkButton classes
 *
 * Copyright (C) 2013 Tristan Van Berkom
 *
 * Authors:
 *      Tristan Van Berkom <tristan.van.berkom@gmail.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gladeui/glade.h>

#include "glade-button-editor.h"
#include "glade-scale-button-editor.h"
#include "glade-font-button-editor.h"
#include "glade-gtk.h"
#include "glade-gtk-button.h"

/* ----------------------------- GtkFontButton ------------------------------ */

/* Use the font-buttons launch dialog to actually set the font-name
 * glade property through the glade-command api.
 */
static void
glade_gtk_font_button_refresh_font_name (GtkFontButton * button,
                                         GladeWidget * gbutton)
{
  GladeProperty *property;

  if ((property = glade_widget_get_property (gbutton, "font-name")) != NULL)
    glade_command_set_property (property,
                                gtk_font_button_get_font_name (button));
}


/* ----------------------------- GtkColorButton ------------------------------ */
static void
glade_gtk_color_button_refresh_color (GtkColorButton * button,
                                      GladeWidget * gbutton)
{
  GladeProperty *property;
  GdkRGBA rgba = { 0, };

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &rgba);

  if ((property = glade_widget_get_property (gbutton, "color")) != NULL)
    {
      if (glade_property_get_enabled (property))
	{
	  GdkColor color = { 0, };

	  color.red   = (gint16) (rgba.red * 65535);
	  color.green = (gint16) (rgba.green * 65535);
	  color.blue  = (gint16) (rgba.blue * 65535);

	  glade_command_set_property (property, &color);
	}
    }

  if ((property = glade_widget_get_property (gbutton, "rgba")) != NULL)
    {
      if (glade_property_get_enabled (property))
	glade_command_set_property (property, &rgba);
    }
}

void
glade_gtk_color_button_set_property (GladeWidgetAdaptor * adaptor,
                                     GObject * object,
                                     const gchar * id, const GValue * value)
{
  GladeProperty *property;
  GladeWidget *gwidget = glade_widget_get_from_gobject (object);

  if (!strcmp (id, "color"))
    {
      if ((property = glade_widget_get_property (gwidget, "color")) != NULL &&
	  glade_property_get_enabled (property) && g_value_get_boxed (value))
	{
	  GdkColor *color = g_value_get_boxed (value);
	  GdkRGBA copy;

	  copy.red   = color->red   / 65535.0;
	  copy.green = color->green / 65535.0;
	  copy.blue  = color->blue  / 65535.0;
	  copy.alpha = 1.0;

	  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (object), &copy);
	}
    }
  else if (!strcmp (id, "rgba"))
    {
      if ((property = glade_widget_get_property (gwidget, "rgba")) != NULL &&
	  glade_property_get_enabled (property) && g_value_get_boxed (value))
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (object),
				    (GdkRGBA *) g_value_get_boxed (value));
    }
  else
    GWA_GET_CLASS (GTK_TYPE_BUTTON)->set_property (adaptor, object, id, value);
}

/* ----------------------------- GtkButton ------------------------------ */
GladeEditable *
glade_gtk_button_create_editable (GladeWidgetAdaptor * adaptor,
                                  GladeEditorPageType type)
{
  if (type == GLADE_PAGE_GENERAL)
    {
      GType type = glade_widget_adaptor_get_object_type (adaptor);

      if (g_type_is_a (type, GTK_TYPE_FONT_BUTTON))
	return (GladeEditable *) glade_font_button_editor_new ();
      else if (g_type_is_a (type, GTK_TYPE_SCALE_BUTTON))
	return (GladeEditable *) glade_scale_button_editor_new ();
      else
	return (GladeEditable *) glade_button_editor_new ();
    }

  return GWA_GET_CLASS (GTK_TYPE_CONTAINER)->create_editable (adaptor, type);
}

static void
glade_gtk_button_update_stock (GladeWidget *widget)
{
  gboolean use_stock;
  gchar *label = NULL;
  
  /* Update the stock property */
  glade_widget_property_get (widget, "use-stock", &use_stock);
  if (use_stock)
    {
      glade_widget_property_get (widget, "label", &label);
      glade_widget_property_set (widget, "stock", label);
    }
}

void
glade_gtk_button_post_create (GladeWidgetAdaptor * adaptor,
                              GObject * button, GladeCreateReason reason)
{
  GladeWidget *gbutton = glade_widget_get_from_gobject (button);

  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (GLADE_IS_WIDGET (gbutton));

  if (GTK_IS_FONT_BUTTON (button))
    g_signal_connect
        (button, "font-set",
         G_CALLBACK (glade_gtk_font_button_refresh_font_name), gbutton);
  else if (GTK_IS_COLOR_BUTTON (button))
    g_signal_connect
        (button, "color-set",
         G_CALLBACK (glade_gtk_color_button_refresh_color), gbutton);

  /* Disabled response-id until its in an action area */
  glade_widget_property_set_sensitive (gbutton, "response-id", FALSE,
                                       RESPID_INSENSITIVE_MSG);

  if (reason == GLADE_CREATE_USER)
    glade_gtk_button_update_stock (gbutton);
}

void
glade_gtk_button_set_property (GladeWidgetAdaptor * adaptor,
                               GObject * object,
                               const gchar * id, const GValue * value)
{
  GladeWidget *widget = glade_widget_get_from_gobject (object);
  GladeProperty *property = glade_widget_get_property (widget, id);

  if (strcmp (id, "custom-child") == 0)
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (object));
      gboolean custom_child = g_value_get_boolean (value);

      /* Avoid removing a child if we already have a custom child */
      if (custom_child && (child && glade_widget_get_from_gobject (child)))
        return;

      if (custom_child)
        {
          if (child)
            gtk_container_remove (GTK_CONTAINER (object), child);

          gtk_container_add (GTK_CONTAINER (object), glade_placeholder_new ());
        }
      else if (child && GLADE_IS_PLACEHOLDER (child))
        gtk_container_remove (GTK_CONTAINER (object), child);
    }
  else if (strcmp (id, "stock") == 0)
    {
      gboolean use_stock = FALSE;
      glade_widget_property_get (widget, "use-stock", &use_stock);

      if (use_stock)
        gtk_button_set_label (GTK_BUTTON (object), g_value_get_string (value));
    }
  else if (strcmp (id, "use-stock") == 0)
    {
      /* I guess its my bug in GTK+, we need to resync the appearance property
       * on GtkButton when the GtkButton:use-stock property changes.
       */
      GWA_GET_CLASS (GTK_TYPE_CONTAINER)->set_property (adaptor, object,
                                                        id, value);
      glade_gtk_sync_use_appearance (widget);
    }
  else if (GPC_VERSION_CHECK (glade_property_get_class (property), gtk_major_version, gtk_minor_version + 1))
    GWA_GET_CLASS (GTK_TYPE_CONTAINER)->set_property (adaptor, object, id, value);
}

void
glade_gtk_button_read_widget (GladeWidgetAdaptor * adaptor,
                              GladeWidget * widget, GladeXmlNode * node)
{
  GObject *object;

  if (!(glade_xml_node_verify_silent (node, GLADE_XML_TAG_WIDGET) ||
	glade_xml_node_verify_silent (node, GLADE_XML_TAG_TEMPLATE)))
    return;

  /* First chain up and read in all the normal properties.. */
  GWA_GET_CLASS (GTK_TYPE_CONTAINER)->read_widget (adaptor, widget, node);

  glade_gtk_button_update_stock (widget);

  /* Fold "font-name" property into the "font" propery */
  object = glade_widget_get_object (widget);
  if (GTK_IS_FONT_BUTTON (object))
    {
      gchar *font_prop_value = NULL;

      glade_widget_property_get (widget, "font-name", &font_prop_value);

      if (font_prop_value != NULL)
	{
	  glade_widget_property_set (widget, "font", font_prop_value);
	  glade_widget_property_set (widget, "font-name", NULL);
	}
    }
}

void
glade_gtk_button_write_widget (GladeWidgetAdaptor * adaptor,
                               GladeWidget * widget,
                               GladeXmlContext * context, GladeXmlNode * node)
{
  GladeProperty *prop;
  gboolean use_stock;
  gchar *stock = NULL;

  if (!(glade_xml_node_verify_silent (node, GLADE_XML_TAG_WIDGET) ||
	glade_xml_node_verify_silent (node, GLADE_XML_TAG_TEMPLATE)))
    return;

  /* Do not save GtkColorButton GtkFontButton and GtkScaleButton label property */
  if (!(GTK_IS_COLOR_BUTTON (glade_widget_get_object (widget)) ||
	GTK_IS_FONT_BUTTON (glade_widget_get_object (widget)) ||
	GTK_IS_SCALE_BUTTON (glade_widget_get_object (widget))))
    {
      /* Make a copy of the GladeProperty, 
       * override its value and ensure non-translatable if use-stock is TRUE
       */
      prop = glade_widget_get_property (widget, "label");
      prop = glade_property_dup (prop, widget);
      glade_widget_property_get (widget, "use-stock", &use_stock);
      if (use_stock)
        {
          glade_widget_property_get (widget, "stock", &stock);
          glade_property_i18n_set_translatable (prop, FALSE);
          glade_property_set (prop, stock);
        }
      glade_property_write (prop, context, node);
      g_object_unref (G_OBJECT (prop));
    }

  /* Write out other normal properties and any other class derived custom properties after ... */
  GWA_GET_CLASS (GTK_TYPE_CONTAINER)->write_widget (adaptor, widget, context,
                                                    node);

}

/* Shared with other classes */
void 
glade_gtk_sync_use_appearance (GladeWidget *gwidget)
{
  GladeProperty *prop;
  gboolean       use_appearance;

  /* This is the kind of thing we avoid doing at project load time ;-) */
  if (glade_widget_superuser ())
    return;

  prop = glade_widget_get_property (gwidget, "use-action-appearance");
  use_appearance = FALSE;
  
  glade_property_get (prop, &use_appearance);
  if (use_appearance)
    {
      glade_property_set (prop, FALSE);
      glade_property_set (prop, TRUE);
    }
}
