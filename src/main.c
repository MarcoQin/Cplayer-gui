#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>
#include <sys/stat.h>
#include "db.h"
#include "utils.h"
#include "player_core.h"

#define DIRECTION_DOWN 1
#define DIRECTION_UP 0
#define DIRECTION_CURRENT 2 /* not move the cursor, just play first song */

enum { COL_INDEX = 0, COL_ID, COL_NAME, COL_PATH, N_COLUMNS };

gpointer global_signal_handle_tree_view;
gpointer global_signal_handel_user_data;
gint global_slider_value;

pid_t parent_pid, child_pid, wpid;

void watch_dog();
void tree_view_scroll(gpointer tree_view, gint direction, gpointer user_data);

void stopping(int signum) {
    if (playing_status != STOP) {
        outfp = 0;
        tree_view_scroll(global_signal_handle_tree_view, DIRECTION_DOWN, global_signal_handel_user_data);
        watch_dog();
    }
}

void watch_dog() {
    if (child_pid) {
        kill(child_pid, SIGKILL);
    }
    child_pid = fork();
    if (child_pid < 0)
        exit(1);
    else if (child_pid == 0) {
        // child process
        int code = 0;
        parent_pid = getppid();
        usleep(100000);
        while (1) {
            code = kill(mplayer_pid, 0);
            if (code == 0) { // alive
                usleep(50000);
            } else {
                kill(parent_pid, SIGUSR1); // custom signal
                break;
            }
        }
        exit(1);
    } else {
        // parent process
        signal(SIGCHLD, SIG_IGN);
    }
}

static gpointer thread_func(gpointer user_data) {
    GObject *slider_adjustment = g_object_get_data(user_data, "slider_adjustment");
    GObject *time_label = g_object_get_data(user_data, "time_label");
    char buffer[1024];
    int percent_pos = 0;
    int i;
    int percent_pos_length = strlen("ANS_PERCENT_POSITION=");
    int ans_length = strlen("ANS_LENGTH=");
    int ans_time_position = strlen("ANS_TIME_POSITION=");
    int nbytes;
    int num_start;
    double time_length;
    double current_pos;
    char *time_string = (char *)malloc(15);
    while(1) {
        if (playing_status == STOP) {
            gtk_adjustment_set_value(GTK_ADJUSTMENT(slider_adjustment), 0);
            global_slider_value = 0;
            gtk_label_set_text(GTK_LABEL(time_label), "00:00 / 00:00");
        }
        if (outfp != 0) {
            if (playing_status == PLAYING) {
                /* current time pos */
                if (alive == ALIVE && is_alive() == ALIVE && playing_status == PLAYING && outfp != 0) {
                    get_time_pos();
                    while(1) {
                        nbytes = read(outfp, buffer, sizeof(buffer));
                        i = index_of(buffer, "Exit");
                        if (i != -1) {
                            break;
                        }
                        i = index_of(buffer, "ANS");
                        if(i != -1) {
                            i += ans_time_position;
                            char current_pos_num[nbytes - i];
                            num_start = 0;
                            while (buffer[i] != '\n') {
                                current_pos_num[num_start]  = buffer[i];
                                i++;
                                num_start ++;
                            }
                            current_pos = atof(current_pos_num);
                            break;
                        }
                    }
                    /* end */
                }
                g_usleep(20000);
                if (alive == ALIVE && is_alive() == ALIVE && playing_status == PLAYING && outfp != 0) {
                    /* get time percent pos */
                    get_time_percent_pos();
                    while(1) {
                        nbytes = read(outfp, buffer, sizeof(buffer));
                        i = index_of(buffer, "Exit");
                        if (i != -1) {
                            break;
                        }
                        i = index_of(buffer, "ANS");
                        if(i != -1) {
                            i += percent_pos_length;
                            char pos_num[nbytes - i];
                            num_start = 0;
                            while (buffer[i] != '\n') {
                                pos_num[num_start]  = buffer[i];
                                i++;
                                num_start ++;
                            }
                            percent_pos = atoi(pos_num);
                            break;
                        }
                    }
                    /* end */
                }
                g_usleep(20000);
                if (alive == ALIVE && is_alive() == ALIVE && playing_status == PLAYING && outfp != 0) {
                    /* get time length */
                    get_time_length();
                    while(1) {
                        nbytes = read(outfp, buffer, sizeof(buffer));
                        i = index_of(buffer, "Exit");
                        if (i != -1) {
                            break;
                        }
                        i = index_of(buffer, "ANS");
                        if(i != -1) {
                            i += ans_length;
                            char time_length_num[nbytes - i];
                            num_start = 0;
                            while (buffer[i] != '\n') {
                                time_length_num[num_start] = buffer[i];
                                i++;
                                num_start++;
                            }
                            time_length = atof(time_length_num);
                            break;
                        }
                    }
                    /* end */
                }

                song_time_to_str(time_string, time_length, current_pos);

                gtk_label_set_text(GTK_LABEL(time_label), time_string);
                gtk_adjustment_set_value(GTK_ADJUSTMENT(slider_adjustment), percent_pos);
                global_slider_value = percent_pos;
                g_usleep(1000000);
            }
        } else {
            g_usleep(200000);
        }
    }
    return (NULL);
}

void show_file_dialog(GtkButton *button, gpointer file_chooser_dialog) {
    gtk_widget_show_all(GTK_WIDGET(file_chooser_dialog));
}

void hide_file_dialog(GtkButton *button, gpointer file_chooser_dialog) {
    gtk_widget_hide(GTK_WIDGET(file_chooser_dialog));
}

void hide_file_dialog_2(GtkDialog *arg0, gpointer file_chooser_dialog) {
    gtk_widget_hide(GTK_WIDGET(file_chooser_dialog));
}

void add_song(gpointer file_path, gpointer liststore) {
    GtkTreeIter iter;
    gint n_rows = gtk_tree_model_iter_n_children(liststore, NULL);
    gtk_list_store_append(liststore, &iter);
    char *file_name = extract_file_name((char *)file_path);
    gint id = db_insert_song(file_name, (char *)file_path);
    gtk_list_store_set(liststore, &iter, COL_INDEX, (n_rows + 1), COL_NAME,
                       (gchar *)file_name, COL_PATH, (gchar *)file_path, COL_ID, id, -1);
    free(file_name);
}

void file_activated(GtkFileChooser *chooser, gpointer liststore) {
    gchar *file_path;
    file_path = gtk_file_chooser_get_filename(chooser);
    add_song(file_path, liststore);
    gtk_widget_hide(GTK_WIDGET(chooser));
    g_free(file_path);
}

void file_chooser_ok_button(GtkButton *button, GObject *user_data) {
    GSList *list;
    GObject *file_chooser = g_object_get_data(user_data, "file_chooser");
    GObject *liststore = g_object_get_data(user_data, "liststore");
    list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(file_chooser));
    g_slist_foreach(list, add_song, liststore);
    g_slist_free_full(list, g_free);
    gtk_widget_hide(GTK_WIDGET(file_chooser));
}

void selection_remove_foreach_fuc(GtkTreeModel *model, GtkTreePath *path,
                           GtkTreeIter *iter, GList **rowref_list) {
    gint id;
    gtk_tree_model_get(model, iter, COL_ID, &id, -1);
    if (id != 0) {
        db_delete_song(id);
        GtkTreeRowReference *rowref;
        rowref = gtk_tree_row_reference_new(model, path);
        *rowref_list = g_list_append(*rowref_list, rowref);
    }
}

gboolean re_index_func(GtkTreeModel *model, GtkTreePath *path,
                       GtkTreeIter *iter, gint *index) {
    *index += 1;
    gtk_list_store_set(GTK_LIST_STORE(model), iter, COL_INDEX, *index, -1);
    return FALSE; /* do not stop walking the store, call us with next row */
}

void remove_files(GtkButton *button, gpointer *tree_view) {
    GtkTreeModel *liststore = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    GList *rr_list = NULL; /* list of GtkTreeRowReferences to remove */
    GList *node;
    gtk_tree_selection_selected_foreach(
        selection, (GtkTreeSelectionForeachFunc)selection_remove_foreach_fuc,
        &rr_list);
    for (node = rr_list; node != NULL; node = node->next) {
        GtkTreePath *path;
        path =
            gtk_tree_row_reference_get_path((GtkTreeRowReference *)node->data);
        if (path) {
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(liststore), &iter,
                                        path)) {
                gtk_list_store_remove(GTK_LIST_STORE(liststore), &iter);
            }
        }
    }
    /* re-render all row's index */
    gint index = 0;
    gtk_tree_model_foreach(GTK_TREE_MODEL(liststore),
                           (GtkTreeModelForeachFunc)re_index_func, &index);

    g_list_foreach(rr_list, (GFunc)gtk_tree_row_reference_free, NULL);
    g_list_free(rr_list);
}

void slider_value_changed(GtkAdjustment *adjustment, gpointer *user_data) {
    gdouble value = gtk_adjustment_get_value(adjustment);
    if (abs(value - global_slider_value) > 5) {
        g_print("slider value: %d\n", (int)value);
        global_slider_value = value;
        seek(value);
    }
}

void update_play_button_label(GtkButton *button) {
    if (playing_status == PLAYING) {
        gtk_button_set_label(button, "▮▮");
    } else if (playing_status == PAUSE || playing_status == STOP) {
        gtk_button_set_label(button, "►");
    }
}

void update_play_state_label(GtkLabel *label, gchar *song_name) {
    gtk_label_set_text(GTK_LABEL(label), song_name);
}

void song_list_tree_view_row_activated(GtkTreeView *tree_view,
                                       GtkTreePath *path,
                                       GtkTreeViewColumn *column,
                                       gpointer user_data) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar *name, *file_path;
        gint id;
        gtk_tree_model_get(model, &iter, COL_ID, &id, COL_NAME, &name, COL_PATH,
                           &file_path, -1);

        GObject *play_button = g_object_get_data(user_data, "play_button");
        GObject *state_label = g_object_get_data(user_data, "state_label");
        load_song(id);
        watch_dog();
        update_play_button_label(GTK_BUTTON(play_button));
        update_play_state_label(GTK_LABEL(state_label), name);
        g_free(name);
        g_free(file_path);
    }
}

void tree_view_scroll(gpointer tree_view, gint direction, gpointer user_data) {
    GtkTreePath *path;
    GtkTreeIter iter;
    GtkTreeViewColumn *col;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(tree_view), &path, &col);
    gboolean first = FALSE;

    /* With NULL as iter, we get the number of toplevel nodes. */
    gint n_rows = gtk_tree_model_iter_n_children(model, NULL);

    /* App start, nothing is selected */
    if (path == NULL && n_rows != 0) {
        if (direction == DIRECTION_DOWN || direction == DIRECTION_CURRENT) {
            /* first node's path */
            gtk_tree_model_get_iter_first(model, &iter);
            path = gtk_tree_model_get_path(model, &iter);
        } else {
            /* get last node's path */
            path = gtk_tree_path_new_from_indices(n_rows - 1, -1);
        }
        first = TRUE;
    }
    /* move the cursor down or up */
    if (path != NULL) {
        if (first != TRUE) {
            if (direction == DIRECTION_DOWN) {
                gtk_tree_path_next(path);
            } else if (direction == DIRECTION_UP) {
                gboolean result = gtk_tree_path_prev(path);
                if (result != TRUE) {
                    path = gtk_tree_path_new_from_indices(n_rows - 1, -1);
                }
            }
        }
        /* when cursor is on the last row, called gtk_tree_path_next(path) will
         * not fail
         * and the path is not NULL, too. But the fact is that path is invalid
         * at all
         * so I have to verify weather the path is valid here */
        if (path != NULL) {
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path) ==
                FALSE) {
                gtk_tree_model_get_iter_first(model, &iter);
                path = gtk_tree_model_get_path(model, &iter);
            }
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree_view), path, NULL,
                                     FALSE);
            /* get song id to player, name to display, path to load */
            gchar *name, *file_path;
            gint id;
            gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
            gtk_tree_model_get(model, &iter, COL_ID, &id, COL_NAME, &name,
                               COL_PATH, &file_path, -1);

            load_song(id);
            watch_dog();
            GObject *play_button = g_object_get_data(user_data, "play_button");
            GObject *state_label = g_object_get_data(user_data, "state_label");
            update_play_button_label(GTK_BUTTON(play_button));
            update_play_state_label(GTK_LABEL(state_label), name);
            g_free(name);
            g_free(file_path);
            /* end */
        }
    }
    /* clean */
    gtk_tree_path_free(path);
}

void init_tree_view_data(gpointer liststore) {
    char **result = 0;
    int i, j, nrow, ncol, index, rc;
    char *errmsg;
    gint id;
    gchar *name;
    gchar *path;
    GtkTreeIter iter;
    rc = db_load_songs(&result, &nrow, &ncol, &errmsg);
    if (rc == 0) {
        index = ncol;
        for (i = 0; i < nrow; i++) {
            for (j = 0; j < ncol; j++) {
                if (j == 0)
                    id = atoi(result[index]);
                if (j == 1)
                    name = result[index];
                if (j == 2)
                    path = result[index];
                index ++;
            }
            gtk_list_store_append(GTK_LIST_STORE(liststore), &iter);
            gtk_list_store_set(GTK_LIST_STORE(liststore), &iter, COL_ID, id, COL_NAME, name, COL_PATH, path, COL_INDEX, (i+1), -1);
        }
        sqlite3_free_table(result);
    }
}

void next_button_pressed(GtkButton *button, gpointer user_data) {
    GObject *tree_view = g_object_get_data(user_data, "tree_view");
    tree_view_scroll(GTK_TREE_VIEW(tree_view), DIRECTION_DOWN, user_data);
}

void previous_button_pressed(GtkButton *button, gpointer user_data) {
    GObject *tree_view = g_object_get_data(user_data, "tree_view");
    tree_view_scroll(GTK_TREE_VIEW(tree_view), DIRECTION_UP, user_data);
}

void play_button_pressed(GtkButton *button, gpointer user_data) {
    if (playing_status == PLAYING || playing_status == PAUSE) {
        pause_song();
        update_play_button_label(button);
    } else {  /* row selected or no row selected */
        GObject *tree_view = g_object_get_data(user_data, "tree_view");
        tree_view_scroll(GTK_TREE_VIEW(tree_view), DIRECTION_CURRENT, user_data);
    }
}

void stop_button_pressed(GtkButton *button, gpointer play_button) {
    stop_song();
    update_play_button_label(play_button);
}

int main(int argc, char *argv[]) {
    GtkBuilder *builder;
    GObject *button;
    GObject *window;
    GObject *liststore;
    GObject *file_chooser_dialog;
    GObject *tree_view;
    GObject *slider_adjustment;
    GObject *state_label;
    GObject *time_label;
    gtk_init(&argc, &argv);
    builder = gtk_builder_new();
    /* gtk_builder_add_from_file(builder, "cplayer.ui", NULL); */
    gtk_builder_add_from_resource(builder, "/src/cplayer.ui", NULL);
    /* gtk_builder_add_from_resource(builder, "/home/marcoqin/marco/code/Cplayer-gui/src/cplayer.ui", NULL); */
    window = gtk_builder_get_object(builder, "window1"); /* main window */

    tree_view = gtk_builder_get_object(builder, "treeview1");

    liststore = gtk_builder_get_object(builder, "liststore1"); /* liststore */

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    file_chooser_dialog = gtk_builder_get_object( builder, "filechooserdialog1"); /* file chooser dialog */

    state_label = gtk_builder_get_object(builder, "label1"); /* play state label */

    button = gtk_builder_get_object(builder, "button5"); /* "+"(add files) button */
    g_signal_connect(button, "clicked", G_CALLBACK(show_file_dialog),
                     file_chooser_dialog);

    button = gtk_builder_get_object(
        builder, "button8"); /* filechooserdialog1 "cancel" button */
    g_signal_connect(button, "clicked", G_CALLBACK(hide_file_dialog),
                     file_chooser_dialog);

    g_signal_connect_swapped(file_chooser_dialog, "response",
                             G_CALLBACK(gtk_widget_destroy),
                             file_chooser_dialog);

    /* "Escape" pressed */
    g_signal_connect_swapped(file_chooser_dialog, "close",
                             G_CALLBACK(hide_file_dialog_2),
                             file_chooser_dialog);

    /* double clicked */
    g_signal_connect(file_chooser_dialog, "file-activated",
                     G_CALLBACK(file_activated), liststore);

    button = gtk_builder_get_object(
        builder, "button7"); /* filechooserdialog1 "ok" button */
    GtkWidget *user_data = gtk_label_new("this is a new lable");
    g_object_set_data(G_OBJECT(user_data), "file_chooser", file_chooser_dialog);
    g_object_set_data(G_OBJECT(user_data), "liststore", liststore);
    g_signal_connect(button, "clicked", G_CALLBACK(file_chooser_ok_button), user_data);

    button = gtk_builder_get_object(builder,
                                    "button6"); /* "-"(remove files) button */
    g_signal_connect(button, "clicked", G_CALLBACK(remove_files), tree_view);

    slider_adjustment = gtk_builder_get_object(builder, "adjustment2");
    g_signal_connect(slider_adjustment, "value-changed",
                     G_CALLBACK(slider_value_changed), NULL);

    GObject *play_button = gtk_builder_get_object(builder, "button1"); /* "play" button */

    g_object_set_data(G_OBJECT(user_data), "play_button", play_button);
    g_object_set_data(G_OBJECT(user_data), "state_label", state_label);
    g_signal_connect(tree_view, "row-activated",
                     G_CALLBACK(song_list_tree_view_row_activated), user_data);

    g_object_set_data(G_OBJECT(user_data), "tree_view", tree_view);
    button = gtk_builder_get_object(builder, "button4"); /* "next" button */
    g_signal_connect(button, "clicked", G_CALLBACK(next_button_pressed),
                     user_data);

    button = gtk_builder_get_object(builder, "button3"); /* "previous" button */
    g_signal_connect(button, "clicked", G_CALLBACK(previous_button_pressed),
                     user_data);

    button = gtk_builder_get_object(builder, "button1"); /* "play" button */
    g_signal_connect(button, "clicked", G_CALLBACK(play_button_pressed),
                     user_data);

    button = gtk_builder_get_object(builder, "button2"); /* "stop" button */
    g_signal_connect(button, "clicked", G_CALLBACK(stop_button_pressed),
                     play_button);


    db_enable();

    const char *homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    char *db_path = merge_str((char *)homedir, "/.cplayer/", "songs.db");
    char *main_path = merge_str((char *)homedir, "/.cplayer", "/");

    if (access(db_path, F_OK) == -1) {
        // file doesn't exist, create one
        int status = mkdir(main_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    int rc = db_init(db_path);
    free(db_path);
    free(main_path);
    if (rc != 0) {
        fprintf(stderr, "Fail to init db.\n");
        exit(1);
    }
    init_tree_view_data(liststore);

    /* handel signal */
    global_signal_handle_tree_view = tree_view;
    global_signal_handel_user_data = user_data;
    signal(SIGUSR1, stopping);
    /* end */

    /* thread */
    time_label = gtk_builder_get_object(builder, "label2"); /* time screen */
    g_object_set_data(G_OBJECT(user_data), "slider_adjustment", slider_adjustment);
    g_object_set_data(G_OBJECT(user_data), "time_label", time_label);
    GError *error = NULL;
    GThread *thread;
    thread = g_thread_try_new("thread1", thread_func, (gpointer)user_data, &error);
    if (!thread) {
        g_print("Error: %s\n", error->message);
        return (-1);
    }
    /* end thread */

    gtk_main();
    db_close();
    db_disable();
    free_player();
    return 0;
}
