#include <gtk/gtk.h>

enum {
    COL_INDEX = 0,
    COL_ID,
    COL_NAME,
    COL_PATH,
    N_COLUMNS
};

void show_file_dialog(GtkButton *button,
                       gpointer   file_chooser_dialog)
{
    gtk_widget_show_all (GTK_WIDGET(file_chooser_dialog));
}

void hide_file_dialog(GtkButton *button,
                        gpointer file_chooser_dialog)
{
    gtk_widget_hide (GTK_WIDGET(file_chooser_dialog));
}

void hide_file_dialog_2 (GtkDialog *arg0,
                        gpointer file_chooser_dialog)
{
    g_print("escape pressed");
    gtk_widget_hide (GTK_WIDGET(file_chooser_dialog));
    g_print("escape pressed end");
}

void file_activated(GtkFileChooser *chooser,
                    gpointer liststore)
{
    gchar *filename;
    GtkTreeIter iter;
    filename = gtk_file_chooser_get_filename(chooser);
    g_print("%s\n", filename);
    gtk_list_store_append (liststore, &iter);
    gtk_list_store_set(liststore, &iter, COL_INDEX, 10, COL_NAME, filename, -1);
    gtk_widget_hide (GTK_WIDGET(chooser));
    /* g_free(filename); */
}

void print_everyting(gpointer data, gpointer user_data)
{
    g_print("%s\n", (char *)data);
}

void file_chooser_ok_button( GtkButton *button,
                        gpointer file_chooser)
{
    GSList *list;
    list = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (file_chooser));
    g_slist_foreach (list, print_everyting, NULL);
    g_slist_free(list);
    gtk_widget_hide (GTK_WIDGET(file_chooser));
}

int main(int argc, char *argv[])
{
    GtkBuilder *builder;
    GObject *button;
    GObject *window;
    GObject *liststore;
    GObject *file_chooser_dialog;
    gtk_init (&argc, &argv);
    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "cplayer.ui", NULL);
    window = gtk_builder_get_object (builder, "window1");
    liststore = gtk_builder_get_object (builder, "liststore1");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    button = gtk_builder_get_object (builder, "button5");
    file_chooser_dialog = gtk_builder_get_object (builder, "filechooserdialog1");
    g_signal_connect (button, "clicked", G_CALLBACK (show_file_dialog), file_chooser_dialog);
    button = gtk_builder_get_object (builder, "button8");
    g_signal_connect (button, "clicked", G_CALLBACK (hide_file_dialog), file_chooser_dialog);
    g_signal_connect_swapped (file_chooser_dialog, "response", G_CALLBACK (gtk_widget_destroy), file_chooser_dialog);
    g_signal_connect_swapped (file_chooser_dialog, "close", G_CALLBACK (hide_file_dialog_2), file_chooser_dialog);
    g_signal_connect (file_chooser_dialog, "file-activated", G_CALLBACK (file_activated), liststore);

    button = gtk_builder_get_object (builder, "button7");
    g_signal_connect(button, "clicked", G_CALLBACK(file_chooser_ok_button), file_chooser_dialog);


    gtk_main ();
    return 0;
}
