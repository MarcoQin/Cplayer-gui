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
    gtk_list_store_append(liststore, &iter);
    gtk_list_store_set(liststore, &iter, COL_INDEX, (n_rows + 1), COL_NAME,
                       (gchar *)file_path, -1);
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

int main(int argc, char *argv[]) {
    GtkBuilder *builder;
    GObject *button;
    GObject *window;
    GObject *liststore;
    GObject *file_chooser_dialog;
    gtk_init(&argc, &argv);
    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "cplayer.ui", NULL);
    window = gtk_builder_get_object(builder, "window1");  /* main window */

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

    gtk_main();
    return 0;
}
