#ifndef TEST_NANOROUTER_RULE_PARSER_H
#define TEST_NANOROUTER_RULE_PARSER_H

#include "unity.h"
#include "nanorouter_redirect_rule_parser.h" // Include the header with redirect_rule_t and parsing functions
#include <string.h> // For memset, strlen
#include <stdlib.h> // For free
#include <stdbool.h> // For bool type
#include <stdio.h>   // For printf
#include "nanorouter.h" // For nr_redirect_part_type_t

// Helper function to initialize a redirect_rule_t struct
void init_redirect_rule(redirect_rule_t *rule) {
    memset(rule, 0, sizeof(redirect_rule_t));
}

void test_nr_parse_redirect_rule_basic_301(void) {
    // Arrange
    const char *rule_line = "/from /to 301";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/from", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/to", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(301, parsed_rule.status_code);
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_conditions);
}

void test_nr_parse_redirect_rule_basic_200_force(void) {
    // Arrange
    const char *rule_line = "/from /to 200!";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/from", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/to", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(200, parsed_rule.status_code);
    TEST_ASSERT_TRUE(parsed_rule.force);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_conditions);
}

void test_nr_parse_redirect_rule_query_param(void) {
    // Arrange
    const char *rule_line = "/store id=:id /blog/:id 301";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/store", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/blog/:id", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(301, parsed_rule.status_code);
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(1, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL_STRING("id", parsed_rule.query_params[0].key);
    TEST_ASSERT_EQUAL_STRING(":id", parsed_rule.query_params[0].value);
    TEST_ASSERT_TRUE(parsed_rule.query_params[0].is_present);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_conditions);
}

void test_nr_parse_redirect_rule_condition_country(void) {
    // Arrange
    const char *rule_line = "/ /anz 302 Country=au,nz";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/anz", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(302, parsed_rule.status_code);
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL(1, parsed_rule.num_conditions);
    TEST_ASSERT_EQUAL_STRING("Country", parsed_rule.conditions[0].key);
    TEST_ASSERT_EQUAL_STRING("au,nz", parsed_rule.conditions[0].value);
    TEST_ASSERT_TRUE(parsed_rule.conditions[0].is_present);
}

void test_nr_parse_redirect_rule_splat(void) {
    // Arrange
    const char *rule_line = "/news/* /blog/:splat";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/news/*", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/blog/:splat", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(0, parsed_rule.status_code); // Default status for rewrite
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_conditions);
}

void test_nr_parse_redirect_rule_full_url_proxy(void) {
    // Arrange
    const char *rule_line = "/api/* https://api.example.com/:splat 200";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/api/*", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("https://api.example.com/:splat", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(200, parsed_rule.status_code);
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_conditions);
}

void test_nr_parse_redirect_rule_comment_and_empty_lines(void) {
    // Arrange
    const char *comment_rule = "# This is a comment";
    const char *empty_rule = "   ";
    redirect_rule_t parsed_rule;

    // Act & Assert for comment
    init_redirect_rule(&parsed_rule);
    bool success_comment = nr_parse_redirect_rule(comment_rule, strlen(comment_rule), &parsed_rule);
    TEST_ASSERT_FALSE(success_comment); // Should return false for comments/empty lines

    // Act & Assert for empty line
    init_redirect_rule(&parsed_rule);
    bool success_empty = nr_parse_redirect_rule(empty_rule, strlen(empty_rule), &parsed_rule);
    TEST_ASSERT_FALSE(success_empty); // Should return false for comments/empty lines
}

void test_nr_parse_redirect_rule_multiple_query_params(void) {
    // Arrange
    const char *rule_line = "/articles id=:id tag=:tag /posts/:tag/:id 301";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/articles", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/posts/:tag/:id", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(301, parsed_rule.status_code);
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(2, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL_STRING("id", parsed_rule.query_params[0].key);
    TEST_ASSERT_EQUAL_STRING(":id", parsed_rule.query_params[0].value);
    TEST_ASSERT_TRUE(parsed_rule.query_params[0].is_present);
    TEST_ASSERT_EQUAL_STRING("tag", parsed_rule.query_params[1].key);
    TEST_ASSERT_EQUAL_STRING(":tag", parsed_rule.query_params[1].value);
    TEST_ASSERT_TRUE(parsed_rule.query_params[1].is_present);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_conditions);
}

void test_nr_parse_redirect_rule_multiple_conditions(void) {
    // Arrange
    const char *rule_line = "/en/* /en/404.html 404 Language=en Country=us";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/en/*", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/en/404.html", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(404, parsed_rule.status_code);
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL(2, parsed_rule.num_conditions);
    TEST_ASSERT_EQUAL_STRING("Language", parsed_rule.conditions[0].key);
    TEST_ASSERT_EQUAL_STRING("en", parsed_rule.conditions[0].value);
    TEST_ASSERT_TRUE(parsed_rule.conditions[0].is_present);
    TEST_ASSERT_EQUAL_STRING("Country", parsed_rule.conditions[1].key);
    TEST_ASSERT_EQUAL_STRING("us", parsed_rule.conditions[1].value);
    TEST_ASSERT_TRUE(parsed_rule.conditions[1].is_present);
}

void test_nr_parse_redirect_rule_only_from_to(void) {
    // Arrange
    const char *rule_line = "/old /new";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/old", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/new", parsed_rule.to_route);
    TEST_ASSERT_EQUAL(0, parsed_rule.status_code); // Default status for rewrite
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_conditions);
}

void test_nr_parse_redirect_rule_unknown_part(void) {
    // Arrange
    const char *rule_line = "/from /to unknown_part 301";
    redirect_rule_t parsed_rule;
    init_redirect_rule(&parsed_rule);

    // Act
    bool success = nr_parse_redirect_rule(rule_line, strlen(rule_line), &parsed_rule);

    // Assert
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("/from", parsed_rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/to", parsed_rule.to_route);
    // The 'unknown_part' should be ignored or handled gracefully by the parser.
    // For now, we assert that the known parts are correct and other counts are zero.
    TEST_ASSERT_EQUAL(301, parsed_rule.status_code);
    TEST_ASSERT_FALSE(parsed_rule.force);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_query_params);
    TEST_ASSERT_EQUAL(0, parsed_rule.num_conditions);
}

// Helper struct for individual part data in redirect rule tests
typedef struct {
    const char *expected_token;
    size_t expected_token_len;
    nr_redirect_part_type_t expected_part_type;
} ExpectedPart;

// Combined user data for redirect rule tests
typedef struct {
    ExpectedPart *expected_parts;
    size_t num_expected_parts;
    size_t current_part_index;
} TestContext;

// Mock callback function for nr_process_redirect_rule tests
void mock_redirect_rule_part_callback(const char *token, size_t token_len, nr_redirect_part_type_t part_type, void *user_data) {
    printf("called %s\n", token);
    TestContext *context = (TestContext *)user_data;

    TEST_ASSERT_NOT_NULL(context);
    TEST_ASSERT_LESS_THAN_MESSAGE(context->num_expected_parts,context->current_part_index, "Callback called more times than expected");

    ExpectedPart *expected = &context->expected_parts[context->current_part_index];

    TEST_ASSERT_EQUAL_STRING_LEN(expected->expected_token, token, token_len);
    TEST_ASSERT_EQUAL(expected->expected_token_len, token_len);
    TEST_ASSERT_EQUAL_INT(expected->expected_part_type, part_type);

    context->current_part_index++;
}

void test_nr_process_redirect_rule_basic_301(void) {
    // Arrange
    const char *rule = "/from /to 301";
    ExpectedPart expected[] = {
        {"/from", 5, NR_REDIRECT_PART_FROM_ROUTE},
        {"/to", 3, NR_REDIRECT_PART_TO_ROUTE},
        {"301", 3, NR_REDIRECT_PART_STATUS}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    printf("expected %d\n", context.num_expected_parts);

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_basic_200_force(void) {
    // Arrange
    const char *rule = "/from /to 200!";
    ExpectedPart expected[] = {
        {"/from", 5, NR_REDIRECT_PART_FROM_ROUTE},
        {"/to", 3, NR_REDIRECT_PART_TO_ROUTE},
        {"200", 3, NR_REDIRECT_PART_STATUS},
        {"!", 1, NR_REDIRECT_PART_FORCE}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_query_param(void) {
    // Arrange
    const char *rule = "/store id=:id /blog/:id 301";
    ExpectedPart expected[] = {
        {"/store", 6, NR_REDIRECT_PART_FROM_ROUTE},
        {"id=:id", 6, NR_REDIRECT_PART_QUERY},
        {"/blog/:id", 9, NR_REDIRECT_PART_TO_ROUTE},
        {"301", 3, NR_REDIRECT_PART_STATUS}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_condition_country(void) {
    // Arrange
    const char *rule = "/ /anz 302 Country=au,nz";
    ExpectedPart expected[] = {
        {"/", 1, NR_REDIRECT_PART_FROM_ROUTE},
        {"/anz", 4, NR_REDIRECT_PART_TO_ROUTE},
        {"302", 3, NR_REDIRECT_PART_STATUS},
        {"Country=au,nz", 13, NR_REDIRECT_PART_CONDITION}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_splat(void) {
    // Arrange
    const char *rule = "/news/* /blog/:splat";
    ExpectedPart expected[] = {
        {"/news/*", 7, NR_REDIRECT_PART_FROM_ROUTE},
        {"/blog/:splat", 12, NR_REDIRECT_PART_TO_ROUTE}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_full_url_proxy(void) {
    // Arrange
    const char *rule = "/api/* https://api.example.com/:splat 200";
    ExpectedPart expected[] = {
        {"/api/*", 6, NR_REDIRECT_PART_FROM_ROUTE},
        {"https://api.example.com/:splat", 30, NR_REDIRECT_PART_TO_ROUTE},
        {"200", 3, NR_REDIRECT_PART_STATUS}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_comment_and_empty_lines(void) {
    // Arrange
    const char *comment_rule = "# This is a comment";
    const char *empty_rule = "   ";
    TestContext context = {
        .expected_parts = NULL,
        .num_expected_parts = 0,
        .current_part_index = 0
    };

    // Act & Assert for comment
    nr_process_redirect_rule(comment_rule, strlen(comment_rule), mock_redirect_rule_part_callback, &context);
    TEST_ASSERT_EQUAL(0, context.current_part_index); // No callbacks should be made

    // Act & Assert for empty line
    nr_process_redirect_rule(empty_rule, strlen(empty_rule), mock_redirect_rule_part_callback, &context);
    TEST_ASSERT_EQUAL(0, context.current_part_index); // No callbacks should be made
}

void test_nr_process_redirect_rule_multiple_query_params(void) {
    // Arrange
    const char *rule = "/articles id=:id tag=:tag /posts/:tag/:id 301";
    ExpectedPart expected[] = {
        {"/articles", 9, NR_REDIRECT_PART_FROM_ROUTE},
        {"id=:id", 6, NR_REDIRECT_PART_QUERY},
        {"tag=:tag", 8, NR_REDIRECT_PART_QUERY},
        {"/posts/:tag/:id", 15, NR_REDIRECT_PART_TO_ROUTE},
        {"301", 3, NR_REDIRECT_PART_STATUS}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_multiple_conditions(void) {
    // Arrange
    const char *rule = "/en/* /en/404.html 404 Language=en Country=us";
    ExpectedPart expected[] = {
        {"/en/*", 5, NR_REDIRECT_PART_FROM_ROUTE},
        {"/en/404.html", 12, NR_REDIRECT_PART_TO_ROUTE},
        {"404", 3, NR_REDIRECT_PART_STATUS},
        {"Language=en", 11, NR_REDIRECT_PART_CONDITION},
        {"Country=us", 10, NR_REDIRECT_PART_CONDITION}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_only_from_to(void) {
    // Arrange
    const char *rule = "/old /new";
    ExpectedPart expected[] = {
        {"/old", 4, NR_REDIRECT_PART_FROM_ROUTE},
        {"/new", 4, NR_REDIRECT_PART_TO_ROUTE}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_nr_process_redirect_rule_unknown_part(void) {
    // Arrange
    const char *rule = "/from /to unknown_part 301";
    ExpectedPart expected[] = {
        {"/from", 5, NR_REDIRECT_PART_FROM_ROUTE},
        {"/to", 3, NR_REDIRECT_PART_TO_ROUTE},
        {"unknown_part", 12, NR_REDIRECT_PART_UNKNOWN},
        {"301", 3, NR_REDIRECT_PART_STATUS}
    };
    TestContext context = {
        .expected_parts = expected,
        .num_expected_parts = sizeof(expected) / sizeof(expected[0]),
        .current_part_index = 0
    };

    // Act
    nr_process_redirect_rule(rule, strlen(rule), mock_redirect_rule_part_callback, &context);

    // Assert
    TEST_ASSERT_EQUAL(context.num_expected_parts, context.current_part_index);
}

void test_rule_parser_redirect_rules(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nr_process_redirect_rule_basic_301);
    RUN_TEST(test_nr_process_redirect_rule_basic_200_force);
    RUN_TEST(test_nr_process_redirect_rule_query_param);
    RUN_TEST(test_nr_process_redirect_rule_condition_country);
    RUN_TEST(test_nr_process_redirect_rule_splat);
    RUN_TEST(test_nr_process_redirect_rule_full_url_proxy);
    RUN_TEST(test_nr_process_redirect_rule_comment_and_empty_lines);
    RUN_TEST(test_nr_process_redirect_rule_multiple_query_params);
    RUN_TEST(test_nr_process_redirect_rule_multiple_conditions);
    RUN_TEST(test_nr_process_redirect_rule_only_from_to);
    RUN_TEST(test_nr_process_redirect_rule_unknown_part);
    UNITY_END();
}

void test_rule_parser(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nr_parse_redirect_rule_basic_301);
    RUN_TEST(test_nr_parse_redirect_rule_basic_200_force);
    RUN_TEST(test_nr_parse_redirect_rule_query_param);
    RUN_TEST(test_nr_parse_redirect_rule_condition_country);
    RUN_TEST(test_nr_parse_redirect_rule_splat);
    RUN_TEST(test_nr_parse_redirect_rule_full_url_proxy);
    RUN_TEST(test_nr_parse_redirect_rule_comment_and_empty_lines);
    RUN_TEST(test_nr_parse_redirect_rule_multiple_query_params);
    RUN_TEST(test_nr_parse_redirect_rule_multiple_conditions);
    RUN_TEST(test_nr_parse_redirect_rule_only_from_to);
    RUN_TEST(test_nr_parse_redirect_rule_unknown_part);
    UNITY_END();
}

#endif // TEST_NANOROUTER_RULE_PARSER_H
