/*
 * glade-gtk-grid.c - GladeWidgetAdaptor for GtkGrid widget
 *
 * Copyright (C) 2011 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
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

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "glade-fixed.h"

typedef struct
{
  gint left_attach;
  gint top_attach;
  gint width;
  gint height;
} GladeGridAttachments;

typedef struct
{
  /* comparable part: */
  GladeWidget *widget;
  gint left_attach;
  gint top_attach;
  gint width;
  gint height;
} GladeGridChild;

typedef enum
{
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
} GladeGridDir;

static GladeGridChild grid_edit = { 0, };
static GladeGridChild grid_cur_attach = { 0, };

static void
glade_gtk_grid_get_child_attachments (GtkWidget            *grid,
				      GtkWidget            *child,
				      GladeGridAttachments *grid_child)
{
  gtk_container_child_get (GTK_CONTAINER (grid), child,
                           "left-attach", &grid_child->left_attach,
                           "width",       &grid_child->width,
                           "top-attach",  &grid_child->top_attach,
                           "height",      &grid_child->height,
			   NULL);
}

/* Takes a point (x or y depending on 'row') relative to
 * grid, and returns the row or column in which the point
 * was found.
 */
static gint
glade_gtk_grid_get_row_col_from_point (GtkGrid * grid,
				       gboolean row, gint point)
{
  GladeGridAttachments attach;
  GtkAllocation allocation;
  GList *list, *children;
  gint span, trans_point, size, base, end;

  children = gtk_container_get_children (GTK_CONTAINER (grid));

  for (list = children; list; list = list->next)
    {
      GtkWidget *widget = list->data;

      glade_gtk_grid_get_child_attachments (GTK_WIDGET (grid), widget, &attach);

      if (row)
        gtk_widget_translate_coordinates (GTK_WIDGET (grid), widget, 0, point, NULL, &trans_point);
      else
        gtk_widget_translate_coordinates (GTK_WIDGET (grid), widget, point, 0, &trans_point, NULL);

      gtk_widget_get_allocation (widget, &allocation);

      /* Find any widget in our row/column
       */
      end = row ? allocation.height : allocation.width;

      if (trans_point >= 0 &&
          /* should be trans_point < end ... test FIXME ! */
          trans_point < end)
        {
          base = row ? attach.top_attach : attach.left_attach;
          size = row ? allocation.height : allocation.width;
          span = row ? attach.height     : attach.width;

          return base + (trans_point * span / size);
        }
    }

  g_list_free (children);

  return -1;
}


static gboolean
glade_gtk_grid_point_crosses_threshold (GtkGrid     *grid,
					gboolean     row,
					gint         num,
					GladeGridDir dir, 
					gint         point)
{
  GladeGridAttachments attach;
  GtkAllocation allocation;
  GList *list, *children;
  gint span, trans_point, size, rowcol_size, base;

  children = gtk_container_get_children (GTK_CONTAINER (grid));

  for (list = children; list; list = list->next)
    {
      GtkWidget *widget = list->data;

      glade_gtk_grid_get_child_attachments (GTK_WIDGET (grid), widget, &attach);

      /* Find any widget in our row/column
       */
      if ((row  && num >= attach.top_attach  && num < attach.top_attach  + attach.height) ||
          (!row && num >= attach.left_attach && num < attach.left_attach + attach.width))
        {

          if (row)
            gtk_widget_translate_coordinates (GTK_WIDGET (grid), widget, 0, point, NULL, &trans_point);
          else
            gtk_widget_translate_coordinates (GTK_WIDGET (grid), widget, point, 0, &trans_point, NULL);

          span = row ? attach.height : attach.width;
          gtk_widget_get_allocation (widget, &allocation);
          size = row ? allocation.height : allocation.width;

          base = row ? attach.top_attach : attach.left_attach;
          rowcol_size = size / span;
          trans_point -= (num - base) * rowcol_size;

#if 0
          g_print ("dir: %s, widget size: %d, rowcol size: %d, "
                   "requested rowcol: %d, widget base rowcol: %d, trim: %d, "
                   "widget point: %d, thresh: %d\n",
                   dir == DIR_UP ? "up" : dir == DIR_DOWN ? "down" :
                   dir == DIR_LEFT ? "left" : "right",
                   size, rowcol_size, num, base, (num - base) * rowcol_size,
                   trans_point,
                   dir == DIR_UP || dir == DIR_LEFT ?
                   (rowcol_size / 2) : (rowcol_size / 2));
#endif
          switch (dir)
            {
              case DIR_UP:
              case DIR_LEFT:
                return trans_point <= (rowcol_size / 2);
              case DIR_DOWN:
              case DIR_RIGHT:
                return trans_point >= (rowcol_size / 2);
              default:
                break;
            }
        }

    }

  g_list_free (children);

  return FALSE;
}

static gboolean
glade_gtk_grid_get_attachments (GladeFixed        *fixed,
				GtkGrid           *grid,
				GdkRectangle      *rect,
				GladeGridChild    *configure)
{
  GladeWidget *widget = GLADE_WIDGET (fixed);
  gint center_x, center_y, row, column;
  guint n_columns, n_rows;

  center_x = rect->x + (rect->width / 2);
  center_y = rect->y + (rect->height / 2);

  column = glade_gtk_grid_get_row_col_from_point (grid, FALSE, center_x);
  row    = glade_gtk_grid_get_row_col_from_point (grid, TRUE, center_y);

  /* its a start, now try to grow when the rect extents
   * reach at least half way into the next row/column 
   */
  configure->left_attach   = column;
  configure->width         = 1;
  configure->top_attach    = row;
  configure->height        = 1;

  glade_widget_property_get (widget, "n-columns", &n_columns);
  glade_widget_property_get (widget, "n-rows", &n_rows);

  if (column >= 0 && row >= 0)
    {
      /* Check and expand left
       */
      while (configure->left_attach > 0)
        {
          if (rect->x < fixed->child_x_origin &&
              fixed->operation != GLADE_CURSOR_DRAG &&
              GLADE_FIXED_CURSOR_LEFT (fixed->operation) == FALSE)
            break;

          if (glade_gtk_grid_point_crosses_threshold
              (grid, FALSE, configure->left_attach - 1,
               DIR_LEFT, rect->x) == FALSE)
            break;

          configure->left_attach--;
        }

      /* Check and expand right
       */
      while (configure->left_attach + configure->width < n_columns)
        {
          if (rect->x + rect->width >
              fixed->child_x_origin + fixed->child_width_origin &&
              fixed->operation != GLADE_CURSOR_DRAG &&
              GLADE_FIXED_CURSOR_RIGHT (fixed->operation) == FALSE)
            break;

          if (glade_gtk_grid_point_crosses_threshold
              (grid, FALSE, configure->left_attach + configure->width,
               DIR_RIGHT, rect->x + rect->width) == FALSE)
            break;

          configure->width++;
        }

      /* Check and expand top
       */
      while (configure->top_attach > 0)
        {
          if (rect->y < fixed->child_y_origin &&
              fixed->operation != GLADE_CURSOR_DRAG &&
              GLADE_FIXED_CURSOR_TOP (fixed->operation) == FALSE)
            break;

          if (glade_gtk_grid_point_crosses_threshold
              (grid, TRUE, configure->top_attach - 1,
               DIR_UP, rect->y) == FALSE)
            break;

          configure->top_attach--;
        }

      /* Check and expand bottom
       */
      while (configure->top_attach + configure->height < n_rows)
        {
          if (rect->y + rect->height >
              fixed->child_y_origin + fixed->child_height_origin &&
              fixed->operation != GLADE_CURSOR_DRAG &&
              GLADE_FIXED_CURSOR_BOTTOM (fixed->operation) == FALSE)
            break;

          if (glade_gtk_grid_point_crosses_threshold
              (grid, TRUE, configure->top_attach + configure->height,
               DIR_DOWN, rect->y + rect->height) == FALSE)
            break;

          configure->height++;
        }
    }

  return column >= 0 && row >= 0;
}

static gboolean
glade_gtk_grid_configure_child (GladeFixed * fixed,
                                 GladeWidget * child,
                                 GdkRectangle * rect, GtkWidget * grid)
{
  GladeGridChild configure = { child, };

  /* Sometimes we are unable to find a widget in the appropriate column,
   * usually because a placeholder hasnt had its size allocation yet.
   */
  if (glade_gtk_grid_get_attachments (fixed, GTK_GRID (grid), rect, &configure))
    {
      if (memcmp (&configure, &grid_cur_attach, sizeof (GladeGridChild)) != 0)
        {

          glade_property_push_superuser ();
          glade_widget_pack_property_set (child, "left-attach",
                                          configure.left_attach);
          glade_widget_pack_property_set (child, "width",
                                          configure.width);
          glade_widget_pack_property_set (child, "top-attach",
                                          configure.top_attach);
          glade_widget_pack_property_set (child, "height",
                                          configure.height);
          glade_property_pop_superuser ();

          memcpy (&grid_cur_attach, &configure, sizeof (GladeGridChild));
        }
    }
  return TRUE;
}


static gboolean
glade_gtk_grid_configure_begin (GladeFixed * fixed,
                                 GladeWidget * child, GtkWidget * grid)
{
  grid_edit.widget = child;

  glade_widget_pack_property_get (child, "left-attach", &grid_edit.left_attach);
  glade_widget_pack_property_get (child, "width",       &grid_edit.width);
  glade_widget_pack_property_get (child, "top-attach",  &grid_edit.top_attach);
  glade_widget_pack_property_get (child, "height",      &grid_edit.height);

  memcpy (&grid_cur_attach, &grid_edit, sizeof (GladeGridChild));

  return TRUE;
}

static gboolean
glade_gtk_grid_configure_end (GladeFixed * fixed,
                               GladeWidget * child, GtkWidget * grid)
{
  GladeGridChild new_child = { child, };

  glade_widget_pack_property_get (child, "left-attach", &new_child.left_attach);
  glade_widget_pack_property_get (child, "width",       &new_child.width);
  glade_widget_pack_property_get (child, "top-attach",  &new_child.top_attach);
  glade_widget_pack_property_get (child, "height",      &new_child.height);

  /* Compare the meaningfull part of the current edit. */
  if (memcmp (&new_child, &grid_edit, sizeof (GladeGridChild)) != 0)
    {
      GValue left_attach_value = { 0, };
      GValue width_attach_value = { 0, };
      GValue top_attach_value = { 0, };
      GValue height_attach_value = { 0, };

      GValue new_left_attach_value = { 0, };
      GValue new_width_attach_value = { 0, };
      GValue new_top_attach_value = { 0, };
      GValue new_height_attach_value = { 0, };

      GladeProperty *left_attach_prop, *width_attach_prop,
          *top_attach_prop, *height_attach_prop;

      left_attach_prop   = glade_widget_get_pack_property (child, "left-attach");
      width_attach_prop  = glade_widget_get_pack_property (child, "width");
      top_attach_prop    = glade_widget_get_pack_property (child, "top-attach");
      height_attach_prop = glade_widget_get_pack_property (child, "height");

      g_return_val_if_fail (GLADE_IS_PROPERTY (left_attach_prop), FALSE);
      g_return_val_if_fail (GLADE_IS_PROPERTY (width_attach_prop), FALSE);
      g_return_val_if_fail (GLADE_IS_PROPERTY (top_attach_prop), FALSE);
      g_return_val_if_fail (GLADE_IS_PROPERTY (height_attach_prop), FALSE);

      glade_property_get_value (left_attach_prop, &new_left_attach_value);
      glade_property_get_value (width_attach_prop, &new_width_attach_value);
      glade_property_get_value (top_attach_prop, &new_top_attach_value);
      glade_property_get_value (height_attach_prop, &new_height_attach_value);

      g_value_init (&left_attach_value, G_TYPE_UINT);
      g_value_init (&width_attach_value, G_TYPE_UINT);
      g_value_init (&top_attach_value, G_TYPE_UINT);
      g_value_init (&height_attach_value, G_TYPE_UINT);

      g_value_set_uint (&left_attach_value, grid_edit.left_attach);
      g_value_set_uint (&width_attach_value, grid_edit.width);
      g_value_set_uint (&top_attach_value, grid_edit.top_attach);
      g_value_set_uint (&height_attach_value, grid_edit.height);

      glade_command_push_group (_("Placing %s inside %s"),
                                glade_widget_get_name (child), 
				glade_widget_get_name (GLADE_WIDGET (fixed)));
      glade_command_set_properties
          (left_attach_prop, &left_attach_value, &new_left_attach_value,
           width_attach_prop, &width_attach_value, &new_width_attach_value,
           top_attach_prop, &top_attach_value, &new_top_attach_value,
           height_attach_prop, &height_attach_value, &new_height_attach_value,
           NULL);
      glade_command_pop_group ();

      g_value_unset (&left_attach_value);
      g_value_unset (&width_attach_value);
      g_value_unset (&top_attach_value);
      g_value_unset (&height_attach_value);
      g_value_unset (&new_left_attach_value);
      g_value_unset (&new_width_attach_value);
      g_value_unset (&new_top_attach_value);
      g_value_unset (&new_height_attach_value);
    }

  return TRUE;
}

void
glade_gtk_grid_post_create (GladeWidgetAdaptor * adaptor,
                             GObject * container, GladeCreateReason reason)
{
  GladeWidget *gwidget = glade_widget_get_from_gobject (container);

  g_signal_connect (G_OBJECT (gwidget), "configure-child",
                    G_CALLBACK (glade_gtk_grid_configure_child), container);

  g_signal_connect (G_OBJECT (gwidget), "configure-begin",
                    G_CALLBACK (glade_gtk_grid_configure_begin), container);

  g_signal_connect (G_OBJECT (gwidget), "configure-end",
                    G_CALLBACK (glade_gtk_grid_configure_end), container);
}

static gboolean
glade_gtk_grid_has_child (GtkGrid * grid, guint left_attach,
                           guint top_attach)
{
  GList *list, *children;
  gboolean ret = FALSE;

  children = gtk_container_get_children (GTK_CONTAINER (grid));

  for (list = children; list && list->data; list = list->next)
    {
      GladeGridAttachments attach;
      GtkWidget *widget = list->data;

      glade_gtk_grid_get_child_attachments (GTK_WIDGET (grid), widget, &attach);

      if (left_attach >= attach.left_attach && left_attach < attach.left_attach + attach.width && 
	  top_attach  >= attach.top_attach  && top_attach  < attach.top_attach  + attach.height)
        {
          ret = TRUE;
          break;
        }
    }

  g_list_free (children);

  return ret;
}

static gboolean
glade_gtk_grid_widget_exceeds_bounds (GtkGrid * grid, gint n_rows,
                                       gint n_cols)
{
  GList *list, *children;
  gboolean ret = FALSE;

  children = gtk_container_get_children (GTK_CONTAINER (grid));

  for (list = children; list && list->data; list = list->next)
    {
      GladeGridAttachments attach;
      GtkWidget *widget = list->data;

      glade_gtk_grid_get_child_attachments (GTK_WIDGET (grid), widget, &attach);

      if (GLADE_IS_PLACEHOLDER (widget) == FALSE &&
          (attach.left_attach + attach.width  > n_cols || 
	   attach.top_attach  + attach.height > n_rows))
        {
          ret = TRUE;
          break;
        }
    }

  g_list_free (children);

  return ret;
}

static void
glade_gtk_grid_refresh_placeholders (GtkGrid * grid)
{
  GladeWidget *widget;
  GList *list, *children;
  guint n_columns, n_rows;
  gint i, j;

  widget = glade_widget_get_from_gobject (GTK_WIDGET (grid));
  glade_widget_property_get (widget, "n-columns", &n_columns);
  glade_widget_property_get (widget, "n-rows", &n_rows);

  children = gtk_container_get_children (GTK_CONTAINER (grid));

  for (list = children; list && list->data; list = list->next)
    {
      if (GLADE_IS_PLACEHOLDER (list->data))
        gtk_container_remove (GTK_CONTAINER (grid), GTK_WIDGET (list->data));
    }
  g_list_free (children);

  for (i = 0; i < n_columns; i++)
    for (j = 0; j < n_rows; j++)
      if (glade_gtk_grid_has_child (grid, i, j) == FALSE)
        {
          gtk_grid_attach (grid, glade_placeholder_new (),
			   i, j, 1, 1);
        }
  gtk_container_check_resize (GTK_CONTAINER (grid));
}

static void
gtk_grid_children_callback (GtkWidget * widget, gpointer client_data)
{
  GList **children;

  children = (GList **) client_data;
  *children = g_list_prepend (*children, widget);
}

GList *
glade_gtk_grid_get_children (GladeWidgetAdaptor * adaptor,
                              GtkContainer * container)
{
  GList *children = NULL;

  g_return_val_if_fail (GTK_IS_GRID (container), NULL);

  gtk_container_forall (container, gtk_grid_children_callback, &children);

  /* GtkGrid has the children list already reversed */
  return children;
}

void
glade_gtk_grid_add_child (GladeWidgetAdaptor * adaptor,
                           GObject * object, GObject * child)
{
  g_return_if_fail (GTK_IS_GRID (object));
  g_return_if_fail (GTK_IS_WIDGET (child));

  gtk_container_add (GTK_CONTAINER (object), GTK_WIDGET (child));

  glade_gtk_grid_refresh_placeholders (GTK_GRID (object));
}

void
glade_gtk_grid_remove_child (GladeWidgetAdaptor * adaptor,
                              GObject * object, GObject * child)
{
  g_return_if_fail (GTK_IS_GRID (object));
  g_return_if_fail (GTK_IS_WIDGET (child));

  gtk_container_remove (GTK_CONTAINER (object), GTK_WIDGET (child));

  glade_gtk_grid_refresh_placeholders (GTK_GRID (object));
}

void
glade_gtk_grid_replace_child (GladeWidgetAdaptor * adaptor,
                               GtkWidget * container,
                               GtkWidget * current, GtkWidget * new_widget)
{
  g_return_if_fail (GTK_IS_GRID (container));
  g_return_if_fail (GTK_IS_WIDGET (current));
  g_return_if_fail (GTK_IS_WIDGET (new_widget));

  /* Chain Up */
  GWA_GET_CLASS
      (GTK_TYPE_CONTAINER)->replace_child (adaptor,
                                           G_OBJECT (container),
                                           G_OBJECT (current),
                                           G_OBJECT (new_widget));

  /* If we are replacing a GladeWidget, we must refresh placeholders
   * because the widget may have spanned multiple rows/columns, we must
   * not do so in the case we are pasting multiple widgets into a grid,
   * where destroying placeholders results in default packing properties
   * (since the remaining placeholder templates no longer exist, only the
   * first pasted widget would have proper packing properties).
   */
  if (glade_widget_get_from_gobject (new_widget) == NULL)
    glade_gtk_grid_refresh_placeholders (GTK_GRID (container));

}

static void
glade_gtk_grid_set_n_common (GObject * object, const GValue * value,
                              gboolean for_rows)
{
  GladeWidget *widget;
  GtkGrid *grid;
  guint new_size, n_columns, n_rows;

  grid = GTK_GRID (object);
  widget = glade_widget_get_from_gobject (GTK_WIDGET (grid));

  glade_widget_property_get (widget, "n-columns", &n_columns);
  glade_widget_property_get (widget, "n-rows", &n_rows);

  new_size = g_value_get_uint (value);

  if (new_size < 1)
    return;

  if (glade_gtk_grid_widget_exceeds_bounds
      (grid, for_rows ? new_size : n_rows, for_rows ? n_columns : new_size))
    /* Refuse to shrink if it means orphaning widgets */
    return;

  /* Fill grid with placeholders */
  glade_gtk_grid_refresh_placeholders (grid);
}

void
glade_gtk_grid_set_property (GladeWidgetAdaptor * adaptor,
                              GObject * object,
                              const gchar * id, const GValue * value)
{
  if (!strcmp (id, "n-rows"))
    glade_gtk_grid_set_n_common (object, value, TRUE);
  else if (!strcmp (id, "n-columns"))
    glade_gtk_grid_set_n_common (object, value, FALSE);
  else
    GWA_GET_CLASS (GTK_TYPE_CONTAINER)->set_property (adaptor, object,
                                                      id, value);
}

static gboolean
glade_gtk_grid_verify_n_common (GObject * object, const GValue * value,
                                 gboolean for_rows)
{
  GtkGrid *grid = GTK_GRID (object);
  GladeWidget *widget;
  guint n_columns, n_rows, new_size = g_value_get_uint (value);

  widget = glade_widget_get_from_gobject (GTK_WIDGET (grid));
  glade_widget_property_get (widget, "n-columns", &n_columns);
  glade_widget_property_get (widget, "n-rows", &n_rows);

  if (glade_gtk_grid_widget_exceeds_bounds
      (grid, for_rows ? new_size : n_rows, for_rows ? n_columns : new_size))
    /* Refuse to shrink if it means orphaning widgets */
    return FALSE;

  return TRUE;
}

gboolean
glade_gtk_grid_verify_property (GladeWidgetAdaptor * adaptor,
                                 GObject * object,
                                 const gchar * id, const GValue * value)
{
  if (!strcmp (id, "n-rows"))
    return glade_gtk_grid_verify_n_common (object, value, TRUE);
  else if (!strcmp (id, "n-columns"))
    return glade_gtk_grid_verify_n_common (object, value, FALSE);
  else if (GWA_GET_CLASS (GTK_TYPE_CONTAINER)->verify_property)
    GWA_GET_CLASS (GTK_TYPE_CONTAINER)->verify_property (adaptor, object,
                                                         id, value);

  return TRUE;
}

void
glade_gtk_grid_set_child_property (GladeWidgetAdaptor * adaptor,
                                    GObject * container,
                                    GObject * child,
                                    const gchar * property_name, GValue * value)
{
  g_return_if_fail (GTK_IS_GRID (container));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (property_name != NULL && value != NULL);

  GWA_GET_CLASS
      (GTK_TYPE_CONTAINER)->child_set_property (adaptor,
                                                container, child,
                                                property_name, value);

  if (strcmp (property_name, "bottom-attach") == 0 ||
      strcmp (property_name, "left-attach") == 0 ||
      strcmp (property_name, "right-attach") == 0 ||
      strcmp (property_name, "top-attach") == 0)
    {
      /* Refresh placeholders */
      glade_gtk_grid_refresh_placeholders (GTK_GRID (container));
    }

}

static gboolean
glade_gtk_grid_verify_attach_common (GObject     *object,
				     GValue      *value,
				     gint        *val,
				     const gchar *prop,
				     gint        *prop_val,
				     const gchar *parent_prop,
				     guint       *parent_val)
{
  GladeWidget *widget, *parent;

  widget = glade_widget_get_from_gobject (object);
  g_return_val_if_fail (GLADE_IS_WIDGET (widget), TRUE);
  parent = glade_widget_get_parent (widget);
  g_return_val_if_fail (GLADE_IS_WIDGET (parent), TRUE);

  *val = g_value_get_int (value);
  glade_widget_property_get (widget, prop, prop_val);
  glade_widget_property_get (parent, parent_prop, parent_val);

  return FALSE;
}

static gboolean
glade_gtk_grid_verify_left_top_attach (GObject * object,
                                        GValue * value,
                                        const gchar * prop,
                                        const gchar * parent_prop)
{
  guint parent_val;
  gint val, prop_val;

  if (glade_gtk_grid_verify_attach_common (object, value, &val,
                                            prop, &prop_val,
                                            parent_prop, &parent_val))
    return FALSE;

  if (val >= parent_val || val >= prop_val)
    return FALSE;

  return TRUE;
}

static gboolean
glade_gtk_grid_verify_right_bottom_attach (GObject * object,
                                            GValue * value,
                                            const gchar * prop,
                                            const gchar * parent_prop)
{
  guint parent_val;
  gint val, prop_val;

  if (glade_gtk_grid_verify_attach_common (object, value, &val,
                                            prop, &prop_val,
                                            parent_prop, &parent_val))
    return FALSE;

  if (val <= prop_val || val > parent_val)
    return FALSE;

  return TRUE;
}

gboolean
glade_gtk_grid_child_verify_property (GladeWidgetAdaptor * adaptor,
                                       GObject * container,
                                       GObject * child,
                                       const gchar * id, GValue * value)
{
  if (!strcmp (id, "left-attach"))
    return glade_gtk_grid_verify_left_top_attach (child,
                                                   value,
                                                   "right-attach", "n-columns");
  else if (!strcmp (id, "right-attach"))
    return glade_gtk_grid_verify_right_bottom_attach (child,
                                                       value,
                                                       "left-attach",
                                                       "n-columns");
  else if (!strcmp (id, "top-attach"))
    return glade_gtk_grid_verify_left_top_attach (child,
                                                   value,
                                                   "bottom-attach", "n-rows");
  else if (!strcmp (id, "bottom-attach"))
    return glade_gtk_grid_verify_right_bottom_attach (child,
                                                       value,
                                                       "top-attach", "n-rows");
  else if (GWA_GET_CLASS (GTK_TYPE_CONTAINER)->child_verify_property)
    GWA_GET_CLASS
        (GTK_TYPE_CONTAINER)->child_verify_property (adaptor,
                                                     container, child,
                                                     id, value);

  return TRUE;
}

static void
glade_gtk_grid_child_insert_remove_action (GladeWidgetAdaptor *adaptor, 
					    GObject            *container, 
					    GObject            *object, 
					    const gchar        *group_format, 
					    const gchar        *n_row_col, 
					    const gchar        *attach1,    /* should be smaller (top/left) attachment */
                                            const gchar        *attach2,      /* should be larger (bot/right) attachment */
                                            gboolean            remove, 
					    gboolean            after)
{
  GladeWidget *parent;
  GList *children, *l;
  gint child_pos, size, offset;

  gtk_container_child_get (GTK_CONTAINER (container),
                           GTK_WIDGET (object),
                           after ? attach2 : attach1, &child_pos, NULL);

  parent = glade_widget_get_from_gobject (container);
  glade_command_push_group (group_format, glade_widget_get_name (parent));

  children = glade_widget_adaptor_get_children (adaptor, container);
  /* Make sure widgets does not get destroyed */
  g_list_foreach (children, (GFunc) g_object_ref, NULL);

  glade_widget_property_get (parent, n_row_col, &size);

  if (remove)
    {
      GList *del = NULL;
      /* Remove children first */
      for (l = children; l; l = g_list_next (l))
        {
          GladeWidget *gchild = glade_widget_get_from_gobject (l->data);
          gint pos1, pos2;

          /* Skip placeholders */
          if (gchild == NULL)
            continue;

          glade_widget_pack_property_get (gchild, attach1, &pos1);
          glade_widget_pack_property_get (gchild, attach2, &pos2);
          if ((pos1 + 1 == pos2) && ((after ? pos2 : pos1) == child_pos))
            {
              del = g_list_prepend (del, gchild);
            }
        }
      if (del)
        {
          glade_command_delete (del);
          g_list_free (del);
        }
      offset = -1;
    }
  else
    {
      /* Expand the grid */
      glade_command_set_property (glade_widget_get_property (parent, n_row_col),
                                  size + 1);
      offset = 1;
    }

  /* Reorder children */
  for (l = children; l; l = g_list_next (l))
    {
      GladeWidget *gchild = glade_widget_get_from_gobject (l->data);
      gint pos;

      /* Skip placeholders */
      if (gchild == NULL)
        continue;

      /* if removing, do top/left before bot/right */
      if (remove)
        {
          /* adjust top-left attachment */
          glade_widget_pack_property_get (gchild, attach1, &pos);
          if (pos > child_pos || (after && pos == child_pos))
            {
              glade_command_set_property (glade_widget_get_pack_property
                                          (gchild, attach1), pos + offset);
            }

          /* adjust bottom-right attachment */
          glade_widget_pack_property_get (gchild, attach2, &pos);
          if (pos > child_pos || (after && pos == child_pos))
            {
              glade_command_set_property (glade_widget_get_pack_property
                                          (gchild, attach2), pos + offset);
            }

        }
      /* if inserting, do bot/right before top/left */
      else
        {
          /* adjust bottom-right attachment */
          glade_widget_pack_property_get (gchild, attach2, &pos);
          if (pos > child_pos)
            {
              glade_command_set_property (glade_widget_get_pack_property
                                          (gchild, attach2), pos + offset);
            }

          /* adjust top-left attachment */
          glade_widget_pack_property_get (gchild, attach1, &pos);
          if (pos >= child_pos)
            {
              glade_command_set_property (glade_widget_get_pack_property
                                          (gchild, attach1), pos + offset);
            }
        }
    }

  if (remove)
    {
      /* Shrink the grid */
      glade_command_set_property (glade_widget_get_property (parent, n_row_col),
                                  size - 1);
    }

  g_list_foreach (children, (GFunc) g_object_unref, NULL);
  g_list_free (children);

  glade_command_pop_group ();
}

void
glade_gtk_grid_child_action_activate (GladeWidgetAdaptor * adaptor,
                                       GObject * container,
                                       GObject * object,
                                       const gchar * action_path)
{
  if (strcmp (action_path, "insert_row/after") == 0)
    {
      glade_gtk_grid_child_insert_remove_action (adaptor, container, object,
                                                  _("Insert Row on %s"),
                                                  "n-rows", "top-attach",
                                                  "bottom-attach", FALSE, TRUE);
    }
  else if (strcmp (action_path, "insert_row/before") == 0)
    {
      glade_gtk_grid_child_insert_remove_action (adaptor, container, object,
                                                  _("Insert Row on %s"),
                                                  "n-rows", "top-attach",
                                                  "bottom-attach",
                                                  FALSE, FALSE);
    }
  else if (strcmp (action_path, "insert_column/after") == 0)
    {
      glade_gtk_grid_child_insert_remove_action (adaptor, container, object,
                                                  _("Insert Column on %s"),
                                                  "n-columns", "left-attach",
                                                  "right-attach", FALSE, TRUE);
    }
  else if (strcmp (action_path, "insert_column/before") == 0)
    {
      glade_gtk_grid_child_insert_remove_action (adaptor, container, object,
                                                  _("Insert Column on %s"),
                                                  "n-columns", "left-attach",
                                                  "right-attach", FALSE, FALSE);
    }
  else if (strcmp (action_path, "remove_column") == 0)
    {
      glade_gtk_grid_child_insert_remove_action (adaptor, container, object,
                                                  _("Remove Column on %s"),
                                                  "n-columns", "left-attach",
                                                  "right-attach", TRUE, FALSE);
    }
  else if (strcmp (action_path, "remove_row") == 0)
    {
      glade_gtk_grid_child_insert_remove_action (adaptor, container, object,
						 _("Remove Row on %s"),
						 "n-rows", "top-attach",
						 "bottom-attach", TRUE, FALSE);
    }
  else
    GWA_GET_CLASS (GTK_TYPE_CONTAINER)->child_action_activate (adaptor,
                                                               container,
                                                               object,
                                                               action_path);
}