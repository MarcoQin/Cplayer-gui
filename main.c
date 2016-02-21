#include <gtk/gtk.h>

void show_file_dialog(GtkButton *button,
                       gpointer   file_chooser)
{
    gtk_widget_show_all (GTK_WIDGET(file_chooser));
}

void hide_file_dialog(GtkButton *button,
                        gpointer file_chooser)
{
    gtk_widget_hide (GTK_WIDGET(file_chooser));
}

void hide_file_dialog_2 (GtkDialog *arg0,
                        gpointer file_chooser)
{
    g_print("escape pressed");
    gtk_widget_hide (GTK_WIDGET(file_chooser));
    g_print("escape pressed end");
}

int main(int argc, char *argv[])
{
    GtkBuilder *builder;
    GObject *button;
    GObject *window;
    GObject *file_chooser;
    gtk_init (&argc, &argv);
    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "cplayer.ui", NULL);
    window = gtk_builder_get_object (builder, "window1");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    button = gtk_builder_get_object (builder, "button5");
    file_chooser = gtk_builder_get_object (builder, "filechooserdialog1");
    g_signal_connect (button, "clicked", G_CALLBACK (show_file_dialog), file_chooser);
    button = gtk_builder_get_object (builder, "button8");
    g_signal_connect (button, "clicked", G_CALLBACK (hide_file_dialog), file_chooser);
    g_signal_connect_swapped (file_chooser, "response", G_CALLBACK (gtk_widget_destroy), file_chooser);
    g_signal_connect_swapped (file_chooser, "close", G_CALLBACK (hide_file_dialog_2), file_chooser);
    gtk_main ();
    return 0;
}
