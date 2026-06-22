#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>  // Required internally by legacy webview.h
#include <limits.h>

#define WEBVIEW_GTK 1
#define WEBVIEW_IMPLEMENTATION
#include "webview.h"
#include "server.h"

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

void handle_create_config(struct webview *w, const char *arg) {
    (void)arg;
    int success = create_config_from_template("cfg.yml.template", "cfg.yml");
    char json_reply[128];
    snprintf(json_reply, sizeof(json_reply), "window.receiveCreateStatus({\"success\": %s})", success ? "true" : "false");
    webview_eval(w, json_reply);
}

void my_external_invoke_cb(struct webview *w, const char *arg) {
    if (arg != NULL) {
        if (strcmp(arg, "getConfig") == 0) {
            handle_get_config(w, arg);
        } else if (strcmp(arg, "createConfig") == 0) {
            handle_create_config(w, arg);
        }
    }
}

int main(void) {
    struct webview webview = {
        .title = "Configuration Manager",
        .width = 640,
        .height = 480,
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
