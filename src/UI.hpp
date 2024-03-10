#include <gtk/gtk.h>

char* openFileDialog() {
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    gtk_init(NULL, NULL);

    dialog = gtk_file_chooser_dialog_new("Open File",
                                         NULL,
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    char* filename = NULL;
    if (res == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
    }

    gtk_widget_destroy(dialog);
    while (g_main_context_iteration(NULL, FALSE));
    return filename;
}
