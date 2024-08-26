#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <sys/time.h>  // For getting the current time

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
    GtkWidget *duration_label;
    GQueue *cleansing_start_times;
    long last_position;
} SuccessRateData;

void parse_timestamp(const char *line, double *timestamp) {
    sscanf(line, "%lf", timestamp);
}

// Convert gdouble to gpointer
static inline gpointer gdouble_to_pointer(gdouble val) {
    union {
        gdouble val;
        gpointer ptr;
    } u;
    u.val = val;
    return u.ptr;
}

// Convert gpointer to gdouble
static inline gdouble pointer_to_gdouble(gpointer ptr) {
    union {
        gdouble val;
        gpointer ptr;
    } u;
    u.ptr = ptr;
    return u.val;
}

static gboolean refresh_durations(gpointer user_data) {
    SuccessRateData *data = (SuccessRateData *)user_data;

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
        return TRUE;  // repeat calls
    }

    // Move the file pointer to the last read position
    fseek(file, data->last_position, SEEK_SET);

    char line[MAXLEN];
    double timestamp;
    double total_duration = 0.0;
    int cleanse_count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "Cleansing SurvivalLifeSupportPillarCorruptible")) {
            parse_timestamp(line, &timestamp);
            g_queue_push_tail(data->cleansing_start_times, gdouble_to_pointer(timestamp));
            g_print("Logged cleansing start at %.3f seconds\n", timestamp);
        } else if (strstr(line, "Pillars used increased to")) {
            parse_timestamp(line, &timestamp);
            if (!g_queue_is_empty(data->cleansing_start_times)) {
                double start_time = pointer_to_gdouble(g_queue_pop_head(data->cleansing_start_times));
                double duration = timestamp - start_time;
                total_duration += duration;
                cleanse_count++;
                g_print("Logged cleansing duration: %.3f seconds\n", duration);
            } else {
                g_print("Warning: No matching start time found for a completed cleanse.\n");
            }
        }
    }

    // Save the current file position
    data->last_position = ftell(file);

    fclose(file);

    // Calculate the average duration
    if (cleanse_count > 0) {
        double average_duration = total_duration / cleanse_count;
        char label_text[MAXLEN];
        snprintf(label_text, MAXLEN, "Exolizer Average Cleanse Duration: %.2f seconds", average_duration);
        gtk_label_set_text(GTK_LABEL(data->duration_label), label_text);
    }

    return TRUE;  // repeat calls
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* box;
    SuccessRateData *success_rate_data = g_new(SuccessRateData, 1);
    success_rate_data->cleansing_start_times = g_queue_new();
    success_rate_data->last_position = 0;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gascadelyzer");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_window_set_child(GTK_WINDOW(window), box);

    GtkWidget *duration_label = gtk_label_new("Exolizer Average Cleanse Duration: 0.00 seconds");
    gtk_widget_set_halign(duration_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), duration_label);
    success_rate_data->duration_label = duration_label;

    g_timeout_add(REFRESH_INTERVAL, refresh_durations, success_rate_data);

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

