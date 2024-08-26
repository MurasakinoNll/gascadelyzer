#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAXLEN 65536
#define REFRESH_INTERVAL 100  // refresh interval in milliseconds

#ifdef _WIN32
#include <windows.h>
#define DEFAULT_PATH "\\Warframe\\EE.log"
#else
#ifdef DEV_MODE
#define DEFAULT_PATH "/home/haraku/harakdev/gascade/flyra.filtered"
#else
#define DEFAULT_PATH ".local/share/Steam/steamapps/compatdata/230410/pfx/drive_c/users/steamuser/AppData/Local/Warframe/EE.log"
#endif
#endif

typedef struct {
    GtkWidget *label;
    const char *search_string;
    const char *label_format;
    int count;
    FILE *file;
} LabelData;

typedef struct {
    GtkWidget *mission_time_label;
    GtkWidget *levelcap_time_label;
    GtkWidget *reset_label;
    time_t mission_start_time;
    time_t initial_time;
    int reset_count;
    int pillars_count;
    time_t levelcap_time;
    gboolean mission_running;
    FILE *file;
} MissionData;

int count_oc_file(FILE *file, const char *search_string) {
    char line[MAXLEN];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        char *pos = line;
        while ((pos = strstr(pos, search_string)) != NULL) {
            count++;
            pos += strlen(search_string);
        }
    }
    return count;
}

static void find_initial_time(FILE *file, MissionData *data) {
    char line[MAXLEN];
    double earliest_time = -1.0;

    rewind(file);
    while (fgets(line, sizeof(line), file)) {
        char *timestamp_str = strtok(line, " ");
        if (timestamp_str != NULL) {
            double timestamp = strtod(timestamp_str, NULL);
            if (earliest_time < 0.0 || timestamp < earliest_time) {
                earliest_time = timestamp;
            }
        }
    }

    data->initial_time = earliest_time > 0.0 ? (time_t)earliest_time : 0;
}

static gboolean refresh_count(gpointer user_data) {
    LabelData *data = (LabelData *)user_data;
    GtkWidget *label = data->label;
    const char *search_string = data->search_string;
    const char *label_format = data->label_format;

    rewind(data->file);
    data->count = count_oc_file(data->file, search_string);

    char label_text[MAXLEN];
    snprintf(label_text, MAXLEN, label_format, data->count);
    gtk_label_set_text(GTK_LABEL(label), label_text);

    return TRUE;  // repeat calls
}

static gboolean refresh_mission(gpointer user_data) {
    MissionData *data = (MissionData *)user_data;

    char line[MAXLEN];
    while (fgets(line, sizeof(line), data->file)) {
        if (strstr(line, "Host_StartMatch")) {
            data->mission_start_time = time(NULL) - data->initial_time;
            data->mission_running = TRUE;
            data->pillars_count = 0;
            g_print("Mission started\n");
        } else if (strstr(line, "ThemedSquadOverlay.lua: OnSquadCountdown: 1")) {
            data->mission_running = FALSE;
            data->reset_count++;
            char reset_text[MAXLEN];
            snprintf(reset_text, MAXLEN, "Mission Resets: %d", data->reset_count);
            gtk_label_set_text(GTK_LABEL(data->reset_label), reset_text);
            g_print("Mission reset\n");
        } else if (strstr(line, "Pillars used increased to")) {
            data->pillars_count++;
            g_print("Pillars count increased: %d\n", data->pillars_count);
            if (data->pillars_count == 107) {
                data->levelcap_time = time(NULL) - data->initial_time;
                char levelcap_text[MAXLEN];
                snprintf(levelcap_text, MAXLEN, "Levelcap Time: %ld seconds", data->levelcap_time);
                gtk_label_set_text(GTK_LABEL(data->levelcap_time_label), levelcap_text);
                g_print("Levelcap reached\n");
            }
        }
    }

    if (data->mission_running) {
        char mission_text[MAXLEN];
        snprintf(mission_text, MAXLEN, "Mission Time: %ld seconds", time(NULL) - data->initial_time - data->mission_start_time);
        gtk_label_set_text(GTK_LABEL(data->mission_time_label), mission_text);
        g_print("Mission running: %ld seconds\n", time(NULL) - data->initial_time - data->mission_start_time);
    }

    rewind(data->file);
    return TRUE;  // repeat calls
}

static void add_label(GtkWidget *box, const char *search_string, const char *label_format, int refresh_interval, FILE *file) {
    GtkWidget *label = gtk_label_new("");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), label);

    LabelData *data = g_new0(LabelData, 1);
    data->label = label;
    data->search_string = search_string;
    data->label_format = label_format;
    data->file = file;

    g_timeout_add(refresh_interval, refresh_count, data);
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* box;

    char file_path[MAXLEN];
#ifdef _WIN32
    const char* localAppData = getenv("LOCALAPPDATA");
    if (localAppData == NULL) {
        localAppData = "";
    }
    snprintf(file_path, MAXLEN, "%s%s", localAppData, DEFAULT_PATH);
#else
    snprintf(file_path, MAXLEN, "%s/%s", getenv("HOME"), DEFAULT_PATH);
#endif

    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        g_print("Error opening file: %s\n", file_path);
        return;
    }

    MissionData *mission_data = g_new0(MissionData, 1);
    mission_data->mission_running = FALSE;
    mission_data->reset_count = 0;
    mission_data->pillars_count = 0;
    mission_data->file = file;

    find_initial_time(file, mission_data);

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gascadelyzer");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_window_set_child(GTK_WINDOW(window), box);

    GtkWidget *reset_label = gtk_label_new("Mission Resets: 0");
    gtk_widget_set_halign(reset_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), reset_label);
    mission_data->reset_label = reset_label;

    GtkWidget *mission_time_label = gtk_label_new("Mission Time: 0 seconds");
    gtk_widget_set_halign(mission_time_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), mission_time_label);
    mission_data->mission_time_label = mission_time_label;

    GtkWidget *levelcap_time_label = gtk_label_new("Levelcap Time: 0 seconds");
    gtk_widget_set_halign(levelcap_time_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), levelcap_time_label);
    mission_data->levelcap_time_label = levelcap_time_label;

    g_timeout_add(REFRESH_INTERVAL, refresh_mission, mission_data);

    add_label(box, "Pillars used", "Exolizers retired: %d", REFRESH_INTERVAL, file);
    add_label(box, "Cleansing SurvivalLifeSupportPillarCorruptable", "Exolizers cleansed: %d", REFRESH_INTERVAL, file);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char** argv) {
    GtkApplication* app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

