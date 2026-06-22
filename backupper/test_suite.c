#include <stdio.h>
#include <string.h>
#include "server.h"

int tests_run = 0;
int tests_failed = 0;

#define assert_msg(expr, msg) do { \
    tests_run++; \
    if (!(expr)) { \
        printf("[FAIL] %s\n", msg); \
        tests_failed++; \
    } else { \
        printf("[PASS] %s\n", msg); \
    } \
} while(0)

// Helper function mock to test how main.c sanitizes data for JS injection
void sanitize_for_js(const char* input, char* output) {
    int j = 0;
    for(int i = 0; input[i] != '\0'; i++) {
        if(input[i] == '\n') { 
            output[j++] = '\\'; 
            output[j++] = 'n'; 
        } else if(input[i] == '"') { 
            output[j++] = '\\'; 
            output[j++] = '"'; 
        } else { 
            output[j++] = input[i]; 
        }
    }
    output[j] = '\0';
}

void test_missing_config_behavior() {
    remove("cfg.yml"); 
    ConfigData data = read_config_file("cfg.yml");
    assert_msg(data.exists == 0, "Backend should detect when cfg.yml is missing.");
}

void test_reading_valid_config() {
    FILE* f = fopen("cfg.yml", "w");
    fprintf(f, "test: pass");
    fclose(f);

    ConfigData data = read_config_file("cfg.yml");
    assert_msg(data.exists == 1, "Backend should detect when cfg.yml is present.");
    assert_msg(strstr(data.content, "test: pass") != NULL, "Backend must read the accurate contents of cfg.yml.");
    
    remove("cfg.yml"); 
}

void test_js_serialization_escaping() {
    // Tests that multi-line YAML text won't break the JS window context 
    const char* raw_yaml = "key: \"value\"\nnext: true";
    char sanitized[256] = {0};
    
    sanitize_for_js(raw_yaml, sanitized);
    
    assert_msg(strstr(sanitized, "\\n") != NULL, "Newlines must be escaped to '\\n' for safe JS JSON parsing.");
    assert_msg(strstr(sanitized, "\\\"") != NULL, "Quotes must be escaped to '\\\"' so they don't terminate JS strings early.");
}

int main() {
    printf("=== Starting WebView Application Test Suite ===\n");
    test_missing_config_behavior();
    test_reading_valid_config();
    test_js_serialization_escaping();
    printf("=== Summary: %d Passed, %d Failed ===\n", tests_run - tests_failed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
