/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/route_target_selector.h"
#include "gui/widgets/route_target_selector_popover.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (RouteTargetSelectorWidget,
               route_target_selector_widget,
               GTK_TYPE_MENU_BUTTON)

#define MAX_CHARS 8

static void
set_label (RouteTargetSelectorWidget * self)
{
  if (self->channel &&
      self->channel->output)
    {
      Track * track = self->channel->output;
      g_return_if_fail (track->name);

      gtk_label_set_text (
        self->label,
        track->name);
    }
  else
    {
      gtk_label_set_markup (
        self->label,
        _("<tt><i>Engine</i></tt>"));
    }
}

void
route_target_selector_widget_refresh (
  RouteTargetSelectorWidget * self,
  Channel *                   channel)
{
  self->channel = channel;

  set_label (self);

  /*if (self->popover && GTK_IS_WIDGET (self->popover))*/
    /*g_object_unref (self->popover);*/
  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self), NULL);
  self->popover = NULL;

  /* if unroutable */
  if (!self->channel)
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->box),
        _("Cannot be routed"));
    }
  /* if routed by default and cannot be changed */
  else if (
      self->channel->track->type ==
        TRACK_TYPE_MASTER)
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->box),
        _("Routed to engine"));
    }
  /* if routable */
  else
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->box),
        _("Select channel to route signal to"));
      self->popover =
        route_target_selector_popover_widget_new (
          self);
      gtk_menu_button_set_popover (
        GTK_MENU_BUTTON (self),
        GTK_WIDGET (self->popover));
    }
}

void
route_target_selector_widget_setup (
  RouteTargetSelectorWidget * self,
  Channel *                   channel)
{
  self->channel = channel;

  set_label (self);
}

RouteTargetSelectorWidget *
route_target_selector_widget_new (
  Channel * channel)
{
  RouteTargetSelectorWidget * self =
    g_object_new (
      ROUTE_TARGET_SELECTOR_WIDGET_TYPE, NULL);

  route_target_selector_widget_setup (
    self, channel);

  return self;
}

static void
finalize (
  RouteTargetSelectorWidget * self)
{
  if (self->popover && G_IS_OBJECT (self->popover))
    g_object_unref (self->popover);

  G_OBJECT_CLASS (
    route_target_selector_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
route_target_selector_widget_class_init (
  RouteTargetSelectorWidgetClass * _klass)
{
  GObjectClass * oklass =
    G_OBJECT_CLASS (_klass);

  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
route_target_selector_widget_init (
  RouteTargetSelectorWidget * self)
{
  /* add class */
  GtkStyleContext * context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self));
  gtk_style_context_add_class (
    context, "route_target_selector");

  self->box =
    GTK_BOX (gtk_box_new (
              GTK_ORIENTATION_HORIZONTAL, 0));
  self->img = GTK_IMAGE (
    resources_get_icon (
      ICON_TYPE_GNOME_BUILDER,
      "debug-step-out-symbolic-light.svg"));

  self->label =
    GTK_LABEL (
      gtk_label_new (_("Stereo Out")));
  context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self->label));
  gtk_style_context_add_class (
    context, "channel_label_smaller");
  gtk_label_set_ellipsize (
    self->label, PANGO_ELLIPSIZE_END);

  gtk_box_pack_start (self->box,
                      GTK_WIDGET (self->img),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      1);
  gtk_box_pack_start (self->box,
                    GTK_WIDGET (self->label),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    1);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->box));

  gtk_widget_show_all (GTK_WIDGET (self));
}
