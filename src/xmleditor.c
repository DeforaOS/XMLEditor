/* $Id$ */
static char const _copyright[] =
"Copyright © 2010-2015 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop XMLEditor */
static char const _license[] =
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by the\n"
"Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program. If not, see <http://www.gnu.org/licenses/>.\n";
/* TODO:
 * - only allow legitimate columns to be modified */



#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gdk/gdkkeysyms.h>
#include <System/Parser.h>
#include <Desktop.h>
#include "callbacks.h"
#include "xmleditor.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)


/* XMLEditor */
/* private */
/* types */
typedef enum _XMLEditorColumn {
	XEC_TAGNAME = 0, XEC_ATTRIBUTE_NAME, XEC_ATTRIBUTE_VALUE, XEC_DATA,
	XEC_ENTITY
} XMLEditorColumn;
#define XEC_LAST	XEC_ENTITY
#define XEC_COUNT	(XEC_LAST + 1)

struct _XMLEditor
{
	XML * xml;
	gboolean modified;

	/* widgets */
	GtkWidget * window;
	GtkTreeStore * store;
	GtkWidget * view;
	GtkWidget * statusbar;
	/* about */
	GtkWidget * ab_window;
};


/* variables */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};

#ifdef EMBEDDED
static DesktopAccel _xmleditor_accel[] =
{
	{ G_CALLBACK(on_close), GDK_CONTROL_MASK, GDK_KEY_W },
	{ G_CALLBACK(on_new), GDK_CONTROL_MASK, GDK_KEY_N },
	{ G_CALLBACK(on_open), GDK_CONTROL_MASK, GDK_KEY_O },
	{ G_CALLBACK(on_preferences), GDK_CONTROL_MASK, GDK_KEY_P },
	{ G_CALLBACK(on_save), GDK_CONTROL_MASK, GDK_KEY_S },
	{ G_CALLBACK(on_save_as), GDK_CONTROL_MASK | GDK_SHIFT_MASK,
		GDK_KEY_S },
	{ NULL, 0, 0 }
};
#endif

#ifndef EMBEDDED
static DesktopMenu _xmleditor_menu_file[] =
{
	{ N_("_New"), G_CALLBACK(on_file_new), GTK_STOCK_NEW, GDK_CONTROL_MASK,
		GDK_KEY_N },
	{ N_("_Open"), G_CALLBACK(on_file_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Save"), G_CALLBACK(on_file_save), GTK_STOCK_SAVE,
		GDK_CONTROL_MASK, GDK_KEY_S },
	{ N_("_Save as..."), G_CALLBACK(on_file_save_as), GTK_STOCK_SAVE_AS,
		GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_S },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenu _xmleditor_menu_edit[] =
{
	{ N_("_Preferences"), G_CALLBACK(on_edit_preferences),
		GTK_STOCK_PREFERENCES, GDK_CONTROL_MASK, GDK_KEY_P },
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenu _xmleditor_menu_view[] =
{
	{ N_("_Expand all"), G_CALLBACK(on_view_expand_all), NULL, 0,
		GDK_KEY_asterisk },
	{ N_("_Collapse all"), G_CALLBACK(on_view_collapse_all), NULL, 0,
		GDK_KEY_slash },
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenu _xmleditor_menu_help[] =
{
	{ N_("_About"), G_CALLBACK(on_help_about),
#if GTK_CHECK_VERSION(2, 6, 0)
		GTK_STOCK_ABOUT, 0, 0 },
#else
		NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenubar _xmleditor_menubar[] =
{
	{ N_("_File"), _xmleditor_menu_file },
	{ N_("_Edit"), _xmleditor_menu_edit },
	{ N_("_View"), _xmleditor_menu_view },
	{ N_("_Help"), _xmleditor_menu_help },
	{ NULL, NULL }
};
#endif

static DesktopToolbar _xmleditor_toolbar[] =
{
	{ N_("New"), G_CALLBACK(on_new), GTK_STOCK_NEW, 0, 0, NULL },
	{ N_("Open"), G_CALLBACK(on_open), GTK_STOCK_OPEN, 0, 0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Save"), G_CALLBACK(on_save), GTK_STOCK_SAVE, 0, 0, NULL },
	{ N_("Save as"), G_CALLBACK(on_save_as), GTK_STOCK_SAVE_AS, 0, 0,
		NULL },
#ifdef EMBEDDED
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Preferences"), G_CALLBACK(on_preferences), GTK_STOCK_PREFERENCES,
		0, 0, NULL },
#endif
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* xmleditor_new */
static void _new_set_title(XMLEditor * xmleditor);

XMLEditor * xmleditor_new(void)
{
	XMLEditor * xmleditor;
	GtkAccelGroup * group;
	GtkSettings * settings;
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	if((xmleditor = malloc(sizeof(*xmleditor))) == NULL)
		return NULL;
	settings = gtk_settings_get_default();
	xmleditor->xml = NULL;
	xmleditor->modified = FALSE;
	/* widgets */
	group = gtk_accel_group_new();
	xmleditor->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(xmleditor->window), group);
	g_object_unref(group);
	gtk_window_set_default_size(GTK_WINDOW(xmleditor->window), 600, 400);
	_new_set_title(xmleditor);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(xmleditor->window), "text-editor");
#endif
	g_signal_connect_swapped(G_OBJECT(xmleditor->window), "delete-event",
			G_CALLBACK(on_closex), xmleditor);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	/* menubar */
#ifndef EMBEDDED
	widget = desktop_menubar_create(_xmleditor_menubar, xmleditor, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
#else
	desktop_accel_create(_xmleditor_accel, xmleditor, group);
#endif
	/* toolbar */
	widget = desktop_toolbar_create(_xmleditor_toolbar, xmleditor, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	/* view */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	xmleditor->store = gtk_tree_store_new(XEC_COUNT,
			G_TYPE_STRING,	/* XEC_TAGNAME		*/
			G_TYPE_STRING,	/* XEC_ATTRIBUTE_NAME	*/
			G_TYPE_STRING,	/* XEC_ATTRIBUTE_VALUE	*/
			G_TYPE_STRING,	/* XEC_DATA		*/
			G_TYPE_STRING);	/* XEC_ENTITY		*/
	xmleditor->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				xmleditor->store));
#if 0
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(xmleditor->view),
			FALSE);
#endif
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(xmleditor->view), TRUE);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(
				on_tag_name_edited), xmleditor);
	column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer,
			"text", XEC_TAGNAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(xmleditor->view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Entity"), renderer,
			"text", XEC_ENTITY, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(xmleditor->view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Data"), renderer,
			"text", XEC_DATA, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(xmleditor->view), column);
	gtk_container_add(GTK_CONTAINER(widget), xmleditor->view);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	/* statusbar */
	xmleditor->statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), xmleditor->statusbar, FALSE, FALSE,
			0);
	/* about */
	xmleditor->ab_window = NULL;
	gtk_container_add(GTK_CONTAINER(xmleditor->window), vbox);
	gtk_window_set_focus(GTK_WINDOW(xmleditor->window), xmleditor->view);
	gtk_widget_show_all(xmleditor->window);
	return xmleditor;
}

static void _new_set_title(XMLEditor * xmleditor)
{
	char buf[256];

	snprintf(buf, sizeof(buf), "%s%s", _("XML editor - "),
			(xmleditor->xml == NULL) ? _("(Untitled)")
			: xml_get_filename(xmleditor->xml));
	gtk_window_set_title(GTK_WINDOW(xmleditor->window), buf);
}


/* xmleditor_delete */
void xmleditor_delete(XMLEditor * xmleditor)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(xmleditor->xml != NULL)
		xml_delete(xmleditor->xml);
	free(xmleditor);
}


/* useful */
/* xmleditor_about */
static gboolean _about_on_closex(GtkWidget * widget);

void xmleditor_about(XMLEditor * xmleditor)
{
	if(xmleditor->ab_window != NULL)
	{
		gtk_widget_show(xmleditor->ab_window);
		return;
	}
	xmleditor->ab_window = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(xmleditor->ab_window),
			GTK_WINDOW(xmleditor->window));
	g_signal_connect(G_OBJECT(xmleditor->ab_window), "delete-event",
			G_CALLBACK(_about_on_closex), NULL);
	desktop_about_dialog_set_authors(xmleditor->ab_window, _authors);
	desktop_about_dialog_set_comments(xmleditor->ab_window,
			_("XML editor for the DeforaOS desktop"));
	desktop_about_dialog_set_copyright(xmleditor->ab_window, _copyright);
	desktop_about_dialog_set_license(xmleditor->ab_window, _license);
	desktop_about_dialog_set_logo_icon_name(xmleditor->ab_window,
			"text-editor");
	desktop_about_dialog_set_name(xmleditor->ab_window, PACKAGE);
	desktop_about_dialog_set_translator_credits(xmleditor->ab_window,
			_("translator-credits"));
	desktop_about_dialog_set_version(xmleditor->ab_window, VERSION);
	desktop_about_dialog_set_website(xmleditor->ab_window,
			"http://www.defora.org/");
	gtk_widget_show(xmleditor->ab_window);
}

static gboolean _about_on_closex(GtkWidget * widget)
{
	gtk_widget_hide(widget);
	return TRUE;
}


/* xmleditor_error */
int xmleditor_error(XMLEditor * xmleditor, char const * message, int ret)
{
	GtkWidget * dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(xmleditor->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s",
#if GTK_CHECK_VERSION(2, 6, 0)
			_("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
			"%s",
#endif
			message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(
				gtk_widget_destroy), NULL);
	gtk_widget_show(dialog);
	return ret;
}


/* xmleditor_close */
gboolean xmleditor_close(XMLEditor * xmleditor)
{
	GtkWidget * dialog;
	int res;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(xmleditor->modified == FALSE)
	{
		gtk_main_quit();
		return FALSE;
	}
	dialog = gtk_message_dialog_new(GTK_WINDOW(xmleditor->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Warning"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", _("There are unsaved changes.\n"
				"Discard or save them?"));
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_DISCARD,
			GTK_RESPONSE_REJECT, GTK_STOCK_SAVE,
			GTK_RESPONSE_ACCEPT, NULL);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Warning"));
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if(res == GTK_RESPONSE_CANCEL)
		return TRUE;
	else if(res == GTK_RESPONSE_ACCEPT && xmleditor_save(xmleditor) != TRUE)
		return TRUE;
	gtk_main_quit();
	return FALSE;
}


/* xmleditor_collapse_all */
void xmleditor_collapse_all(XMLEditor * xmleditor)
{
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(xmleditor->view));
}


/* xmleditor_expand_all */
void xmleditor_expand_all(XMLEditor * xmleditor)
{
	gtk_tree_view_expand_all(GTK_TREE_VIEW(xmleditor->view));
}


/* xmleditor_open */
static void _open_document_node(XMLEditor * xmleditor, XMLNode * node,
		GtkTreeIter * parent);

int xmleditor_open(XMLEditor * xmleditor, char const * filename)
{
	XMLDocument * doc;

	/* FIXME handle errors */
	gtk_tree_store_clear(xmleditor->store);
	if(filename == NULL)
		return xmleditor_open_dialog(xmleditor);
	if(xmleditor->xml != NULL)
		xml_delete(xmleditor->xml);
	if((xmleditor->xml = xml_new(NULL, filename)) == NULL)
		return -xmleditor_error(xmleditor, error_get(NULL), 1);
	if((doc = xml_get_document(xmleditor->xml)) != NULL)
		_open_document_node(xmleditor, doc->root, NULL);
	_new_set_title(xmleditor); /* XXX make it a generic private function */
	return 0;
}

static void _open_document_node(XMLEditor * xmleditor, XMLNode * node,
		GtkTreeIter * parent)
{
	size_t i;
	GtkTreeIter iter;
	GtkTreeIter child;

	if(node == NULL)
		return;
	switch(node->type)
	{
		case XML_NODE_TYPE_DATA:
			gtk_tree_store_append(xmleditor->store, &iter, parent);
			gtk_tree_store_set(xmleditor->store, &iter, XEC_DATA,
					node->data.buffer, -1);
			break;
		case XML_NODE_TYPE_ENTITY:
			gtk_tree_store_append(xmleditor->store, &iter, parent);
			gtk_tree_store_set(xmleditor->store, &iter, XEC_ENTITY,
					node->entity.name, -1);
			break;
		case XML_NODE_TYPE_TAG:
			gtk_tree_store_append(xmleditor->store, &iter, parent);
			gtk_tree_store_set(xmleditor->store, &iter, XEC_TAGNAME,
					node->tag.name, -1);
			for(i = 0; i < node->tag.attributes_cnt; i++)
			{
				gtk_tree_store_append(xmleditor->store, &child,
						&iter);
				gtk_tree_store_set(xmleditor->store, &child,
						XEC_ATTRIBUTE_NAME,
						node->tag.attributes[i]->name,
						XEC_ATTRIBUTE_VALUE,
						node->tag.attributes[i]->value,
						-1);
			}
			for(i = 0; i < node->tag.childs_cnt; i++)
				_open_document_node(xmleditor,
						node->tag.childs[i], &iter);
			break;
	}
}


/* xmleditor_open_dialog */
int xmleditor_open_dialog(XMLEditor * xmleditor)
{
	int ret;
	GtkWidget * dialog;
	GtkFileFilter * filter;
	char * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Open file..."),
			GTK_WINDOW(xmleditor->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("XML files"));
	gtk_file_filter_add_mime_type(filter, "text/xml");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return -1;
	ret = xmleditor_open(xmleditor, filename);
	g_free(filename);
	return ret;
}


/* xmleditor_save */
static void _save_do(XMLEditor * xmleditor, FILE * fp, GtkTreeIter * parent);

gboolean xmleditor_save(XMLEditor * xmleditor)
{
	char const * filename;
	FILE * fp;
	char * buf;
	GtkTreeIter iter;

	if(xmleditor->xml == NULL)
		return xmleditor_save_as_dialog(xmleditor);
	filename = xml_get_filename(xmleditor->xml);
	if((fp = fopen(filename, "w")) == NULL)
	{
		buf = g_strdup_printf("%s: %s", filename, strerror(errno));
		xmleditor_error(xmleditor, buf, 1);
		g_free(buf);
		return FALSE;
	}
	if(gtk_tree_model_iter_children(GTK_TREE_MODEL(xmleditor->store), &iter,
				NULL) == TRUE)
		_save_do(xmleditor, fp, &iter);
	fclose(fp);
	return TRUE;
}

static void _save_do(XMLEditor * xmleditor, FILE * fp, GtkTreeIter * parent)
{
	GtkTreeModel * model = GTK_TREE_MODEL(xmleditor->store);
	GtkTreeIter child;
	gchar * name;
	gboolean valid;
	gchar * aname;
	gchar * avalue;
	gchar * tag;
	gchar * data;
	gchar * entity;

	gtk_tree_model_get(model, parent, XEC_TAGNAME, &name, -1);
	fprintf(fp, "<%s", name);
	/* attributes */
	gtk_tree_model_iter_children(model, &child, parent);
	for(valid = gtk_tree_model_iter_children(model, &child, parent);
			valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &child))
	{
		gtk_tree_model_get(model, &child, XEC_ATTRIBUTE_NAME, &aname,
				XEC_ATTRIBUTE_VALUE, &avalue, -1);
		if(aname != NULL)
			fprintf(fp, " %s=\"%s\"", aname, (avalue != NULL)
					? avalue : aname);
		g_free(aname);
		g_free(avalue);
	}
	fprintf(fp, ">");
	/* content */
	gtk_tree_model_iter_children(model, &child, parent);
	for(valid = gtk_tree_model_iter_children(model, &child, parent);
			valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &child))
	{
		gtk_tree_model_get(model, &child, XEC_TAGNAME, &tag,
				XEC_DATA, &data, XEC_ENTITY, &entity, -1);
		if(tag != NULL)
			_save_do(xmleditor, fp, &child);
		else if(data != NULL)
			fprintf(fp, "%s", data);
		else if(entity != NULL)
			fprintf(fp, "&%s;", entity);
		g_free(tag);
		g_free(data);
		g_free(entity);
	}
	fprintf(fp, "</%s>", name);
	g_free(name);
}


/* xmleditor_save_as */
gboolean xmleditor_save_as(XMLEditor * xmleditor, char const * filename)
{
	struct stat st;
	GtkWidget * dialog;
	int res;

	if(stat(filename, &st) == 0)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(xmleditor->window),
				GTK_DIALOG_MODAL
				| GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, "%s",
#if GTK_CHECK_VERSION(2, 6, 0)
				_("Warning"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(
					dialog), "%s",
#endif
				_("This file already exists. Overwrite?"));
		gtk_window_set_title(GTK_WINDOW(dialog), _("Warning"));
		res = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(res == GTK_RESPONSE_NO)
			return FALSE;
	}
	/* FIXME implement */
	return TRUE;
}


/* xmleditor_save_as_dialog */
gboolean xmleditor_save_as_dialog(XMLEditor * xmleditor)
{
	GtkWidget * dialog;
	char * filename = NULL;
	gboolean ret;

	dialog = gtk_file_chooser_dialog_new(_("Save as..."),
			GTK_WINDOW(xmleditor->window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return FALSE;
	ret = xmleditor_save_as(xmleditor, filename);
	g_free(filename);
	return ret;
}


/* xmleditor_tag_set_name */
void xmleditor_tag_set_name(XMLEditor * xmleditor, GtkTreePath * treepath,
		char const * name)
{
	GtkTreeModel * model = GTK_TREE_MODEL(xmleditor->store);
	GtkTreeIter iter;

	xmleditor->modified = TRUE;
	gtk_tree_model_get_iter(model, &iter, treepath);
	gtk_tree_store_set(xmleditor->store, &iter, XEC_TAGNAME, name, -1);
}
