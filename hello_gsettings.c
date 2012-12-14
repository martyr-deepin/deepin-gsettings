#include <gtk/gtk.h>
#include <gio/gio.h>

static GSettings *m_settings = NULL;
static GtkAdjustment *m_adjust = NULL;

static void m_settings_changed(GSettings *settings, 
                               gchar *key, 
                               gpointer user_data) 
{
    double value = g_settings_get_double(settings, "brightness");

    printf("%s\n", (gchar*) user_data);
    printf("DEBUG %s key changed\n", key);
    printf("DEBUG changed value %f\n", value);
}

static void m_value_changed(GtkAdjustment *adjust, gpointer user_data) 
{
    double value = gtk_adjustment_get_value(adjust);

    printf("%s\n", (gchar*) user_data);
    printf("DEBUG value %f\n", value);
    g_settings_set_double(m_settings, "brightness", value / 100.0);
    g_settings_sync();
}

int main(int argc, char **argv)
{
    GtkWidget *window = NULL;
    GtkWidget *scale = NULL;

    g_type_init();
    gtk_init(&argc, &argv);

    m_settings = g_settings_new("org.gnome.settings-daemon.plugins.xrandr");
    g_signal_connect(m_settings, 
                     "changed", 
                     m_settings_changed, 
                     "DEBUG GSettings changed signal");

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GSettings Demo");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    m_adjust = gtk_adjustment_new(80, 5, 100, 1, 1, 1);
    g_signal_connect(m_adjust, 
                     "value-changed", 
                     G_CALLBACK(m_value_changed), 
                     "DEBUG GtkAdjustment value-changed signal");
    scale = gtk_hscale_new(m_adjust);
    gtk_container_add(GTK_CONTAINER(window), scale);

    gtk_widget_show(scale);
    gtk_widget_show(window);
  
    gtk_main();

    if (m_settings) {
        g_object_unref(m_settings);
        m_settings = NULL;
    }

    return 0;
}
