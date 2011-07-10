/*************************************************************************/
/* Copyright (C) 2007-2009 sujith <m.sujith@gmail.com>			 */
/* Copyright (C) 2009-2010 matias <mati86dl@gmail.com>			 */
/* 									 */
/* This program is free software: you can redistribute it and/or modify	 */
/* it under the terms of the GNU General Public License as published by	 */
/* the Free Software Foundation, either version 3 of the License, or	 */
/* (at your option) any later version.					 */
/* 									 */
/* This program is distributed in the hope that it will be useful,	 */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	 */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	 */
/* GNU General Public License for more details.				 */
/* 									 */
/* You should have received a copy of the GNU General Public License	 */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*************************************************************************/

#include "pragha.h"

/* Get the first image file from the directory and create a pixbuf of that file */

static GdkPixbuf* get_image_from_dir(gchar *path, struct con_win *cwin)
{
	GdkPixbuf *image = NULL;
	GError *error = NULL;
	GDir *dir = NULL;
	const gchar *next_file = NULL;
	gchar *ab_file = NULL;

	dir = g_dir_open(path, 0, &error);
	if (!dir) {
		g_critical("Unable to open dir: %s", path);
		g_error_free(error);
		return NULL;
	}

	next_file = g_dir_read_name(dir);
	while (next_file) {
		ab_file = g_strconcat(path, "/", next_file, NULL);
		if (g_file_test(ab_file, G_FILE_TEST_IS_REGULAR) &&
		    is_image_file(ab_file)) {
			CDEBUG(DBG_INFO, "Image file: %s", ab_file);
			image = gdk_pixbuf_new_from_file_at_scale(ab_file,
								cwin->cpref->album_art_size,
								cwin->cpref->album_art_size,
								FALSE,
							  	&error);
			if (!image) {
				g_critical("Unable to open image file: %s",
					   ab_file);
				g_error_free(error);
			}
			g_free(ab_file);
			break;
		}
		g_free(ab_file);
		next_file = g_dir_read_name(dir);
	}
	g_dir_close(dir);

	return image;
}

/* Find out if any of the preferred album art files are present in the given dir.
   Runs through the patterns in sequence */

static GdkPixbuf* get_pref_image_dir(gchar *path, struct con_win *cwin)
{
	GdkPixbuf *image = NULL;
	GError *error = NULL;
	GDir *dir = NULL;
	const gchar *next_file = NULL;
	gchar *ab_file = NULL, **pattern;
	GSList *file_list = NULL, *l;
	gint i = 0;

	/* Form a list of all files in the given dir */

	dir = g_dir_open(path, 0, &error);
	if (!dir) {
		g_critical("Unable to open dir: %s", path);
		g_error_free(error);
		return NULL;
	}

	next_file = g_dir_read_name(dir);
	while (next_file) {
		ab_file = g_strconcat(path, "/", next_file, NULL);
		if (g_file_test(ab_file, G_FILE_TEST_IS_REGULAR)) {
			file_list = g_slist_append(file_list, g_strdup(next_file));

		}
		g_free(ab_file);
		next_file = g_dir_read_name(dir);
	}
	g_dir_close(dir);

	/* Now, run the preferred patterns through them */

	pattern = g_strsplit(cwin->cpref->album_art_pattern, ";",
			     ALBUM_ART_NO_PATTERNS);
	while (pattern[i]) {
		if (is_present_str_list(pattern[i], file_list)) {
			ab_file = g_strconcat(path, "/", pattern[i], NULL);
			if (is_image_file(ab_file)) {
				CDEBUG(DBG_INFO, "Image file: %s", ab_file);
				image = gdk_pixbuf_new_from_file_at_scale(ab_file,
							  cwin->cpref->album_art_size,
							  cwin->cpref->album_art_size,
							  FALSE,
							  &error);
				if (!image) {
					g_critical("Unable to open image file: %s\n",
						   ab_file);
					g_error_free(error);
					g_free(ab_file);
					i++;
					continue;
				}
				g_free(ab_file);
				break;
			}
			g_free(ab_file);
		}
		i++;
	}

	/* Cleanup */

	l = file_list;
	while (l) {
		g_free(l->data);
		l = l->next;
	}
	g_slist_free(file_list);
	g_strfreev(pattern);

	return image;
}

void __update_progress_song_info(struct con_win *cwin, gint length)
{
	gchar *tot_length = NULL, *cur_pos = NULL, *str_length = NULL, *str_cur_pos = NULL;

	if (!cwin->cstate->curr_mobj) {
		g_critical("Curr mobj is invalid");
		return;
	}

	cur_pos = convert_length_str(length);
	str_cur_pos = g_markup_printf_escaped ("<small>%s</small>", cur_pos);
	gtk_label_set_markup (GTK_LABEL(cwin->track_time_label), (const gchar*)str_cur_pos);

	if(cwin->cstate->curr_mobj->tags->length == 0 || !cwin->cpref->timer_remaining_mode){
		tot_length = convert_length_str(cwin->cstate->curr_mobj->tags->length);
		str_length = g_markup_printf_escaped ("<small>%s</small>", tot_length);
	}
	else{
		tot_length = convert_length_str(cwin->cstate->curr_mobj->tags->length - length);
		str_length = g_markup_printf_escaped ("<small>- %s</small>", tot_length);
	}
	gtk_label_set_markup (GTK_LABEL(cwin->track_length_label), (const gchar*)str_length);

	gtk_tooltip_trigger_tooltip_query(gtk_widget_get_display (cwin->track_length_label));

	g_free(cur_pos);
	g_free(str_cur_pos);

	g_free(tot_length);
	g_free(str_length);
}

void __update_current_song_info(struct con_win *cwin)
{
	gchar *str = NULL, *str_title = NULL;

	if (!cwin->cstate->curr_mobj) {
		g_critical("Curr mobj is invalid");
		return;
	}

	if(g_utf8_strlen(cwin->cstate->curr_mobj->tags->title, -1))
		str_title = g_strdup(cwin->cstate->curr_mobj->tags->title);
	else
		str_title = get_display_filename(cwin->cstate->curr_mobj->file, FALSE);

	if(g_utf8_strlen(cwin->cstate->curr_mobj->tags->artist, -1)
	 && g_utf8_strlen(cwin->cstate->curr_mobj->tags->album, -1))
		str = g_markup_printf_escaped (_("%s <small><span weight=\"light\">by</span></small> %s <small><span weight=\"light\">in</span></small> %s"), 
						str_title ,
						cwin->cstate->curr_mobj->tags->artist, 
						cwin->cstate->curr_mobj->tags->album);
	else if(g_utf8_strlen(cwin->cstate->curr_mobj->tags->artist, -1))
		str = g_markup_printf_escaped (_("%s <small><span weight=\"light\">by</span></small> %s"), 
						str_title ,
						cwin->cstate->curr_mobj->tags->artist);
	else if(g_utf8_strlen(cwin->cstate->curr_mobj->tags->album, -1))
		str = g_markup_printf_escaped (_("%s <small><span weight=\"light\">in</span></small> %s"), 
						str_title ,
						cwin->cstate->curr_mobj->tags->album);
	else	str = g_markup_printf_escaped ("%s", str_title);

	gtk_label_set_markup(GTK_LABEL(cwin->now_playing_label), (const gchar*)str);

	g_free(str);
	g_free(str_title);
}

void unset_current_song_info(struct con_win *cwin)
{
	gtk_label_set_markup(GTK_LABEL(cwin->now_playing_label),
				  _("<b>Not playing</b>"));
	gtk_label_set_markup(GTK_LABEL(cwin->track_length_label),"<small>--:--</small>");
	gtk_label_set_markup(GTK_LABEL(cwin->track_time_label),"<small>00:00</small>");

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(cwin->track_progress_bar), 0);
}

void __update_track_progress_bar(struct con_win *cwin, gint length)
{
	gdouble fraction = 0;

	if(cwin->cstate->curr_mobj->tags->length == 0) {
		cwin->cstate->curr_mobj->tags->length = GST_TIME_AS_SECONDS(backend_get_current_length(cwin));
	}
	else {
		fraction = (gdouble)length / (gdouble)cwin->cstate->curr_mobj->tags->length;

		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(cwin->track_progress_bar),
					      fraction);
	}
}

void unset_track_progress_bar(struct con_win *cwin)
{
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(cwin->track_progress_bar), 0);
}

void timer_remaining_mode_change(GtkWidget *w, GdkEventButton* event, struct con_win *cwin)
{
	if(cwin->cpref->timer_remaining_mode)
		cwin->cpref->timer_remaining_mode = FALSE;
	else
		cwin->cpref->timer_remaining_mode = TRUE;
	if(cwin->cstate->state != ST_STOPPED)
		update_current_song_info(cwin);
}

void track_progress_change_cb(GtkWidget *widget,
			      GdkEventButton *event,
			      struct con_win *cwin)
{
	gint seek = 0;
	gdouble fraction = 0;

	if (event->button != 1)
		return;

	if (cwin->cstate->state != ST_PLAYING)
		return;

	if (!cwin->cstate->curr_mobj || cwin->cstate->curr_mobj->tags->length == 0)
		return;

	seek = (cwin->cstate->curr_mobj->tags->length * event->x) / widget->allocation.width;
	if (seek >= cwin->cstate->curr_mobj->tags->length)
		seek = cwin->cstate->curr_mobj->tags->length;

	fraction = (gdouble) event->x / widget->allocation.width;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(cwin->track_progress_bar), fraction);

	backend_seek(seek, cwin);
}

void update_album_art(struct musicobject *mobj, struct con_win *cwin)
{
	CDEBUG(DBG_INFO, "Update album art");

	GError *error = NULL;
	GdkPixbuf *scaled_album_art, *album_art, *scaled_frame, *frame;
	gchar *dir;

	if (cwin->cpref->show_album_art) {
		frame = gdk_pixbuf_new_from_file (PIXMAPDIR"/cover.png", &error);

		if (mobj && mobj->file_type != FILE_CDDA){
			dir = g_path_get_dirname(mobj->file);
			if (cwin->cpref->album_art_pattern){
				album_art = get_pref_image_dir(dir, cwin);
				if (!album_art)
					album_art = get_image_from_dir(dir, cwin);
			}
			else album_art = get_image_from_dir(dir, cwin);

			if (album_art) {
				scaled_album_art = gdk_pixbuf_scale_simple (album_art, 112, 112, GDK_INTERP_BILINEAR);
				gdk_pixbuf_copy_area(scaled_album_art, 0 ,0 ,112 ,112, frame, 12, 8);
				g_object_unref(G_OBJECT(scaled_album_art));
				g_object_unref(G_OBJECT(album_art));
			}
			g_free(dir);
		}

		scaled_frame = gdk_pixbuf_scale_simple (frame,
							cwin->cpref->album_art_size,
							cwin->cpref->album_art_size,
							GDK_INTERP_BILINEAR);

		if (cwin->album_art) {
			gtk_image_clear(GTK_IMAGE(cwin->album_art));
			gtk_image_set_from_pixbuf(GTK_IMAGE(cwin->album_art), scaled_frame);
		}
		else {
			cwin->album_art = gtk_image_new_from_pixbuf(scaled_frame);
			gtk_container_add(GTK_CONTAINER(cwin->album_art_frame),
					  GTK_WIDGET(cwin->album_art));
			gtk_widget_show_all(cwin->album_art_frame);
		}

		g_object_unref(G_OBJECT(scaled_frame));
		g_object_unref(G_OBJECT(frame));
	}
}

/* Unset album art */

void unset_album_art(struct con_win *cwin)
{
	GError *error = NULL;
	GdkPixbuf *cover;

	if (cwin->cpref->show_album_art){
		cover = gdk_pixbuf_new_from_file_at_size (PIXMAPDIR"/cover.png",
							cwin->cpref->album_art_size,
							cwin->cpref->album_art_size,
							&error);
		if (cwin->album_art) {
			gtk_image_clear(GTK_IMAGE(cwin->album_art));
			gtk_image_set_from_pixbuf(GTK_IMAGE(cwin->album_art), cover);
		}
		else {
			cwin->album_art = gtk_image_new_from_pixbuf(cover);
			gtk_container_add(GTK_CONTAINER(cwin->album_art_frame),
					  GTK_WIDGET(cwin->album_art));
			gtk_widget_show_all(cwin->album_art_frame);
		}

		g_object_unref(G_OBJECT(cover));
	}
}

/* Handler for the 'Leave fullscren' button item in Panel */

void
unfull_button_handler (GtkButton *button, struct con_win *cwin)
{
	GtkAction *action_fullscreen;

	action_fullscreen = gtk_ui_manager_get_action(cwin->bar_context_menu, "/Menubar/ViewMenu/Fullscreen");

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_fullscreen), FALSE);

}

/* Handler for the 'Shuffle' button item in Panel */

void
shuffle_button_handler (GtkToggleButton *button, struct con_win *cwin)
{
	GtkAction *action_shuffle;

	cwin->cpref->shuffle = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	action_shuffle = gtk_ui_manager_get_action(cwin->bar_context_menu, "/Menubar/EditMenu/Shuffle");

	g_signal_handlers_block_by_func (action_shuffle, shuffle_action, cwin);

		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_shuffle), cwin->cpref->shuffle);
		shuffle_button(cwin);

	g_signal_handlers_unblock_by_func (action_shuffle, shuffle_action, cwin);

	dbus_send_signal(DBUS_EVENT_UPDATE_STATE, cwin);
}

void
repeat_button_handler (GtkToggleButton *button, struct con_win *cwin)
{
	GtkAction *action_repeat;
	action_repeat = gtk_ui_manager_get_action(cwin->bar_context_menu,"/Menubar/EditMenu/Repeat");

	cwin->cpref->repeat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	g_signal_handlers_block_by_func (action_repeat, repeat_action, cwin);

		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_repeat), cwin->cpref->repeat);

	g_signal_handlers_unblock_by_func (action_repeat, repeat_action, cwin);

	dbus_send_signal(DBUS_EVENT_UPDATE_STATE, cwin);
}

void shuffle_button (struct con_win *cwin)
{
	GtkTreeRowReference *ref;

	if(cwin->cstate->tracks_curr_playlist){
		current_playlist_clear_dirty_all(cwin);

 		if (!cwin->cpref->shuffle) {
			CDEBUG(DBG_INFO, "Turning shuffle off");
			cwin->cstate->unplayed_tracks = cwin->cstate->tracks_curr_playlist;
			if (cwin->cstate->curr_rand_ref)
				cwin->cstate->curr_seq_ref =
					gtk_tree_row_reference_copy(cwin->cstate->curr_rand_ref);
			else
				cwin->cstate->curr_seq_ref = NULL;
		}
		else if (cwin->cpref->shuffle) {
			CDEBUG(DBG_INFO, "Turning shuffle on");
			if (cwin->cstate->curr_seq_ref) {
				ref = gtk_tree_row_reference_copy(cwin->cstate->curr_seq_ref);
				cwin->cstate->unplayed_tracks = cwin->cstate->tracks_curr_playlist - 1;
				reset_rand_track_refs(ref, cwin);
			}
		}
	}
}

void keybind_play_handler (const char *keystring, gpointer data)
{
	struct con_win *cwin = data;
	play_pause_resume(cwin);
}

void play_button_handler(GtkButton *button, struct con_win *cwin)
{
	play_pause_resume(cwin);
}

void play_pause_resume(struct con_win *cwin)
{
	struct musicobject *mobj = NULL;
	GtkTreePath *path=NULL;
	GtkTreeModel *model;
	GtkTreeRowReference *ref;

	/* New action is based on the current state */

	/************************************/
        /* State     Action		    */
	/* 				    */
	/* Playing   Pause playback	    */
	/* Paused    Resume playback	    */
	/* Stopped   Start playback	    */
        /************************************/

	switch (cwin->cstate->state) {
	case ST_PLAYING:
		backend_pause(cwin);
		break;
	case ST_PAUSED:
		backend_resume(cwin);
		break;
	case ST_STOPPED:
		if(cwin->cstate->queue_track_refs)
			path = get_next_queue_track(cwin);
		if (!path)
			path = current_playlist_get_selection(cwin);
		if (!path) {
			play_first_current_playlist(cwin);
			break;
		}
		if (cwin->cpref->shuffle) {
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(cwin->current_playlist));
			ref = gtk_tree_row_reference_new(model, path);
			reset_rand_track_refs(ref, cwin);
			cwin->cstate->unplayed_tracks = cwin->cstate->tracks_curr_playlist;
		}

		mobj = current_playlist_mobj_at_path(path, cwin);

		backend_start(mobj, cwin);
		update_current_state(path, PLAYLIST_CURR, cwin);

		gtk_tree_path_free(path);
		break;
	default:
		break;
	}
}

void keybind_stop_handler (const char *keystring, gpointer data)
{
	struct con_win *cwin = data;
	backend_stop(NULL, cwin);
}

void stop_button_handler(GtkButton *button, struct con_win *cwin)
{
	backend_stop(NULL, cwin);
}

void keybind_prev_handler (const char *keystring, gpointer data)
{
	struct con_win *cwin = data;
	play_prev_track(cwin);
}

void prev_button_handler(GtkButton *button, struct con_win *cwin)
{
	play_prev_track(cwin);
}

void keybind_next_handler (const char *keystring, gpointer data)
{
	struct con_win *cwin = data;
	play_next_track(cwin);
}

void next_button_handler(GtkButton *button, struct con_win *cwin)
{
	play_next_track(cwin);
	gtk_widget_grab_focus(cwin->current_playlist);
}

void keybind_media_handler (const char *keystring, gpointer data)
{
	struct con_win *cwin = data;
	toogle_main_window (cwin, FALSE);
}

void toggled_cb(GtkToggleButton *toggle, struct con_win *cwin)
{
	GtkAction *action_lib, *null_action_lib, *action_playlists, *null_action_playlists;

	action_lib = gtk_ui_manager_get_action(cwin->bar_context_menu,"/Menubar/ViewMenu/Lateral panel/Library");
	null_action_lib = gtk_ui_manager_get_action(cwin->cp_null_context_menu, "/popup/Library");

	action_playlists = gtk_ui_manager_get_action(cwin->bar_context_menu,"/Menubar/ViewMenu/Lateral panel/Playlists");
	null_action_playlists = gtk_ui_manager_get_action(cwin->cp_null_context_menu, "/popup/Playlists");

	g_signal_handlers_block_by_func (action_lib, library_pane_action, cwin);
	g_signal_handlers_block_by_func (null_action_lib, library_pane_action, cwin);
	g_signal_handlers_block_by_func (cwin->toggle_lib, toggled_cb, cwin);

	g_signal_handlers_block_by_func (action_playlists, playlists_pane_action, cwin);
	g_signal_handlers_block_by_func (null_action_playlists, playlists_pane_action, cwin);
	g_signal_handlers_block_by_func (cwin->toggle_playlists, toggled_cb, cwin);

	if ((GTK_TOGGLE_BUTTON(toggle) == GTK_TOGGLE_BUTTON(cwin->toggle_lib)) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle))) {
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_lib), TRUE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (null_action_lib), TRUE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_playlists) ,FALSE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (null_action_playlists) ,FALSE);
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(cwin->toggle_playlists), FALSE);
		gtk_widget_show (GTK_WIDGET(cwin->browse_mode));
		gtk_notebook_set_current_page(GTK_NOTEBOOK(cwin->browse_mode), 0);
		gtk_widget_grab_focus(cwin->library_tree);
	}
	else if ((GTK_TOGGLE_BUTTON(toggle) == GTK_TOGGLE_BUTTON(cwin->toggle_playlists)) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle))) {
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_playlists), TRUE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (null_action_playlists), TRUE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_lib), FALSE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (null_action_lib), FALSE);
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(cwin->toggle_lib), FALSE);
		gtk_widget_show (GTK_WIDGET(cwin->browse_mode));
		gtk_notebook_set_current_page(GTK_NOTEBOOK(cwin->browse_mode), 1);
		gtk_widget_grab_focus(cwin->playlist_tree);
	}
	else {
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_lib), FALSE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (null_action_lib), FALSE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_playlists), FALSE);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (null_action_playlists), FALSE);
		gtk_widget_hide (GTK_WIDGET(cwin->browse_mode));
		gtk_widget_grab_focus(cwin->current_playlist);
	}

	g_signal_handlers_unblock_by_func (action_lib, library_pane_action, cwin);
	g_signal_handlers_unblock_by_func (null_action_lib, library_pane_action, cwin);
	g_signal_handlers_unblock_by_func (cwin->toggle_lib, toggled_cb, cwin);

	g_signal_handlers_unblock_by_func (action_playlists, playlists_pane_action, cwin);
	g_signal_handlers_unblock_by_func (null_action_playlists, playlists_pane_action, cwin);
	g_signal_handlers_unblock_by_func (cwin->toggle_playlists, toggled_cb, cwin);
}

void vol_button_handler(GtkScaleButton *button, gdouble value, struct con_win *cwin)
{
	cwin->cgst->curr_vol = value / 100;
	backend_update_volume(cwin);
}

void play_button_toggle_state(struct con_win *cwin)
{
	if (cwin->cstate->state == ST_PLAYING)
		gtk_button_set_image(GTK_BUTTON(cwin->play_button),
				     cwin->pixbuf->image_pause);
	else if ((cwin->cstate->state == ST_PAUSED) ||
		 (cwin->cstate->state == ST_STOPPED))
		gtk_button_set_image(GTK_BUTTON(cwin->play_button),
				     cwin->pixbuf->image_play);
}

/* Toggle appearance of album art widget */

void album_art_toggle_state(struct con_win *cwin)
{
	CDEBUG(DBG_INFO, "Toggle state of album art");

	if (cwin->cpref->show_album_art) {
		if (!cwin->album_art_frame) {
			cwin->album_art_frame = gtk_frame_new(NULL);
			gtk_frame_set_shadow_type (GTK_FRAME(cwin->album_art_frame), GTK_SHADOW_NONE);
			gtk_box_pack_end(GTK_BOX(cwin->hbox_panel),
					   GTK_WIDGET(cwin->album_art_frame),
					   FALSE, FALSE, 0);
			gtk_box_reorder_child(GTK_BOX(cwin->hbox_panel),
					      cwin->album_art_frame,
					      2);
		}
		gtk_widget_show_now(cwin->album_art_frame);
		resize_album_art_frame(cwin);
		if (cwin->cstate->state != ST_STOPPED)
			update_album_art(cwin->cstate->curr_mobj, cwin);
		else unset_album_art(cwin);
	}
	else
		if (cwin->album_art_frame)
			gtk_widget_hide(cwin->album_art_frame);
}

/* Set the initial width of the frame to the default height */

void resize_album_art_frame(struct con_win *cwin)
{
	if (cwin->album_art_frame && cwin->cpref->show_album_art){

		gtk_widget_set_size_request(GTK_WIDGET(cwin->album_art_frame),
					    cwin->cpref->album_art_size+2,
					    cwin->cpref->album_art_size+2);

	}
}
