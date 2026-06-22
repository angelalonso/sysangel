#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int exists;
    char content[4096];
} ConfigData;

static inline ConfigData read_config_file(const char* filename) {
    ConfigData data = {0, ""};
    FILE* file = fopen(filename, "r");
    if (!file) {
        return data;
    }
    data.exists = 1;
    size_t bytes = fread(data.content, 1, sizeof(data.content) - 1, file);
    data.content[bytes] = '\0';
    fclose(file);
    return data;
}

static inline int create_config_from_template(const char* template_fn, const char* dest_fn) {
    FILE* src = fopen(template_fn, "r");
    if (!src) return 0;
    
    FILE* dest = fopen(dest_fn, "w");
    if (!dest) {
        fclose(src);
        return 0;
    }
    
    char ch;
    while ((ch = fgetc(src)) != EOF) {
        fputc(ch, dest);
    }
    
    fclose(src);
    fclose(dest);
    return 1;
}

#endif /* SERVER_H */
