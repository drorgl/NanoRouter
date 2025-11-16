#include "unity.h"
#include "nanorouter_headers_middleware.h"
#include "nanorouter_header_rule_parser.h" // To create rules for testing
#include "nanorouter_condition_matching.h" // For nanorouter_request_context_t
#include <string.h>
#include <stdlib.h>

// Helper function to compare two nanorouter_header_response_t structs
static void assert_header_response_equal(const nanorouter_header_response_t *expected, const nanorouter_header_response_t *actual) {
    TEST_ASSERT_EQUAL_UINT8(expected->num_headers, actual->num_headers);
    for (uint8_t i = 0; i < expected->num_headers; i++) {
        bool found = false;
        for (uint8_t j = 0; j < actual->num_headers; j++) {
            if (strcasecmp(expected->headers[i].key, actual->headers[j].key) == 0 &&
                strcmp(expected->headers[i].value, actual->headers[j].value) == 0) {
                found = true;
                break;
            }
        }
        TEST_ASSERT_TRUE_MESSAGE(found, "Expected header not found or value mismatch");
    }
}

// --- Test Cases ---

void test_nanorouter_process_header_request_no_match(void) {
    nanorouter_header_rule_list_t *rules = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(rules);

    header_rule_t rule1 = {
        .from_route = "/specific",
        .headers = {{"X-Test", "Value"}},
        .num_headers = 1
    };
    nanorouter_header_rule_list_add_rule(rules, &rule1);

    nanorouter_header_response_t response = {0};
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    TEST_ASSERT_FALSE(nanorouter_process_header_request("/nomatch", rules, &response, &context));
    TEST_ASSERT_EQUAL_UINT8(0, response.num_headers);

    nanorouter_header_rule_list_free(rules);
}

void test_nanorouter_process_header_request_basic_match(void) {
    nanorouter_header_rule_list_t *rules = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(rules);

    header_rule_t rule1 = {
        .from_route = "/*",
        .headers = {{"X-Frame-Options", "DENY"}},
        .num_headers = 1
    };
    nanorouter_header_rule_list_add_rule(rules, &rule1);

    nanorouter_header_response_t response = {0};
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    TEST_ASSERT_TRUE(nanorouter_process_header_request("/any/path", rules, &response, &context));
    TEST_ASSERT_EQUAL_UINT8(1, response.num_headers);

    nanorouter_header_response_t expected_response = {
        .headers = {{"X-Frame-Options", "DENY"}},
        .num_headers = 1
    };
    assert_header_response_equal(&expected_response, &response);

    nanorouter_header_rule_list_free(rules);
}

void test_nanorouter_process_header_request_path_specific_match(void) {
    nanorouter_header_rule_list_t *rules = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(rules);

    header_rule_t rule1 = {
        .from_route = "/templates/index.html",
        .headers = {{"X-Frame-Options", "DENY"}},
        .num_headers = 1
    };
    header_rule_t rule2 = {
        .from_route = "/templates/index2.html",
        .headers = {{"X-Frame-Options", "SAMEORIGIN"}},
        .num_headers = 1
    };
    nanorouter_header_rule_list_add_rule(rules, &rule1);
    nanorouter_header_rule_list_add_rule(rules, &rule2);

    nanorouter_header_response_t response = {0};
    TEST_ASSERT_TRUE(nanorouter_process_header_request("/templates/index.html", rules, &response,NULL));
    TEST_ASSERT_EQUAL_UINT8(1, response.num_headers);
    nanorouter_header_response_t expected_response1 = {
        .headers = {{"X-Frame-Options", "DENY"}},
        .num_headers = 1
    };
    assert_header_response_equal(&expected_response1, &response);

    memset(&response, 0, sizeof(nanorouter_header_response_t)); // Clear response
    TEST_ASSERT_TRUE(nanorouter_process_header_request("/templates/index2.html", rules, &response,NULL));
    TEST_ASSERT_EQUAL_UINT8(1, response.num_headers);
    nanorouter_header_response_t expected_response2 = {
        .headers = {{"X-Frame-Options", "SAMEORIGIN"}},
        .num_headers = 1
    };
    assert_header_response_equal(&expected_response2, &response);

    nanorouter_header_rule_list_free(rules);
}

void test_nanorouter_process_header_request_wildcard_match(void) {
    nanorouter_header_rule_list_t *rules = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(rules);

    header_rule_t rule1 = {
        .from_route = "/api/*",
        .headers = {{"Access-Control-Allow-Origin", "*"}},
        .num_headers = 1
    };
    nanorouter_header_rule_list_add_rule(rules, &rule1);

    nanorouter_header_response_t response = {0};
    TEST_ASSERT_TRUE(nanorouter_process_header_request("/api/users/123", rules, &response,NULL));
    TEST_ASSERT_EQUAL_UINT8(1, response.num_headers);
    nanorouter_header_response_t expected_response = {
        .headers = {{"Access-Control-Allow-Origin", "*"}},
        .num_headers = 1
    };
    assert_header_response_equal(&expected_response, &response);

    nanorouter_header_rule_list_free(rules);
}

void test_nanorouter_process_header_request_multi_value_headers(void) {
    nanorouter_header_rule_list_t *rules = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(rules);

    header_rule_t rule1 = {
        .from_route = "/*",
        .headers = {
            {"Cache-Control", "max-age=0"},
            {"Cache-Control", "no-cache"},
            {"Cache-Control", "no-store"}
        },
        .num_headers = 3
    };
    nanorouter_header_rule_list_add_rule(rules, &rule1);

    nanorouter_header_response_t response = {0};
    TEST_ASSERT_TRUE(nanorouter_process_header_request("/any", rules, &response,NULL));
    TEST_ASSERT_EQUAL_UINT8(1, response.num_headers); // Should be one concatenated header

    nanorouter_header_response_t expected_response = {
        .headers = {{"Cache-Control", "max-age=0,no-cache,no-store"}},
        .num_headers = 1
    };
    assert_header_response_equal(&expected_response, &response);

    nanorouter_header_rule_list_free(rules);
}

void test_nanorouter_process_header_request_ignored_headers(void) {
    nanorouter_header_rule_list_t *rules = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(rules);

    header_rule_t rule1 = {
        .from_route = "/*",
        .headers = {
            {"X-Custom-Header", "CustomValue"},
            {"Content-Length", "123"}, // Ignored
            {"Server", "MyServer"}      // Ignored
        },
        .num_headers = 3
    };
    nanorouter_header_rule_list_add_rule(rules, &rule1);

    nanorouter_header_response_t response = {0};
    TEST_ASSERT_TRUE(nanorouter_process_header_request("/any", rules, &response,NULL));
    TEST_ASSERT_EQUAL_UINT8(1, response.num_headers);

    nanorouter_header_response_t expected_response = {
        .headers = {{"X-Custom-Header", "CustomValue"}},
        .num_headers = 1
    };
    assert_header_response_equal(&expected_response, &response);

    nanorouter_header_rule_list_free(rules);
}

void test_nanorouter_process_header_request_no_duplicate_values_from_different_rules(void) {
    nanorouter_header_rule_list_t *rules = nanorouter_header_rule_list_create();
    TEST_ASSERT_NOT_NULL(rules);

    // Rule 1: Matches /path/* and adds X-Header: Value1
    header_rule_t rule1 = {
        .from_route = "/path/*",
        .headers = {{"X-Header", "Value1"}},
        .num_headers = 1
    };
    nanorouter_header_rule_list_add_rule(rules, &rule1);

    // Rule 2: Matches /path/subpath and also adds X-Header: Value1
    header_rule_t rule2 = {
        .from_route = "/path/subpath",
        .headers = {{"X-Header", "Value1"}},
        .num_headers = 1
    };
    nanorouter_header_rule_list_add_rule(rules, &rule2);

    nanorouter_header_response_t response = {0};
    // Request matches both rules
    TEST_ASSERT_TRUE(nanorouter_process_header_request("/path/subpath", rules, &response,NULL));
    
    // Expect only one "X-Header: Value1"
    TEST_ASSERT_EQUAL_UINT8(1, response.num_headers);
    nanorouter_header_response_t expected_response = {
        .headers = {{"X-Header", "Value1"}},
        .num_headers = 1
    };
    assert_header_response_equal(&expected_response, &response);

    nanorouter_header_rule_list_free(rules);
}


int test_nanorouter_headers_middleware(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nanorouter_process_header_request_no_match);
    RUN_TEST(test_nanorouter_process_header_request_basic_match);
    RUN_TEST(test_nanorouter_process_header_request_path_specific_match);
    RUN_TEST(test_nanorouter_process_header_request_wildcard_match);
    RUN_TEST(test_nanorouter_process_header_request_multi_value_headers);
    RUN_TEST(test_nanorouter_process_header_request_ignored_headers);
    RUN_TEST(test_nanorouter_process_header_request_no_duplicate_values_from_different_rules);
    return UNITY_END();
}

// Not adding a main function here, as Unity tests are typically run via a test runner.
