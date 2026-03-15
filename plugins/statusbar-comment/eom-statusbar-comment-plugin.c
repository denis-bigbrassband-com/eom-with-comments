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
#include <glib/gi18n-lib.h>
#include <libpeas/peas-activatable.h>
#include <pango/pango.h>

#include <eom-debug.h>
#include <eom-image.h>
#include <eom-list-store.h>
#include <eom-thumb-view.h>
#include <eom-window.h>
#include <eom-window-activatable.h>

static void eom_window_activatable_iface_init (EomWindowActivatableInterface *iface);
static void statusbar_set_comment (GtkLabel    *statusbar_comment,
                                   EomThumbView *view);
static void update_edit_comment_action_sensitivity (EomStatusbarCommentPlugin *plugin);

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

static const gchar* const ui_definition =
	"<ui><menubar name=\"MainMenu\">"
	"<menu name=\"Edit\" action=\"Edit\">"
	"<separator name=\"EomPluginCommentSep1\"/>"
	"<menuitem name=\"EomPluginEditComment\" action=\"EomPluginEditComment\"/>"
	"<separator name=\"EomPluginCommentSep2\"/>"
	"</menu></menubar></ui>";

static gboolean
prepared_idle_cb (gpointer user_data)
{
	EomStatusbarCommentPlugin *plugin = EOM_STATUSBAR_COMMENT_PLUGIN (user_data);
	GtkWidget *thumbview;

	/* One-shot deferred refresh to catch startup ordering races. */
	plugin->prepared_idle_id = 0;

	if (plugin->window == NULL || plugin->statusbar_comment == NULL) {
		return G_SOURCE_REMOVE;
	}

	thumbview = eom_window_get_thumb_view (plugin->window);
	statusbar_set_comment (GTK_LABEL (plugin->statusbar_comment),
	                       EOM_THUMB_VIEW (thumbview));
	update_edit_comment_action_sensitivity (plugin);

	return G_SOURCE_REMOVE;
}

static void
update_edit_comment_action_sensitivity (EomStatusbarCommentPlugin *plugin)
{
	EomImage *image;
	GtkAction *action;
	gboolean sensitive = FALSE;

	if (plugin->ui_action_group == NULL) {
		return;
	}

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	action = gtk_action_group_get_action (plugin->ui_action_group, "EomPluginEditComment");
	G_GNUC_END_IGNORE_DEPRECATIONS;

	if (action == NULL) {
		return;
	}

	image = eom_window_get_image (plugin->window);
	if (image != NULL) {
		sensitive = eom_image_is_jpeg (image);
	} else {
		GtkWidget *thumbview;

		/* Fall back to selection/store during command-line startup races. */
		thumbview = eom_window_get_thumb_view (plugin->window);
		image = eom_thumb_view_get_first_selected_image (EOM_THUMB_VIEW (thumbview));
		if (image != NULL) {
			sensitive = eom_image_is_jpeg (image);
			g_object_unref (image);
		} else {
			EomListStore *store;

			store = eom_window_get_store (plugin->window);
			if (store != NULL && eom_list_store_length (store) > 0) {
				image = eom_list_store_get_image_by_pos (store, 0);
				if (image != NULL) {
					sensitive = eom_image_is_jpeg (image);
					g_object_unref (image);
				}
			}
		}
	}

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	gtk_action_set_sensitive (action, sensitive);
	G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
edit_comment_cb (GtkAction                *action,
                 EomStatusbarCommentPlugin *plugin)
{
	GtkWidget *dialog;
	GtkWidget *content;
	GtkWidget *scrolled;
	GtkWidget *text_view;
	GtkTextBuffer *buffer;
	EomImage *image;
	const gchar *comment;
	gint response;

	image = eom_window_get_image (plugin->window);
	if (image == NULL) {
		GtkWidget *thumbview;

		/* Reuse the same fallback chain as sensitivity checks. */
		thumbview = eom_window_get_thumb_view (plugin->window);
		image = eom_thumb_view_get_first_selected_image (EOM_THUMB_VIEW (thumbview));
		if (image == NULL) {
			EomListStore *store;

			store = eom_window_get_store (plugin->window);
			if (store != NULL && eom_list_store_length (store) > 0) {
				image = eom_list_store_get_image_by_pos (store, 0);
			}
		}
	}

	if (image == NULL || !eom_image_is_jpeg (image)) {
		if (image != NULL) {
			g_object_unref (image);
		}
		return;
	}

	dialog = gtk_dialog_new_with_buttons (_("Image Comment"),
	                                      GTK_WINDOW (plugin->window),
	                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                      _("_Cancel"),
	                                      GTK_RESPONSE_CANCEL,
	                                      _("_Save"),
	                                      GTK_RESPONSE_ACCEPT,
	                                      NULL);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 520, 260);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_hexpand (scrolled, TRUE);
	gtk_widget_set_vexpand (scrolled, TRUE);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
	                                GTK_POLICY_AUTOMATIC,
	                                GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (content), scrolled, TRUE, TRUE, 6);

	text_view = gtk_text_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolled), text_view);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
	comment = eom_image_get_comment (image);
	gtk_text_buffer_set_text (buffer, comment != NULL ? comment : "", -1);
	gtk_widget_show_all (content);

	for (;;) {
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		if (response != GTK_RESPONSE_ACCEPT) {
			break;
		} else {
			GtkTextIter start, end;
			gchar *new_comment;
			GError *error = NULL;

			gtk_text_buffer_get_bounds (buffer, &start, &end);
			new_comment = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

			eom_image_set_comment (image, new_comment);

			/* Persist directly to disk without touching undo/transform state. */
			if (eom_image_save_comment (image, &error)) {
				statusbar_set_comment (GTK_LABEL (plugin->statusbar_comment),
				                       EOM_THUMB_VIEW (eom_window_get_thumb_view (plugin->window)));
				g_free (new_comment);
				break;
			}

			/* Keep the editor open so the user can correct and retry. */
			{
				GtkWidget *err_dialog;
				const gchar *details = (error != NULL) ? error->message : _("Unknown error.");

				err_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
				                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				                                     GTK_MESSAGE_ERROR,
				                                     GTK_BUTTONS_CLOSE,
				                                     "%s",
				                                     _("Could not save image comment."));
				gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (err_dialog),
				                                          "%s",
				                                          details);
				gtk_dialog_run (GTK_DIALOG (err_dialog));
				gtk_widget_destroy (err_dialog);
			}

			g_clear_error (&error);
			g_free (new_comment);
		}
	}

	gtk_widget_destroy (dialog);
	g_object_unref (image);
}

static const GtkActionEntry action_entries[] = {
	{ "EomPluginEditComment", "document-edit", N_("Edit comment"), "<Control>I", N_("Edit image comment"), G_CALLBACK (edit_comment_cb) }
};

static void
statusbar_set_comment (GtkLabel    *statusbar_comment,
                       EomThumbView *view)
{
	EomImage *image;
	const gchar *comment;
	gchar *clean_comment;

	if (eom_thumb_view_get_n_selected (view) == 0) {
		gtk_label_set_text (statusbar_comment, "");
		gtk_widget_hide (GTK_WIDGET (statusbar_comment));
		return;
	}

	image = eom_thumb_view_get_first_selected_image (view);
	if (image == NULL) {
		gtk_label_set_text (statusbar_comment, "");
		gtk_widget_hide (GTK_WIDGET (statusbar_comment));
		return;
	}

	if (!eom_image_has_data (image, EOM_IMAGE_DATA_EXIF)) {
		/* Comment is read via metadata path, so ensure EXIF pass happened. */
		if (!eom_image_load (image, EOM_IMAGE_DATA_EXIF, NULL, NULL)) {
			gtk_label_set_text (statusbar_comment, "");
			gtk_widget_hide (GTK_WIDGET (statusbar_comment));
			g_object_unref (image);
			return;
		}
	}

	comment = eom_image_get_comment (image);
	if (comment == NULL || *comment == '\0') {
		gtk_label_set_text (statusbar_comment, "");
		gtk_widget_hide (GTK_WIDGET (statusbar_comment));
		g_object_unref (image);
		return;
	}

	clean_comment = g_strdup (comment);
	/* Keep the statusbar single-line even for multiline comments. */
	g_strdelimit (clean_comment, "\r\n\t", ' ');
	g_strstrip (clean_comment);

	if (*clean_comment == '\0') {
		gtk_label_set_text (statusbar_comment, "");
		gtk_widget_hide (GTK_WIDGET (statusbar_comment));
		g_free (clean_comment);
		g_object_unref (image);
		return;
	}

	gtk_label_set_text (statusbar_comment, clean_comment);
	gtk_widget_show (GTK_WIDGET (statusbar_comment));
	g_free (clean_comment);
	g_object_unref (image);
}

static void
selection_changed_cb (EomThumbView              *view,
                      EomStatusbarCommentPlugin *plugin)
{
	statusbar_set_comment (GTK_LABEL (plugin->statusbar_comment), view);
	update_edit_comment_action_sensitivity (plugin);
}

static void
window_prepared_cb (EomWindow                 *window,
                    EomStatusbarCommentPlugin *plugin)
{
	GtkWidget *thumbview;

	thumbview = eom_window_get_thumb_view (window);
	statusbar_set_comment (GTK_LABEL (plugin->statusbar_comment),
	                       EOM_THUMB_VIEW (thumbview));
	update_edit_comment_action_sensitivity (plugin);

	if (plugin->prepared_idle_id != 0) {
		g_source_remove (plugin->prepared_idle_id);
	}

	/* Run once more in idle after other startup handlers settle state. */
	plugin->prepared_idle_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
	                                            prepared_idle_cb,
	                                            g_object_ref (plugin),
	                                            (GDestroyNotify) g_object_unref);
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
	GtkUIManager *manager;

	eom_debug (DEBUG_PLUGINS);

	manager = eom_window_get_ui_manager (plugin->window);

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	plugin->ui_action_group = gtk_action_group_new ("EomStatusbarCommentPluginActions");
#ifdef ENABLE_NLS
	gtk_action_group_set_translation_domain (plugin->ui_action_group, GETTEXT_PACKAGE);
#endif
	gtk_action_group_add_actions (plugin->ui_action_group,
	                              action_entries,
	                              G_N_ELEMENTS (action_entries),
	                              plugin);
	G_GNUC_END_IGNORE_DEPRECATIONS;

	gtk_ui_manager_insert_action_group (manager, plugin->ui_action_group, -1);
	plugin->ui_id = gtk_ui_manager_add_ui_from_string (manager, ui_definition, -1, NULL);
	g_warn_if_fail (plugin->ui_id != 0);

	plugin->statusbar_comment = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (plugin->statusbar_comment), 0.0f);
	gtk_widget_set_hexpand (plugin->statusbar_comment, TRUE);
	gtk_widget_set_halign (plugin->statusbar_comment, GTK_ALIGN_START);
	gtk_widget_set_valign (plugin->statusbar_comment, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_start (plugin->statusbar_comment, 8);
	gtk_widget_set_margin_top (GTK_WIDGET (plugin->statusbar_comment), 0);
	gtk_widget_set_margin_bottom (GTK_WIDGET (plugin->statusbar_comment), 0);
	/* Reserve remaining center space and truncate long comments visually. */
	gtk_label_set_ellipsize (GTK_LABEL (plugin->statusbar_comment), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start (GTK_BOX (statusbar), plugin->statusbar_comment, TRUE, TRUE, 0);
	/* Child index 1 puts comment right after the main status text area. */
	gtk_box_reorder_child (GTK_BOX (statusbar), plugin->statusbar_comment, 1);

	plugin->signal_id = g_signal_connect_after (G_OBJECT (thumbview), "selection_changed",
	                                            G_CALLBACK (selection_changed_cb), plugin);
	/* Window emits "prepared" when initial image/model loading finishes. */
	plugin->prepared_signal_id = g_signal_connect (G_OBJECT (window),
	                                               "prepared",
	                                               G_CALLBACK (window_prepared_cb),
	                                               plugin);

	statusbar_set_comment (GTK_LABEL (plugin->statusbar_comment),
	                       EOM_THUMB_VIEW (thumbview));
	update_edit_comment_action_sensitivity (plugin);
}

static void
eom_statusbar_comment_plugin_deactivate (EomWindowActivatable *activatable)
{
	EomStatusbarCommentPlugin *plugin = EOM_STATUSBAR_COMMENT_PLUGIN (activatable);
	EomWindow *window = plugin->window;
	GtkWidget *statusbar = eom_window_get_statusbar (window);
	GtkWidget *view = eom_window_get_thumb_view (window);
	GtkUIManager *manager = eom_window_get_ui_manager (plugin->window);

#if GLIB_CHECK_VERSION(2,62,0)
	g_clear_signal_handler (&plugin->signal_id, view);
#else
	if (plugin->signal_id != 0) {
		g_signal_handler_disconnect (view, plugin->signal_id);
		plugin->signal_id = 0;
	}
#endif

	if (plugin->prepared_signal_id != 0) {
		g_signal_handler_disconnect (window, plugin->prepared_signal_id);
		plugin->prepared_signal_id = 0;
	}

	if (plugin->prepared_idle_id != 0) {
		g_source_remove (plugin->prepared_idle_id);
		plugin->prepared_idle_id = 0;
	}

	gtk_container_remove (GTK_CONTAINER (statusbar), plugin->statusbar_comment);

	if (plugin->ui_id != 0) {
		gtk_ui_manager_remove_ui (manager, plugin->ui_id);
		plugin->ui_id = 0;
	}

	if (plugin->ui_action_group != NULL) {
		gtk_ui_manager_remove_action_group (manager, plugin->ui_action_group);
		g_object_unref (plugin->ui_action_group);
		plugin->ui_action_group = NULL;
	}

	gtk_ui_manager_ensure_update (manager);
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
