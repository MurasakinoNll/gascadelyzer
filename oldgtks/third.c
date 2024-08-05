#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#define MAXLEN 1024
#define DEFAULT_FILE_PATH "/home/harak/harakdev/gascade/"
#define SEARCH_SUBJECT "Pillars used"

int count_oc_file(FILE *file, const char *search_subject_str) {
    char line[MAXLEN];
    int exo_count = 0;

    while (fgets(line, sizeof(line), file)) {
        char *pos = line;
        while ((pos = strstr(pos, search_subject_str)) != NULL) {
            exo_count++;
            pos += strlen(search_subject_str);
        }
    }
    return exo_count;
}

GtkWidget *file_path_entry;
GtkWidget *output_label;

static void on_button_pressed(GtkButton *button, gpointer user_data) {
    const char *file_path = gtk_editable_get_text(GTK_EDITABLE(file_path_entry));

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        g_print("Error opening file: %s\n", file_path);
        gtk_label_set_text(GTK_LABEL(output_label), "Error opening file");
        return;
    }

    int count = count_oc_file(file, SEARCH_SUBJECT);
    fclose(file);

    char output_text[MAXLEN];
    snprintf(output_text, sizeof(output_text), "Occurrences of 'Pillars used': %d", count);
    gtk_label_set_text(GTK_LABEL(output_label), output_text);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *button;
    GtkEntryBuffer *buffer;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Path Example");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(window), box);

    button = gtk_button_new_with_label("Calculate");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_pressed), NULL);
    gtk_box_append(GTK_BOX(box), button);

    buffer = gtk_entry_buffer_new(DEFAULT_FILE_PATH, -1);
    file_path_entry = gtk_entry_new_with_buffer(buffer);
    gtk_box_append(GTK_BOX(box), file_path_entry);

    output_label = gtk_label_new("Occurrences: 0");
    gtk_box_append(GTK_BOX(box), output_label);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
