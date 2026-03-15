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

#ifndef __EOM_STATUSBAR_COMMENT_PLUGIN_H__
#define __EOM_STATUSBAR_COMMENT_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

#include <eom-window.h>

G_BEGIN_DECLS

#define EOM_TYPE_STATUSBAR_COMMENT_PLUGIN \
	(eom_statusbar_comment_plugin_get_type())
#define EOM_STATUSBAR_COMMENT_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), EOM_TYPE_STATUSBAR_COMMENT_PLUGIN, EomStatusbarCommentPlugin))
#define EOM_STATUSBAR_COMMENT_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), EOM_TYPE_STATUSBAR_COMMENT_PLUGIN, EomStatusbarCommentPluginClass))
#define EOM_IS_STATUSBAR_COMMENT_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), EOM_TYPE_STATUSBAR_COMMENT_PLUGIN))
#define EOM_IS_STATUSBAR_COMMENT_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), EOM_TYPE_STATUSBAR_COMMENT_PLUGIN))
#define EOM_STATUSBAR_COMMENT_PLUGIN_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), EOM_TYPE_STATUSBAR_COMMENT_PLUGIN, EomStatusbarCommentPluginClass))

typedef struct _EomStatusbarCommentPlugin EomStatusbarCommentPlugin;
typedef struct _EomStatusbarCommentPluginClass EomStatusbarCommentPluginClass;

struct _EomStatusbarCommentPlugin {
	PeasExtensionBase parent_instance;

	EomWindow *window;
	GtkActionGroup *ui_action_group;
	guint ui_id;
	GtkWidget *statusbar_comment;
	gulong signal_id;
	gulong prepared_signal_id;
	guint prepared_idle_id;
};

struct _EomStatusbarCommentPluginClass {
	PeasExtensionBaseClass parent_class;
};

GType eom_statusbar_comment_plugin_get_type (void) G_GNUC_CONST;

G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_END_DECLS

#endif /* __EOM_STATUSBAR_COMMENT_PLUGIN_H__ */
