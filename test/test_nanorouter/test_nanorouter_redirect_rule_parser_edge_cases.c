#include "unity.h"
#include "nanorouter_redirect_rule_parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // For strtol

// Helper to print rule for debugging
// void print_rule(const redirect_rule_t *rule) {
//     printf("FROM: %s\n", rule->from_route);
//     printf("TO: %s\n", rule->to_route);
//     printf("STATUS: %d (Force: %s)\n", rule->status_code, rule->force ? "true" : "false");
//     for (int i = 0; i < rule->num_query_params; i++) {
//         printf("  Query Param %d: %s=%s (is_present: %s)\n", i, rule->query_params[i].key, rule->query_params[i].value, rule->query_params[i].is_present ? "true" : "false");
//     }
//     for (int i = 0; i < rule->num_conditions; i++) {
//         printf("  Condition %d: %s=%s (is_present: %s)\n", i, rule->conditions[i].key, rule->conditions[i].value, rule->conditions[i].is_present ? "true" : "false");
//     }
// }

// void setUp(void) {
//     // set stuff up here
// }

// void tearDown(void) {
//     // clean stuff up here
// }

void test_nr_parse_redirect_rule_invalid_status_code_overflow(void) {
    redirect_rule_t rule;
    const char *rule_line = "/from /to 9999999999"; // Number too large for uint16_t
    TEST_ASSERT_FALSE(nr_parse_redirect_rule(rule_line, strlen(rule_line), &rule));
    // The rule should not be successfully parsed due to an invalid status code.
    TEST_ASSERT_TRUE(rule.parsing_error);
    TEST_ASSERT_EQUAL_STRING("", rule.from_route); // Should be empty as parsing failed
    TEST_ASSERT_EQUAL_STRING("", rule.to_route);   // Should be empty as parsing failed
    TEST_ASSERT_EQUAL(0, rule.status_code);        // Should be 0 as parsing failed
}

void test_nr_parse_redirect_rule_invalid_status_code_non_numeric(void) {
    redirect_rule_t rule;
    const char *rule_line = "/from /to invalid_status";
    TEST_ASSERT_FALSE(nr_parse_redirect_rule(rule_line, strlen(rule_line), &rule));
    // The rule should not be successfully parsed due to an invalid status code.
    TEST_ASSERT_TRUE(rule.parsing_error);
    TEST_ASSERT_EQUAL_STRING("", rule.from_route); // Should be empty as parsing failed
    TEST_ASSERT_EQUAL_STRING("", rule.to_route);   // Should be empty as parsing failed
    TEST_ASSERT_EQUAL(0, rule.status_code);        // Should be 0 as parsing failed
}

void test_nr_parse_redirect_rule_valid_status_code(void) {
    redirect_rule_t rule;
    const char *rule_line = "/from /to 301";
    TEST_ASSERT_TRUE(nr_parse_redirect_rule(rule_line, strlen(rule_line), &rule));
    TEST_ASSERT_EQUAL(301, rule.status_code);
    TEST_ASSERT_EQUAL_STRING("/from", rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/to", rule.to_route);
}

int test_parser_edge_cases(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nr_parse_redirect_rule_invalid_status_code_overflow);
    RUN_TEST(test_nr_parse_redirect_rule_invalid_status_code_non_numeric);
    RUN_TEST(test_nr_parse_redirect_rule_valid_status_code);
    return UNITY_END();
}
