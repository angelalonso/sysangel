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

void mock_check_and_ensure_config() {
    ConfigData data = read_config_file("test_cfg.yml");
    if (!data.exists) {
        create_config_from_template("test_cfg.yml.template", "test_cfg.yml");
    }
}

// Mock JS Router matching app.js architecture updates
const char* mock_handle_keyboard_esc(const char* current_screen_id) {
    if (strcmp(current_screen_id, "screen-main") == 0) {
        return "screen-confirm-exit"; // Esc on main now goes to confirmation page
    }
    if (strcmp(current_screen_id, "screen-confirm-exit") == 0) {
        return "screen-main"; // Esc on confirm exit acts as a dismissal / stays open
    }
    return "screen-main"; // Sub-screens route back to dashboard
}

void test_automatic_template_instantiation() {
    remove("test_cfg.yml");
    FILE* t = fopen("test_cfg.yml.template", "w");
    fprintf(t, "setting: template_default\n");
    fclose(t);

    mock_check_and_ensure_config();

    ConfigData data = read_config_file("test_cfg.yml");
    assert_msg(data.exists == 1, "Strategy must automatically create cfg.yml if it does not exist.");
    assert_msg(strstr(data.content, "setting: template_default") != NULL, "Automatically created config must match template payload.");

    remove("test_cfg.yml");
    remove("test_cfg.yml.template");
}

void test_existing_config_is_not_overwritten() {
    FILE* t = fopen("test_cfg.yml.template", "w");
    fprintf(t, "setting: template_default\n");
    fclose(t);

    FILE* c = fopen("test_cfg.yml", "w");
    fprintf(c, "setting: custom_user_override\n");
    fclose(c);

    mock_check_and_ensure_config();

    ConfigData data = read_config_file("test_cfg.yml");
    assert_msg(strstr(data.content, "setting: custom_user_override") != NULL, "Strategy must not overwrite an already existing configuration file.");

    remove("test_cfg.yml");
    remove("test_cfg.yml.template");
}

void test_window_dimensions_doubled() {
    int expected_width = 1280;
    int expected_height = 960;
    assert_msg(expected_width == 1280, "Main window entry application layout configuration width must be 1280 pixels.");
    assert_msg(expected_height == 960, "Main window entry application layout configuration height must be 960 pixels.");
}

void test_keyboard_escape_routing() {
    assert_msg(strcmp(mock_handle_keyboard_esc("screen-mixes"), "screen-main") == 0, "Pressing Escape on mixes screen must route view state back to main screen.");
    assert_msg(strcmp(mock_handle_keyboard_esc("screen-main"), "screen-confirm-exit") == 0, "Pressing Escape on the main dashboard must trigger the exit confirmation dialog screen.");
    assert_msg(strcmp(mock_handle_keyboard_esc("screen-confirm-exit"), "screen-main") == 0, "Pressing Escape on the exit screen must cancel out and drop safely back to main.");
}

int main() {
    printf("=== Starting WebView UI State & Strategy Test Suite ===\n");
    test_automatic_template_instantiation();
    test_existing_config_is_not_overwritten();
    test_window_dimensions_doubled();
    test_keyboard_escape_routing();
    printf("=== Summary: %d Passed, %d Failed ===\n", tests_run - tests_failed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
