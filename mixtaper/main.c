#define WEBVIEW_GTK
#define WEBVIEW_IMPLEMENTATION
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include "webview.h"
#include "server.h"

// Structure for background rsync task
typedef struct {
    char cmd[8192];
    char cmd_id[128];
    char result[4096];
    int exit_code;
    int completed;
    struct webview *webview;
} rsync_task_t;

// Thread-safe queue for rsync tasks
#define MAX_RSYNC_TASKS 16
rsync_task_t rsync_tasks[MAX_RSYNC_TASKS];
int rsync_task_count = 0;
pthread_mutex_t rsync_mutex = PTHREAD_MUTEX_INITIALIZER;
int rsync_thread_running = 1;

// Forward declaration
static gboolean idle_rsync_callback(gpointer user_data);

// Background thread for rsync execution
void* rsync_worker(void* arg) {
    (void)arg;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100ms
    
    while (rsync_thread_running) {
        rsync_task_t task;
        int has_task = 0;
        
        pthread_mutex_lock(&rsync_mutex);
        for (int i = 0; i < MAX_RSYNC_TASKS; i++) {
            if (rsync_tasks[i].completed == 0 && rsync_tasks[i].cmd[0] != '\0') {
                task = rsync_tasks[i];
                rsync_tasks[i].completed = 1; // Mark as being processed
                has_task = 1;
                break;
            }
        }
        pthread_mutex_unlock(&rsync_mutex);
        
        if (!has_task) {
            nanosleep(&ts, NULL);
            continue;
        }
        
        // Execute the command
        printf("[Rsync] Executing: %s\n", task.cmd);
        fflush(stdout);
        
        FILE *fp = popen(task.cmd, "r");
        if (fp == NULL) {
            strcpy(task.result, "Failed to execute command");
            task.exit_code = -1;
        } else {
            char line[512];
            size_t result_len = 0;
            while (fgets(line, sizeof(line), fp) != NULL) {
                size_t line_len = strlen(line);
                if (result_len + line_len < sizeof(task.result) - 1) {
                    strcpy(task.result + result_len, line);
                    result_len += line_len;
                }
                // Also print to stdout for real-time monitoring
                printf("%s", line);
                fflush(stdout);
            }
            task.exit_code = pclose(fp);
            if (WIFEXITED(task.exit_code)) {
                task.exit_code = WEXITSTATUS(task.exit_code);
            } else {
                task.exit_code = -1;
            }
        }
        
        printf("[Rsync] Task %s completed with exit code %d\n", task.cmd_id, task.exit_code);
        fflush(stdout);
        
        // Notify the UI about completion - schedule on main thread
        if (task.webview != NULL) {
            // Create a copy of the data to pass to the idle callback
            rsync_task_t *task_copy = (rsync_task_t*)malloc(sizeof(rsync_task_t));
            if (task_copy != NULL) {
                memcpy(task_copy, &task, sizeof(rsync_task_t));
                // Schedule the UI update on the main thread using g_idle_add
                g_idle_add(idle_rsync_callback, task_copy);
            }
        }
        
        // Clear the task
        pthread_mutex_lock(&rsync_mutex);
        memset(&task, 0, sizeof(task));
        pthread_mutex_unlock(&rsync_mutex);
    }
    return NULL;
}

// Idle callback to safely call webview_eval from the main thread
static gboolean idle_rsync_callback(gpointer user_data) {
    rsync_task_t *task = (rsync_task_t*)user_data;
    if (task == NULL || task->webview == NULL) {
        if (task) free(task);
        return FALSE;
    }
    
    char escaped_result[4096] = {0};
    js_escape(task->result, escaped_result, sizeof(escaped_result));
    
    char js_buf[16384];
    if (task->exit_code == 0) {
        snprintf(js_buf, sizeof(js_buf), 
            "window.rsyncCallbacks && window.rsyncCallbacks['%s'] && window.rsyncCallbacks['%s']('success', '%s');",
            task->cmd_id, task->cmd_id, escaped_result);
    } else {
        snprintf(js_buf, sizeof(js_buf), 
            "window.rsyncCallbacks && window.rsyncCallbacks['%s'] && window.rsyncCallbacks['%s']('error: exit code %d', '%s');",
            task->cmd_id, task->cmd_id, task->exit_code, escaped_result);
    }
    webview_eval(task->webview, js_buf);
    
    // Also log to console
    char notify_buf[1024];
    snprintf(notify_buf, sizeof(notify_buf),
        "console.log('[Rsync] Task %s completed with exit code %d');",
        task->cmd_id, task->exit_code);
    webview_eval(task->webview, notify_buf);
    
    free(task);
    return FALSE;
}

// Callback for handling frontend invocations
void my_external_invoke_cb(struct webview *w, const char *arg) {
    if (strcmp(arg, "exitApp") == 0) {
        webview_terminate(w);
        // Signal the rsync worker to stop
        rsync_thread_running = 0;
    } 
    else if (strcmp(arg, "getConfig") == 0 || strcmp(arg, "loadInitialData") == 0) {
        FILE *cfg_file = fopen("cfg.yml", "r");
        if (!cfg_file) {
            cfg_file = fopen("cfg.yml", "w");
            if (cfg_file) {
                fprintf(cfg_file, "data_type: file\ndata_file: data.json\n");
                fclose(cfg_file);
            }
            cfg_file = fopen("cfg.yml", "r");
        }
        
        char cfg_content[4096] = {0};
        if (cfg_file) {
            size_t bytes = fread(cfg_content, 1, sizeof(cfg_content) - 1, cfg_file);
            cfg_content[bytes] = '\0';
            fclose(cfg_file);
        }
        
        char data_type[128] = {0};
        char data_file_path[256] = {0};
        parse_config(cfg_content, data_type, data_file_path);
        
        if (strcmp(data_type, "file") != 0) {
            webview_dialog(w, WEBVIEW_DIALOG_TYPE_ALERT, WEBVIEW_DIALOG_FLAG_ERROR, 
                           "Configuration Error", "Invalid data_type specified in cfg.yml. Only 'file' is supported.", NULL, 0);
            webview_eval(w, "window.initializeData('', '', 1);");
            return;
        }
        
        FILE *d_file = fopen(data_file_path, "r");
        if (!d_file) {
            d_file = fopen(data_file_path, "w");
            if (d_file) {
                fprintf(d_file, "{\"mixes\":[],\"tapes\":[],\"mixTapes\":[]}");
                fclose(d_file);
            }
            d_file = fopen(data_file_path, "r");
        }
        
        char data_content[8192] = {0};
        if (d_file) {
            size_t bytes = fread(data_content, 1, sizeof(data_content) - 1, d_file);
            data_content[bytes] = '\0';
            fclose(d_file);
        }
        
        char *escaped_cfg = malloc(8192);
        char *escaped_data = malloc(16384);
        js_escape(cfg_content, escaped_cfg, 8192);
        js_escape(data_content, escaped_data, 16384);
        
        char *js_eval = malloc(32768);
        snprintf(js_eval, 32768, "window.initializeData(\"%s\", \"%s\", 0);", escaped_cfg, escaped_data);
        webview_eval(w, js_eval);
        
        free(escaped_cfg);
        free(escaped_data);
        free(js_eval);
    }
    else if (strncmp(arg, "saveData:", 9) == 0) {
        const char *json_str = arg + 9;
        FILE *cfg_file = fopen("cfg.yml", "r");
        char cfg_content[4096] = {0};
        if (cfg_file) {
            size_t bytes = fread(cfg_content, 1, sizeof(cfg_content) - 1, cfg_file);
            cfg_content[bytes] = '\0';
            fclose(cfg_file);
        } else {
            strcpy(cfg_content, "data_type: file\ndata_file: data.json\n");
        }
        
        char data_type[128] = {0};
        char data_file_path[256] = {0};
        parse_config(cfg_content, data_type, data_file_path);
        
        if (strcmp(data_type, "file") == 0) {
            FILE *d_file = fopen(data_file_path, "w");
            if (d_file) {
                fprintf(d_file, "%s", json_str);
                fclose(d_file);
            }
        }
    }
    else if (strcmp(arg, "selectTapeFolder") == 0) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Select Tape Directory Storage Folder",
            GTK_WINDOW(w->priv.window),
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT,
            NULL
        );

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (path != NULL) {
                char js_buf[4120];
                char escaped[2048] = {0};
                js_escape(path, escaped, sizeof(escaped));
                snprintf(js_buf, sizeof(js_buf), "window.receiveSelectedFolder(\"%s\");", escaped);
                webview_eval(w, js_buf);
                g_free(path);
            }
        }
        gtk_widget_destroy(dialog);
    }
    else if (strcmp(arg, "selectMixPaths") == 0) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Select Mix Files",
            GTK_WINDOW(w->priv.window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Add", GTK_RESPONSE_ACCEPT,
            NULL
        );
        
        // Allows selecting MULTIPLE files holding CTRL/SHIFT
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            GSList *paths = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
            
            size_t buf_size = 32768;
            char *js_buf = malloc(buf_size);
            strcpy(js_buf, "window.receiveMixPaths([");
            
            GSList *iter = paths;
            while (iter != NULL) {
                char *path = (char *)iter->data;
                char escaped[2048] = {0};
                js_escape(path, escaped, sizeof(escaped));
                
                strcat(js_buf, "\"");
                strcat(js_buf, escaped);
                strcat(js_buf, "\"");
                
                if (iter->next != NULL) {
                    strcat(js_buf, ", ");
                }
                
                g_free(path);
                iter = iter->next;
            }
            g_slist_free(paths);
            strcat(js_buf, "]);");
            
            webview_eval(w, js_buf);
            free(js_buf);
        }
        gtk_widget_destroy(dialog);
    }
    else if (strcmp(arg, "selectMixFolder") == 0) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Select Mix Folder",
            GTK_WINDOW(w->priv.window),
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Add", GTK_RESPONSE_ACCEPT,
            NULL
        );

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (path != NULL) {
                char js_buf[4120];
                char escaped[2048] = {0};
                js_escape(path, escaped, sizeof(escaped));
                snprintf(js_buf, sizeof(js_buf), "window.receiveMixFolder(\"%s\");", escaped);
                webview_eval(w, js_buf);
                g_free(path);
            }
        }
        gtk_widget_destroy(dialog);
    }
    else if (strncmp(arg, "rsync:", 6) == 0) {
        // Parse: "rsync:<command>|<callback_id>"
        const char *cmd_part = arg + 6;
        char cmd[8192] = {0};
        char cmd_id[128] = {0};
        
        // Split at the pipe separator
        const char *pipe_pos = strchr(cmd_part, '|');
        if (pipe_pos == NULL) {
            // No callback ID provided, use a default one
            strncpy(cmd, cmd_part, sizeof(cmd) - 1);
            cmd[sizeof(cmd) - 1] = '\0';
            snprintf(cmd_id, sizeof(cmd_id), "rsync-%d", (int)time(NULL));
        } else {
            size_t cmd_len = pipe_pos - cmd_part;
            if (cmd_len >= sizeof(cmd)) cmd_len = sizeof(cmd) - 1;
            strncpy(cmd, cmd_part, cmd_len);
            cmd[cmd_len] = '\0';
            strncpy(cmd_id, pipe_pos + 1, sizeof(cmd_id) - 1);
            cmd_id[sizeof(cmd_id) - 1] = '\0';
        }
        
        // Clean up the command - remove extra quotes and use proper shell escaping
        // The command already has single quotes from the JS side, which is correct
        char full_cmd[16384];
        snprintf(full_cmd, sizeof(full_cmd), "%s 2>&1", cmd);
        
        // Queue the task for background execution
        pthread_mutex_lock(&rsync_mutex);
        int task_index = -1;
        for (int i = 0; i < MAX_RSYNC_TASKS; i++) {
            if (rsync_tasks[i].cmd[0] == '\0') {
                task_index = i;
                break;
            }
        }
        
        if (task_index == -1) {
            // No slots available, report error back to JS
            char js_buf[1024];
            snprintf(js_buf, sizeof(js_buf), 
                "window.rsyncCallbacks && window.rsyncCallbacks['%s'] && window.rsyncCallbacks['%s']('error: too many rsync tasks', '');",
                cmd_id, cmd_id);
            webview_eval(w, js_buf);
            pthread_mutex_unlock(&rsync_mutex);
            return;
        }
        
        // Initialize the task
        memset(&rsync_tasks[task_index], 0, sizeof(rsync_task_t));
        strncpy(rsync_tasks[task_index].cmd, full_cmd, sizeof(rsync_tasks[task_index].cmd) - 1);
        rsync_tasks[task_index].cmd[sizeof(rsync_tasks[task_index].cmd) - 1] = '\0';
        strncpy(rsync_tasks[task_index].cmd_id, cmd_id, sizeof(rsync_tasks[task_index].cmd_id) - 1);
        rsync_tasks[task_index].cmd_id[sizeof(rsync_tasks[task_index].cmd_id) - 1] = '\0';
        rsync_tasks[task_index].webview = w;
        rsync_tasks[task_index].completed = 0;
        pthread_mutex_unlock(&rsync_mutex);
        
        // Log to stdout
        printf("[Rsync] Started background task %s: %s\n", cmd_id, cmd);
        fflush(stdout);
    }
}

int main() {
    struct webview webview = {
        .title = "Audio Storage Manager",
        .width = 1280,
        .height = 960,
        .resizable = 1,
        .debug = 1,
        .external_invoke_cb = my_external_invoke_cb
    };

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "Error: Unable to get current working directory.\n");
        return 1;
    }

    char url[4150];
    snprintf(url, sizeof(url), "file://%s/index.html", cwd);
    webview.url = url;

    int res = webview_init(&webview);
    if (res != 0) {
        fprintf(stderr, "Error: WebView initialization failed with code %d. Ensure you have a running X11/Wayland display server.\n", res);
        return 1;
    }

    // Start the background rsync worker thread
    pthread_t rsync_thread;
    if (pthread_create(&rsync_thread, NULL, rsync_worker, NULL) != 0) {
        fprintf(stderr, "Warning: Failed to create rsync background thread. Rsync operations will block.\n");
    } else {
        pthread_detach(rsync_thread);
        fprintf(stderr, "Rsync background worker thread started.\n");
    }

    while (webview_loop(&webview, 1) == 0) {
        // Yields CPU loop block
    }

    // Signal the rsync worker to stop
    rsync_thread_running = 0;
    
    // Wait a bit for the thread to finish
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;
    nanosleep(&ts, NULL);
    
    webview_exit(&webview);
    return 0;
}
