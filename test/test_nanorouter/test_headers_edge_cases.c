#include "test_headers_edge_cases.h"

#include "unity.h"
#include "nanorouter_header_rule_parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Helper function to compare two header_rule_t structs
static void assert_header_rule_equal(const header_rule_t *expected, const header_rule_t *actual) {
    TEST_ASSERT_EQUAL_STRING(expected->from_route, actual->from_route);
    TEST_ASSERT_EQUAL_UINT8(expected->num_headers, actual->num_headers);
    for (uint8_t i = 0; i < expected->num_headers; i++) {
        TEST_ASSERT_EQUAL_STRING(expected->headers[i].key, actual->headers[i].key);
        TEST_ASSERT_EQUAL_STRING(expected->headers[i].value, actual->headers[i].value);
    }
}

// Test edge cases for line length validation
void test_nanorouter_parse_headers_file_line_too_long() {
    // Create a line that exceeds the buffer size
    char long_line[NR_MAX_HEADER_VALUE_LEN + NR_MAX_HEADER_KEY_LEN + 50];
    memset(long_line, 'a', sizeof(long_line) - 1);
    long_line[sizeof(long_line) - 1] = '\0';
    
    // Create file content with the long line
    char file_content[512];
    snprintf(file_content, sizeof(file_content), "/test\n  X-Header: %s\n", long_line);

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    // Should handle long lines gracefully
    bool result = nanorouter_parse_headers_file(file_content, list);
    TEST_ASSERT_TRUE(result);
    // Should still parse the route and skip the overly long header
    TEST_ASSERT_EQUAL_UINT(1, list->count);

    nanorouter_header_rule_list_free(list);
}

// Test edge cases for empty and null inputs
void test_nanorouter_parse_headers_file_null_file_content() {
    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    bool result = nanorouter_parse_headers_file(NULL, list);
    TEST_ASSERT_FALSE(result);

    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_parse_headers_file_null_rule_list() {
    bool result = nanorouter_parse_headers_file("/test\n  X-Header: Value", NULL);
    TEST_ASSERT_FALSE(result);
}

void test_nanorouter_parse_headers_file_empty_file_content() {
    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    bool result = nanorouter_parse_headers_file("", list);
    TEST_ASSERT_TRUE(result); // Empty content should be considered valid
    TEST_ASSERT_EQUAL_UINT(0, list->count);

    nanorouter_header_rule_list_free(list);
}

// Test edge cases for malformed header lines
void test_nanorouter_parse_headers_file_header_without_colon() {
    const char *file_content =
        "/test\n"
        "  MalformedHeader\n" // Missing colon
        "  X-Valid: Value\n"
        "  AnotherMalformed\n"; // Another malformed line

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    bool result = nanorouter_parse_headers_file(file_content, list);
    TEST_ASSERT_TRUE(result);
    // Should only parse valid headers
    TEST_ASSERT_EQUAL_UINT(1, list->count);

    header_rule_t expected_rule = {
        .from_route = "/test",
        .headers = {{"X-Valid", "Value"}},
        .num_headers = 1
    };
    assert_header_rule_equal(&expected_rule, &list->head->rule);

    nanorouter_header_rule_list_free(list);
}

// Test edge case for maximum headers per rule exceeded
void test_nanorouter_parse_headers_file_too_many_headers() {
    char file_content[512] = "/test\n";
    
    // Add exactly NR_MAX_HEADERS_PER_RULES headers
    for (int i = 0; i < NR_MAX_HEADERS_PER_RULE; i++) {
        char header_line[64];
        snprintf(header_line, sizeof(header_line), "  X-Header%d: Value%d\n", i, i);
        strcat(file_content, header_line);
    }
    
    // Add one more header to exceed the limit
    strcat(file_content, "  X-Extra: ExtraValue\n");

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    bool result = nanorouter_parse_headers_file(file_content, list);
    TEST_ASSERT_TRUE(result);
    // Should limit to NR_MAX_HEADERS_PER_RULE headers
    TEST_ASSERT_EQUAL_UINT(NR_MAX_HEADERS_PER_RULE, list->head->rule.num_headers);

    nanorouter_header_rule_list_free(list);
}

// Test edge cases for header key/value length limits
void test_nanorouter_parse_headers_file_header_key_too_long() {
    char long_key[NR_MAX_HEADER_KEY_LEN + 10];
    memset(long_key, 'k', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    
    char file_content[512];
    snprintf(file_content, sizeof(file_content), "/test\n  %s: Value\n", long_key);

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    bool result = nanorouter_parse_headers_file(file_content, list);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT(1, list->count);
    
    // Key should be truncated to NR_MAX_HEADER_KEY_LEN
    TEST_ASSERT_EQUAL_UINT(NR_MAX_HEADER_KEY_LEN, strlen(list->head->rule.headers[0].key));

    nanorouter_header_rule_list_free(list);
}

void test_nanorouter_parse_headers_file_header_value_too_long() {
    char long_value[NR_MAX_HEADER_VALUE_LEN + 10];
    memset(long_value, 'v', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    
    char file_content[512];
    snprintf(file_content, sizeof(file_content), "/test\n  X-Header: %s\n", long_value);

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    bool result = nanorouter_parse_headers_file(file_content, list);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT(1, list->count);
    
    // Value should be truncated to NR_MAX_HEADER_VALUE_LEN
    TEST_ASSERT_EQUAL_UINT(NR_MAX_HEADER_VALUE_LEN - 1, strlen(list->head->rule.headers[0].value));

    nanorouter_header_rule_list_free(list);
}

// Test edge case for route length limits
void test_nanorouter_parse_headers_file_route_too_long() {
    char long_route[NR_MAX_ROUTE_LEN + 10];
    long_route[0] = '/';
    memset(long_route + 1, 'r', sizeof(long_route) - 2);
    long_route[sizeof(long_route) - 1] = '\0';
    
    char file_content[512];
    snprintf(file_content, sizeof(file_content), "%s\n  X-Header: Value\n", long_route);

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    bool result = nanorouter_parse_headers_file(file_content, list);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT(1, list->count);
    
    // Route should be truncated to NR_MAX_ROUTE_LEN - 1
    TEST_ASSERT_EQUAL_UINT(NR_MAX_ROUTE_LEN - 1, strlen(list->head->rule.from_route));

    nanorouter_header_rule_list_free(list);
}

// Test edge case for header lines outside rule blocks
void test_nanorouter_parse_headers_file_header_outside_rule() {
    const char *file_content =
        "  X-Outside: Value\n" // Header before any route
        "/test\n"
        "  X-Inside: Value\n";

    nanorouter_header_rule_list_t *list = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);

    bool result = nanorouter_parse_headers_file(file_content, list);
    TEST_ASSERT_TRUE(result);
    // Should only parse the valid rule
    TEST_ASSERT_EQUAL_UINT(1, list->count);

    header_rule_t expected_rule = {
        .from_route = "/test",
        .headers = {{"X-Inside", "Value"}},
        .num_headers = 1
    };
    assert_header_rule_equal(&expected_rule, &list->head->rule);

    nanorouter_header_rule_list_free(list);
}

// Main test runner
int test_headers_edge_cases() {
    UNITY_BEGIN();
    
    RUN_TEST(test_nanorouter_parse_headers_file_line_too_long);
    RUN_TEST(test_nanorouter_parse_headers_file_null_file_content);
    RUN_TEST(test_nanorouter_parse_headers_file_null_rule_list);
    RUN_TEST(test_nanorouter_parse_headers_file_empty_file_content);
    RUN_TEST(test_nanorouter_parse_headers_file_header_without_colon);
    RUN_TEST(test_nanorouter_parse_headers_file_too_many_headers);
    RUN_TEST(test_nanorouter_parse_headers_file_header_key_too_long);
    RUN_TEST(test_nanorouter_parse_headers_file_header_value_too_long);
    RUN_TEST(test_nanorouter_parse_headers_file_route_too_long);
    RUN_TEST(test_nanorouter_parse_headers_file_header_outside_rule);
    
    return UNITY_END();
}
