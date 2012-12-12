#include <stdio.h>
#include <gio/gio.h>

int main(int argc, char **argv) 
{
    GSettings *settings = NULL;
    int i = 0;
    
    g_type_init();

    settings = g_settings_new("org.gnome.settings-daemon.plugins.power");
    if (!settings) 
        return -1;
    
    while (i < 1000) {
        printf("DEBUG g_settings_set_int\n");
        g_settings_set_int(settings, "idle-brightness", 31);
        printf("DEBUG g_settings_sync %d times\n", i + 1);
        g_settings_sync();
        i++;
    }
    
    if (settings) {
        g_object_unref(settings);
        settings = NULL;
    }
    
    return 0;
}
