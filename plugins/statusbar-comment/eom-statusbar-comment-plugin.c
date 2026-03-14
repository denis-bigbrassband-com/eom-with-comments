/* Statusbar Comment -- Shows the image comment in EOM's statusbar
 *
 * Copyright (C) 2026 The Free Software Foundation
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eom-statusbar-comment-plugin.h"

#include <gmodule.h>
#include <libpeas/peas-activatable.h>

#include <eom-debug.h>
#include <eom-image.h>
#include <eom-thumb-view.h>
#include <eom-window.h>
#include <eom-window-activatable.h>

static void eom_window_activatable_iface_init (EomWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (EomStatusbarCommentPlugin,
                                eom_statusbar_comment_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (EOM_TYPE_WINDOW_ACTIVATABLE,
                                                               eom_window_activatable_iface_init))

enum {
	PROP_0,
	PROP_WINDOW
};

static void
statusbar_set_comment (GtkStatusbar *statusbar,
                       EomThumbView *view)
{
	EomImage *image;
	const gchar *comment;
	gchar *clean_comment;

	gtk_statusbar_pop (statusbar, 0);

	if (eom_thumb_view_get_n_selected (view) == 0) {
		gtk_widget_hide (GTK_WIDGET (statusbar));
		return;
	}

	image = eom_thumb_view_get_first_selected_image (view);
	if (image == NULL) {
		gtk_widget_hide (GTK_WIDGET (statusbar));
		return;
	}

	if (!eom_image_has_data (image, EOM_IMAGE_DATA_EXIF)) {
		if (!eom_image_load (image, EOM_IMAGE_DATA_EXIF, NULL, NULL)) {
			gtk_widget_hide (GTK_WIDGET (statusbar));
			return;
		}
	}

	comment = eom_image_get_comment (image);
	if (comment == NULL || *comment == '\0') {
		gtk_widget_hide (GTK_WIDGET (statusbar));
		return;
	}

	clean_comment = g_strdup (comment);
	g_strdelimit (clean_comment, "\r\n\t", ' ');
	g_strstrip (clean_comment);

	if (*clean_comment == '\0') {
		gtk_widget_hide (GTK_WIDGET (statusbar));
		g_free (clean_comment);
		return;
	}

	gtk_statusbar_push (statusbar, 0, clean_comment);
	gtk_widget_show (GTK_WIDGET (statusbar));
	g_free (clean_comment);
}

static void
selection_changed_cb (EomThumbView              *view,
                      EomStatusbarCommentPlugin *plugin)
{
	statusbar_set_comment (GTK_STATUSBAR (plugin->statusbar_comment), view);
}

static void
eom_statusbar_comment_plugin_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
	EomStatusbarCommentPlugin *plugin = EOM_STATUSBAR_COMMENT_PLUGIN (object);

	switch (prop_id) {
	case PROP_WINDOW:
		plugin->window = EOM_WINDOW (g_value_dup_object (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
eom_statusbar_comment_plugin_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
	EomStatusbarCommentPlugin *plugin = EOM_STATUSBAR_COMMENT_PLUGIN (object);

	switch (prop_id) {
	case PROP_WINDOW:
		g_value_set_object (value, plugin->window);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
eom_statusbar_comment_plugin_init (EomStatusbarCommentPlugin *plugin)
{
	eom_debug_message (DEBUG_PLUGINS, "EomStatusbarCommentPlugin initializing");
}

static void
eom_statusbar_comment_plugin_dispose (GObject *object)
{
	EomStatusbarCommentPlugin *plugin = EOM_STATUSBAR_COMMENT_PLUGIN (object);

	eom_debug_message (DEBUG_PLUGINS, "EomStatusbarCommentPlugin disposing");

	if (plugin->window != NULL) {
		g_object_unref (plugin->window);
		plugin->window = NULL;
	}

	G_OBJECT_CLASS (eom_statusbar_comment_plugin_parent_class)->dispose (object);
}

static void
eom_statusbar_comment_plugin_activate (EomWindowActivatable *activatable)
{
	EomStatusbarCommentPlugin *plugin = EOM_STATUSBAR_COMMENT_PLUGIN (activatable);
	EomWindow *window = plugin->window;
	GtkWidget *statusbar = eom_window_get_statusbar (window);
	GtkWidget *thumbview = eom_window_get_thumb_view (window);

	eom_debug (DEBUG_PLUGINS);

	plugin->statusbar_comment = gtk_statusbar_new ();
	gtk_widget_set_size_request (plugin->statusbar_comment, 200, 10);
	gtk_widget_set_margin_top (GTK_WIDGET (plugin->statusbar_comment), 0);
	gtk_widget_set_margin_bottom (GTK_WIDGET (plugin->statusbar_comment), 0);
	gtk_box_pack_end (GTK_BOX (statusbar), plugin->statusbar_comment, FALSE, FALSE, 0);

	plugin->signal_id = g_signal_connect_after (G_OBJECT (thumbview), "selection_changed",
	                                            G_CALLBACK (selection_changed_cb), plugin);

	statusbar_set_comment (GTK_STATUSBAR (plugin->statusbar_comment),
	                       EOM_THUMB_VIEW (thumbview));
}

static void
eom_statusbar_comment_plugin_deactivate (EomWindowActivatable *activatable)
{
	EomStatusbarCommentPlugin *plugin = EOM_STATUSBAR_COMMENT_PLUGIN (activatable);
	EomWindow *window = plugin->window;
	GtkWidget *statusbar = eom_window_get_statusbar (window);
	GtkWidget *view = eom_window_get_thumb_view (window);

#if GLIB_CHECK_VERSION(2,62,0)
	g_clear_signal_handler (&plugin->signal_id, view);
#else
	if (plugin->signal_id != 0) {
		g_signal_handler_disconnect (view, plugin->signal_id);
		plugin->signal_id = 0;
	}
#endif

	gtk_container_remove (GTK_CONTAINER (statusbar), plugin->statusbar_comment);
}

static void
eom_statusbar_comment_plugin_class_init (EomStatusbarCommentPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = eom_statusbar_comment_plugin_dispose;
	object_class->set_property = eom_statusbar_comment_plugin_set_property;
	object_class->get_property = eom_statusbar_comment_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
eom_statusbar_comment_plugin_class_finalize (EomStatusbarCommentPluginClass *klass)
{
	/* Dummy function - used by G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
eom_window_activatable_iface_init (EomWindowActivatableInterface *iface)
{
	iface->activate = eom_statusbar_comment_plugin_activate;
	iface->deactivate = eom_statusbar_comment_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	eom_statusbar_comment_plugin_register_type (G_TYPE_MODULE (module));
	peas_object_module_register_extension_type (module,
	                                            EOM_TYPE_WINDOW_ACTIVATABLE,
	                                            EOM_TYPE_STATUSBAR_COMMENT_PLUGIN);
}
