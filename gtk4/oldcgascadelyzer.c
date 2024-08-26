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
    GtkWidget *label;
    const char *search_string;
    const char *label_format;
    int count;
} LabelData;

typedef struct {
    LabelData *retired_data;
    LabelData *cleansed_data;
    GtkWidget *success_label;
    GtkWidget *duration_label;
    GList *cleansing_start_times;
} SuccessRateData;

void parse_timestamp(const char *line, double *timestamp) {
    sscanf(line, "%lf", timestamp);
}

// Helper functions to convert gdouble to gpointer and vice versa
static inline gpointer gdouble_to_pointer(gdouble val) {
    union {
        gdouble val;
        gpointer ptr;
    } u;
    u.val = val;
    return u.ptr;
}

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

    struct timeval start, end;
    gettimeofday(&start, NULL);  // Start timing

    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        g_print("Error opening file: %s\n", file_path);
        return TRUE;  // repeat calls
    }

    char line[MAXLEN];
    double timestamp;
    GList *durations = NULL;
    double total_duration = 0.0;
    int cleanse_count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "Cleansing SurvivalLifeSupportPillarCorruptible")) {
            parse_timestamp(line, &timestamp);
            gdouble *start_time_ptr = g_new(gdouble, 1);
            *start_time_ptr = timestamp;
            data->cleansing_start_times = g_list_append(data->cleansing_start_times, start_time_ptr);
            g_print("Logged cleansing start at %.3f seconds\n", timestamp);
        } else if (strstr(line, "Pillars used increased to")) {
            parse_timestamp(line, &timestamp);
            if (data->cleansing_start_times != NULL && g_list_length(data->cleansing_start_times) > 0) {
                gdouble *start_time_ptr = (gdouble *)data->cleansing_start_times->data;
                double duration = timestamp - *start_time_ptr;
                durations = g_list_append(durations, gdouble_to_pointer(duration));
                g_free(start_time_ptr);
                data->cleansing_start_times = g_list_delete_link(data->cleansing_start_times, data->cleansing_start_times);
                cleanse_count++;
                g_print("Logged cleansing duration: %.3f seconds\n", duration);
            } else {
                g_print("Warning: No matching start time found for a completed cleanse.\n");
            }
        }

        // Calculate average duration for every 3 cleanses
        if (cleanse_count >= 3) {
            double group_total = 0.0;
            for (int i = 0; i < 3; i++) {
                gpointer data_ptr = g_list_nth_data(durations, cleanse_count - 3 + i);
                if (data_ptr != NULL) {
                    group_total += pointer_to_gdouble(data_ptr);
                } else {
                    g_print("Error: Null pointer encountered in duration list\n");
                    return FALSE;  // stop on error
                }
            }
            double group_average = group_total / 3.0;

            char label_text[MAXLEN];
            snprintf(label_text, MAXLEN, "Exolizer Average Cleanse Duration: %.2f seconds", group_average);
            gtk_label_set_text(GTK_LABEL(data->duration_label), label_text);

            // Free the durations list
            //g_list_free_full(durations, (GDestroyNotify)g_free);
            g_list_free(durations);
            durations = NULL;
            cleanse_count = 0;
        }
    }

    fclose(file);

    gettimeofday(&end, NULL);  // End timing
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    g_print("Log file read completed in %.3f seconds\n", elapsed_time);

    return TRUE;  // repeat calls
}

static void add_label(GtkWidget *box, const char *search_string, const char *label_format, guint interval, LabelData **out_data) {
    GtkWidget *label = gtk_label_new("");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), label);

    LabelData *data = g_new(LabelData, 1);
    data->label = label;
    data->search_string = search_string;
    data->label_format = label_format;
    data->count = 0;

    *out_data = data;
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* box;
    SuccessRateData *success_rate_data = g_new(SuccessRateData, 1);
    success_rate_data->cleansing_start_times = NULL;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gascadelyzer");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_window_set_child(GTK_WINDOW(window), box);

    add_label(box, "Pillars used", "Exolizers retired: %d", REFRESH_INTERVAL, &success_rate_data->retired_data);
    add_label(box, "Cleansing SurvivalLifeSupportPillarCorruptible", "Exolizers cleansed: %d", REFRESH_INTERVAL, &success_rate_data->cleansed_data);

    GtkWidget *success_label = gtk_label_new("Exolizer Defense Success Rate: 0.00%");
    gtk_widget_set_halign(success_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), success_label);
    success_rate_data->success_label = success_label;

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

