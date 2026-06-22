#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <gtk/gtk.h>

#define WEBVIEW_GTK 1
#define WEBVIEW_IMPLEMENTATION
#include "webview.h"
#include "server.h"

void check_and_ensure_config(void) {
    ConfigData data = read_config_file("cfg.yml");
    if (!data.exists) {
        printf("cfg.yml missing. Automatically creating from template...\n");
        create_config_from_template("cfg.yml.template", "cfg.yml");
    }
}

void handle_get_config(struct webview *w, const char *arg) {
    (void)arg;
    ConfigData data = read_config_file("cfg.yml");
    
    char json_reply[4192];
    if (data.exists) {
        char sanitized[4096] = {0};
        int j = 0;
        for(int i = 0; data.content[i] != '\0'; i++) {
            if(data.content[i] == '\n') { sanitized[j++] = '\\'; sanitized[j++] = 'n'; }
            else if(data.content[i] == '"') { sanitized[j++] = '\\'; sanitized[j++] = '"'; }
            else { sanitized[j++] = data.content[i]; }
        }
        snprintf(json_reply, sizeof(json_reply), "window.receiveConfig({\"exists\": true, \"content\": \"%s\"})", sanitized);
    } else {
        snprintf(json_reply, sizeof(json_reply), "window.receiveConfig({\"exists\": false})");
    }
    
    webview_eval(w, json_reply);
}

void handle_add_mixtape(struct webview *w) {
    GtkWidget *parent_window = NULL;
    if (w != NULL && w->priv.window != NULL) {
        parent_window = GTK_WIDGET(w->priv.window);
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Mix Tape Folder",
                                                    parent_window ? GTK_WINDOW(parent_window) : NULL,
                                                    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename != NULL) {
            printf("Selected mix tape folder: %s\n", filename);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

void my_external_invoke_cb(struct webview *w, const char *arg) {
    if (arg != NULL) {
        if (strcmp(arg, "getConfig") == 0) {
            handle_get_config(w, arg);
        } else if (strcmp(arg, "addMixTape") == 0) {
            handle_add_mixtape(w);
        } else if (strcmp(arg, "exitApp") == 0) {
            printf("Exit confirmation approved. Terminating webview lifecycle loop.\n");
            webview_terminate(w);
        }
    }
}

int main(void) {
    check_and_ensure_config();

    struct webview webview = {
        .title = "Configuration Manager",
        .width = 1280,
        .height = 960,
        .resizable = 0,
        .external_invoke_cb = my_external_invoke_cb
    };

    char path[PATH_MAX];
    if (realpath("./public/index.html", path) != NULL) {
        char url[PATH_MAX + 8];
        snprintf(url, sizeof(url), "file://%s", path);
        webview.url = url;
    } else {
        fprintf(stderr, "Could not find public/index.html\n");
        return 1;
    }

    if (webview_init(&webview) != 0) {
        return 1;
    }

    while (webview_loop(&webview, 1) == 0);
    
    webview_exit(&webview);
    return 0;
}
