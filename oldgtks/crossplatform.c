#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXLEN 65536
#define REFRESH_INTERVAL 1000  // refresh interval in milliseconds

#ifdef _WIN32
#include <windows.h>
#define DEFAULT_PATH "%APPDATA%\\local\\Windows\\EE.log"
#else
#ifdef DEV_MODE
#define DEFAULT_PATH "/home/haraku/harakdev/gascade/flyra.filtered"
#else
#define DEFAULT_PATH ".local/share/Steam/steamapps/compatdata/230410/pfx/drive_c/users/steamuser/AppData/Local/Warframe/EE.log"
#endif
#endif

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

static gboolean refresh_count(gpointer user_data) {
    GtkWidget *label = GTK_WIDGET(user_data);

#ifdef _WIN32
    // expanded Windows path
    char expanded_path[MAXLEN];
    ExpandEnvironmentStrings(DEFAULT_PATH, expanded_path, MAXLEN);
    
    const char *file_path = expanded_path;
#else
    // expanded Linux path
    char file_path[MAXLEN];
    snprintf(file_path, MAXLEN, "%s/%s", getenv("HOME"), DEFAULT_PATH);
#endif

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        g_print("Error opening file: %s\n", file_path);
        return TRUE;  // repeat calls
    }

    const char *search_subject_str = "Pillars used";
    int count = count_oc_file(file, search_subject_str);
    fclose(file);

    char label_text[MAXLEN];
    snprintf(label_text, MAXLEN, "Exolizers retired: %d", count);
    gtk_label_set_text(GTK_LABEL(label), label_text);

    return TRUE;  // repeat calls
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *label;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gascadelyzer");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_window_set_child(GTK_WINDOW(window), box);

    label = gtk_label_new("Exolizers retired: 0");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), label);

    g_timeout_add(REFRESH_INTERVAL, refresh_count, label);

    gtk_window_present(GTK_WINDOW(window));
    g_print("Using path: %s/%s\n", getenv("HOME"), DEFAULT_PATH);
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


