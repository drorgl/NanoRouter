#include "unity.h"
#include "nanorouter_header_rule_parser.h"
#include <string.h>
#include <stdlib.h>

// Helper function to compare two header_rule_t structs
static void assert_header_rule_equal(const header_rule_t *expected, const header_rule_t *actual) {
    TEST_ASSERT_EQUAL_STRING(expected->from_route, actual->from_route);
    TEST_ASSERT_EQUAL_UINT8(expected->num_headers, actual->num_headers);
    for (uint8_t i = 0; i < expected->num_headers; i++) {
        TEST_ASSERT_EQUAL_STRING(expected->headers[i].key, actual->headers[i].key);
        TEST_ASSERT_EQUAL_STRING(expected->headers[i].value, actual->headers[i].value);
    }
}

// --- Test Cases ---

void test_nanorouter_header_rule_list_create_and_free(void) {
    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_NULL(list->head);
    TEST_ASSERT_EQUAL_UINT(0, list->count);
    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_header_rule_list_add_single_rule(void) {
    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    header_rule_t rule1 = {
        .from_route = "/test",
        .headers = {{"X-Test", "Value1"}},
        .num_headers = 1
    };

    TEST_ASSERT_TRUE(nanorouter_header_rule_list_add_rule(list, &rule1));
    TEST_ASSERT_EQUAL_UINT(1, list->count);
    TEST_ASSERT_NOT_NULL(list->head);
    assert_header_rule_equal(&rule1, &list->head->rule);

    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_header_rule_list_add_multiple_rules(void) {
    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    header_rule_t rule1 = {
        .from_route = "/test1",
        .headers = {{"X-Test1", "Value1"}},
        .num_headers = 1
    };
    header_rule_t rule2 = {
        .from_route = "/test2",
        .headers = {{"X-Test2", "Value2"}, {"Cache-Control", "no-cache"}},
        .num_headers = 2
    };

    TEST_ASSERT_TRUE(nanorouter_header_rule_list_add_rule(list, &rule1));
    TEST_ASSERT_TRUE(nanorouter_header_rule_list_add_rule(list, &rule2));
    TEST_ASSERT_EQUAL_UINT(2, list->count);

    nanorouter_header_rule_node_t *current = list->head;
    TEST_ASSERT_NOT_NULL(current);
    assert_header_rule_equal(&rule1, &current->rule);

    current = current->next;
    TEST_ASSERT_NOT_NULL(current);
    assert_header_rule_equal(&rule2, &current->rule);
    TEST_ASSERT_NULL(current->next);

    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_parse_headers_file_basic(void) {
    const char *file_content =
        "/*\n"
        "  X-Frame-Options: DENY\n"
        "/path/to/file\n"
        "  Content-Type: text/html\n";

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_TRUE(nanorouter_parse_headers_file(file_content, list));
    TEST_ASSERT_EQUAL_UINT(2, list->count);

    // Rule 1
    header_rule_t expected_rule1 = {
        .from_route = "/*",
        .headers = {{"X-Frame-Options", "DENY"}},
        .num_headers = 1
    };
    assert_header_rule_equal(&expected_rule1, &list->head->rule);

    // Rule 2
    header_rule_t expected_rule2 = {
        .from_route = "/path/to/file",
        .headers = {{"Content-Type", "text/html"}},
        .num_headers = 1
    };
    assert_header_rule_equal(&expected_rule2, &list->head->next->rule);

    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_parse_headers_file_comments_and_empty_lines(void) {
    const char *file_content =
        "# This is a comment\n"
        "\n"
        "/*\n"
        "  X-Test: Value\n"
        "# Another comment\n"
        "  Cache-Control: no-cache\n"
        "\n";

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_TRUE(nanorouter_parse_headers_file(file_content, list));
    TEST_ASSERT_EQUAL_UINT(1, list->count);

    header_rule_t expected_rule = {
        .from_route = "/*",
        .headers = {{"X-Test", "Value"}, {"Cache-Control", "no-cache"}},
        .num_headers = 2
    };
    assert_header_rule_equal(&expected_rule, &list->head->rule);

    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_parse_headers_file_multiple_headers_same_route(void) {
    const char *file_content =
        "/api/*\n"
        "  Access-Control-Allow-Origin: *\n"
        "  Access-Control-Allow-Methods: GET, POST\n";

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_TRUE(nanorouter_parse_headers_file(file_content, list));
    TEST_ASSERT_EQUAL_UINT(1, list->count);

    header_rule_t expected_rule = {
        .from_route = "/api/*",
        .headers = {
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Methods", "GET, POST"}
        },
        .num_headers = 2
    };
    assert_header_rule_equal(&expected_rule, &list->head->rule);

    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_parse_headers_file_no_final_newline(void) {
    const char *file_content =
        "/test\n"
        "  X-Header: FinalValue"; // No newline at the end

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_TRUE(nanorouter_parse_headers_file(file_content, list));
    TEST_ASSERT_EQUAL_UINT(1, list->count);

    header_rule_t expected_rule = {
        .from_route = "/test",
        .headers = {{"X-Header", "FinalValue"}},
        .num_headers = 1
    };
    assert_header_rule_equal(&expected_rule, &list->head->rule);

    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_parse_headers_file_malformed_header(void) {
    const char *file_content =
        "/test\n"
        "  MalformedHeader\n" // Missing colon
        "  X-Valid: Value";

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_TRUE(nanorouter_parse_headers_file(file_content, list)); // Should still parse valid parts
    TEST_ASSERT_EQUAL_UINT(1, list->count);

    header_rule_t expected_rule = {
        .from_route = "/test",
        .headers = {{"X-Valid", "Value"}},
        .num_headers = 1
    };
    assert_header_rule_equal(&expected_rule, &list->head->rule);

    nanorouter_header_rule_list_free(list);
}


void test_header_rule_parser(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nanorouter_header_rule_list_create_and_free);
    RUN_TEST(test_nanorouter_header_rule_list_add_single_rule);
    RUN_TEST(test_nanorouter_header_rule_list_add_multiple_rules);
    RUN_TEST(test_nanorouter_parse_headers_file_basic);
    RUN_TEST(test_nanorouter_parse_headers_file_comments_and_empty_lines);
    RUN_TEST(test_nanorouter_parse_headers_file_multiple_headers_same_route);
    RUN_TEST(test_nanorouter_parse_headers_file_no_final_newline);
    RUN_TEST(test_nanorouter_parse_headers_file_malformed_header);
    UNITY_END();
}

// Not adding a main function here, as Unity tests are typically run via a test runner.
