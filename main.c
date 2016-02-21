#include <gtk/gtk.h>

GObject *file_chooser;
void show_file_dialog(GtkButton *button,
                       gpointer   user_data)
{
    gtk_widget_show_all (GTK_WIDGET(file_chooser));
}

int main(int argc, char *argv[])
{
    GtkBuilder *builder;
    GObject *button;
    GObject *window;
    gtk_init (&argc, &argv);
    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "cplayer.ui", NULL);
    window = gtk_builder_get_object (builder, "window1");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    button = gtk_builder_get_object (builder, "button5");
    file_chooser = gtk_builder_get_object (builder, "filechooserdialog1");
    g_signal_connect (button, "clicked", G_CALLBACK (show_file_dialog), NULL);
    gtk_main ();
    return 0;
}
