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

// Duplicate implementation helper mirroring main.c check-and-create strategy logic
void mock_check_and_ensure_config() {
    ConfigData data = read_config_file("cfg.yml");
    if (!data.exists) {
        create_config_from_template("cfg.yml.template", "cfg.yml");
    }
}

void test_automatic_template_instantiation() {
    // 1. Setup mock environment files
    remove("cfg.yml");
    FILE* t = fopen("cfg.yml.template", "w");
    fprintf(t, "setting: template_default\n");
    fclose(t);

    // 2. Execute target logic under test
    mock_check_and_ensure_config();

    // 3. Verify target baseline behavior 
    ConfigData data = read_config_file("cfg.yml");
    assert_msg(data.exists == 1, "Strategy must automatically create cfg.yml if it does not exist.");
    assert_msg(strstr(data.content, "setting: template_default") != NULL, "Automatically created config must match template payload.");

    // Clean up
    remove("cfg.yml");
    remove("cfg.yml.template");
}

void test_existing_config_is_not_overwritten() {
    FILE* t = fopen("cfg.yml.template", "w");
    fprintf(t, "setting: template_default\n");
    fclose(t);

    FILE* c = fopen("cfg.yml", "w");
    fprintf(c, "setting: custom_user_override\n");
    fclose(c);

    // Run strategy logic
    mock_check_and_ensure_config();

    ConfigData data = read_config_file("cfg.yml");
    assert_msg(strstr(data.content, "setting: custom_user_override") != NULL, "Strategy must not overwrite an already existing configuration file.");

    remove("cfg.yml");
    remove("cfg.yml.template");
}

int main() {
    printf("=== Starting WebView UI State & Strategy Test Suite ===\n");
    test_automatic_template_instantiation();
    test_existing_config_is_not_overwritten();
    printf("=== Summary: %d Passed, %d Failed ===\n", tests_run - tests_failed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
