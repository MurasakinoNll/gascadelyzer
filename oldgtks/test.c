#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLEN 65536

static void
print_hello (GtkWidget *widget, gpointer data){
  g_print ("Hello World\n");
}

int count_oc_file(FILE *file, const char *search_subject_str){
  char line[MAXLEN];
  int exo_count = 0;

  while (fgets(line, sizeof(line), file)){
    char *pos = line;
    while ((pos=strstr(pos, search_subject_str)) != NULL){
      exo_count++;
      pos += strlen(search_subject_str);
    }
  }
  return exo_count;
}


static void
activate (GtkApplication* app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *box;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Testing");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box, GTK_ALIGN_CENTER);

  gtk_window_set_child (GTK_WINDOW (window), box);

  button = gtk_button_new_with_label ("Hello");
  
  g_signal_connect (button, "clicked", G_CALLBACK (count_oc_file("/home/haraku/harakdev/gascade/flyra.filtered", "Pillars used" )), NULL);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_window_destroy), window);

  gtk_box_append (GTK_BOX (box), button);
  gtk_window_present (GTK_WINDOW (window));

}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

