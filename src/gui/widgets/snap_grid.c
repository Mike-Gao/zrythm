/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/snap_grid.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/snap_grid_popover.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (SnapGridWidget,
               snap_grid_widget,
               GTK_TYPE_MENU_BUTTON)

static void
on_clicked (GtkButton * button,
            SnapGridWidget * self)
{
  gtk_widget_show_all (GTK_WIDGET (self->popover));
}

static void
set_label (SnapGridWidget * self)
{
  char * string =
    snap_grid_stringize (
      self->snap_grid->note_length,
      self->snap_grid->note_type);
  gtk_label_set_text (self->label, string);
  g_free (string);
}

void
snap_grid_widget_refresh (
  SnapGridWidget * self)
{
  set_label (self);
}

void
snap_grid_widget_setup (
  SnapGridWidget * self,
  SnapGrid * snap_grid)
{
  self->snap_grid = snap_grid;
  self->popover =
    snap_grid_popover_widget_new (self);
  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self),
    GTK_WIDGET (self->popover));

  set_label (self);
}

static void
snap_grid_widget_class_init (
  SnapGridWidgetClass * klass)
{
}

static void
snap_grid_widget_init (SnapGridWidget * self)
{
  self->box =
    GTK_BOX (gtk_box_new (
               GTK_ORIENTATION_HORIZONTAL, 0));
  self->img = GTK_IMAGE (
    resources_get_icon (
      ICON_TYPE_GNOME_BUILDER,
      "completion-snippet-symbolic-light.svg"));
  self->label =
    GTK_LABEL (gtk_label_new (""));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->box),
    _("Snap/Grid options"));
  gtk_box_pack_start (self->box,
                      GTK_WIDGET (self->img),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      1);
  gtk_box_pack_end (self->box,
                    GTK_WIDGET (self->label),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    1);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->box));
  g_signal_connect (G_OBJECT (self),
                    "clicked",
                    G_CALLBACK (on_clicked),
                    self);

  gtk_widget_show_all (GTK_WIDGET (self));
}

