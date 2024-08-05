#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#define MAXLEN 1024

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

char file_path[MAXLEN] = "%appdata%/local/warframe/EE.log";

static void on_button_pressed(GtkButton *button, gpointer user_data) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        g_print("Error opening file: %s\n", file_path);
        return;
    }
    
    const char *search_subject_str = "Pillars used";
    int count = count_oc_file(file, search_subject_str);
    fclose(file);
    
    g_print("Occurrences: %d\n", count);
}

static void on_label_edited(GtkEditable *editable, gpointer user_data) {
    const char *custom_path = gtk_editable_get_text(editable);
    strncpy(file_path, custom_path, MAXLEN - 1);
    file_path[MAXLEN - 1] = '\0';  // Ensure null-termination
    g_print("File path set to: %s\n", file_path);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *button;
    GtkWidget *label_entry;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Path Example");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(window), box);

    button = gtk_button_new_with_label("Count Occurrences");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_pressed), NULL);
    gtk_box_append(GTK_BOX(box), button);

    label_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(label_entry), "Enter custom file path");
    g_signal_connect(label_entry, "changed", G_CALLBACK(on_label_edited), NULL);
    gtk_box_append(GTK_BOX(box), label_entry);

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

