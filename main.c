#include <gtk/gtk.h>

enum { COL_INDEX = 0, COL_ID, COL_NAME, COL_PATH, N_COLUMNS };

void show_file_dialog(GtkButton *button, gpointer file_chooser_dialog) {
    gtk_widget_show_all(GTK_WIDGET(file_chooser_dialog));
}

void hide_file_dialog(GtkButton *button, gpointer file_chooser_dialog) {
    gtk_widget_hide(GTK_WIDGET(file_chooser_dialog));
}

void hide_file_dialog_2(GtkDialog *arg0, gpointer file_chooser_dialog) {
    g_print("escape pressed");
    gtk_widget_hide(GTK_WIDGET(file_chooser_dialog));
    g_print("escape pressed end");
}

void add_song(gpointer file_path, gpointer liststore) {
    g_print("%s\n", (char *)file_path);
    GtkTreeIter iter;
    gint n_rows = gtk_tree_model_iter_n_children(liststore, NULL);
    g_print("n_rows%d\n", n_rows);
    /* TODO: insert into sqlite3 database */
    gtk_list_store_append(liststore, &iter);
    gtk_list_store_set(liststore, &iter, COL_INDEX, (n_rows + 1), COL_NAME,
                       (gchar *)file_path, COL_ID, (n_rows + 1), -1);
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

void selection_foreach_fuc( GtkTreeModel *model,
                            GtkTreePath  *path,
                            GtkTreeIter  *iter,
                            GList       **rowref_list) {
    gint id;
    gtk_tree_model_get (model, iter, COL_ID, &id, -1);
    g_print("id is %d\n", id);
    if (id != 0)
    {
        /* TODO: remove from sqlite3 database */
        GtkTreeRowReference  *rowref;
        rowref = gtk_tree_row_reference_new(model, path);
        *rowref_list = g_list_append(*rowref_list, rowref);
    }
}

gboolean re_index_func (GtkTreeModel *model,
                        GtkTreePath  *path,
                        GtkTreeIter  *iter,
                        gint *index) {
    *index += 1;
    gtk_list_store_set(GTK_LIST_STORE(model), iter, COL_INDEX, *index, -1);
    return FALSE; /* do not stop walking the store, call us with next row */
}

void remove_files(GtkButton *button, gpointer *tree_view) {
    GtkTreeModel *liststore = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    GList *rr_list = NULL;    /* list of GtkTreeRowReferences to remove */
    GList *node;
    gtk_tree_selection_selected_foreach(selection,
                                        (GtkTreeSelectionForeachFunc) selection_foreach_fuc,
                                        &rr_list);
    for (node = rr_list; node != NULL; node = node->next)
    {
        GtkTreePath *path;
        path = gtk_tree_row_reference_get_path((GtkTreeRowReference *)node->data);
        if (path)
        {
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(liststore), &iter, path))
            {
                gtk_list_store_remove(GTK_LIST_STORE(liststore), &iter);
            }
        }
    }
    /* re-render all row's index */
    gint index = 0;
    gtk_tree_model_foreach(GTK_TREE_MODEL(liststore),
            (GtkTreeModelForeachFunc) re_index_func,
            &index);

    g_list_foreach(rr_list, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free(rr_list);
}

void slider_value_changed(GtkAdjustment *adjustment,
                          gpointer *user_data) {
    gdouble value = gtk_adjustment_get_value(adjustment);
    g_print("slider value: %f\n", value);
}

int main(int argc, char *argv[]) {
    GtkBuilder *builder;
    GObject *button;
    GObject *window;
    GObject *liststore;
    GObject *file_chooser_dialog;
    GObject *treeview;
    GObject *slider_adjustment;
    gtk_init(&argc, &argv);
    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "cplayer.ui", NULL);
    window = gtk_builder_get_object(builder, "window1");  /* main window */

    treeview = gtk_builder_get_object(builder, "treeview1");

    liststore = gtk_builder_get_object(builder, "liststore1");  /* liststore */

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    file_chooser_dialog = gtk_builder_get_object(builder, "filechooserdialog1");  /* file chooser dialog */

    button = gtk_builder_get_object(builder, "button5");  /* "+"(add files) button */
    g_signal_connect(button, "clicked", G_CALLBACK(show_file_dialog),
                     file_chooser_dialog);

    button = gtk_builder_get_object(builder, "button8");  /* filechooserdialog1 "cancel" button */
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

    button = gtk_builder_get_object(builder, "button7");  /* filechooserdialog1 "ok" button */
    GtkWidget *w = gtk_label_new("this is a new lable");
    g_object_set_data(G_OBJECT(w), "file_chooser", file_chooser_dialog);
    g_object_set_data(G_OBJECT(w), "liststore", liststore);
    g_signal_connect(button, "clicked", G_CALLBACK(file_chooser_ok_button), w);

    button = gtk_builder_get_object(builder, "button6");  /* "-"(remove files) button */
    g_signal_connect(button, "clicked", G_CALLBACK(remove_files), treeview);

    slider_adjustment = gtk_builder_get_object(builder, "adjustment2");
    g_signal_connect(slider_adjustment, "value-changed", G_CALLBACK(slider_value_changed), NULL);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(slider_adjustment), 35.0);

    gtk_main();
    return 0;
}
