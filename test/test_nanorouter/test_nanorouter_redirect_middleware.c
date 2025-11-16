#include "unity.h"
#include "nanorouter_redirect_middleware.h"
#include "nanorouter_redirect_rule_parser.h" // For redirect_rule_t
#include "nanorouter_condition_matching.h" // For nanorouter_request_context_t
#include <string.h>
#include <stdlib.h>
#include <stdio.h> // For snprintf

// Helper function to initialize a simple redirect_rule_t for testing
redirect_rule_t create_test_rule(const char *from, const char *to, uint16_t status, bool force) {
    redirect_rule_t rule = {0}; // Initialize all members to 0
    strncpy(rule.from_route, from, NR_MAX_ROUTE_LEN);
    rule.from_route[NR_MAX_ROUTE_LEN] = '\0';
    strncpy(rule.to_route, to, NR_MAX_ROUTE_LEN);
    rule.to_route[NR_MAX_ROUTE_LEN] = '\0';
    rule.status_code = status;
    rule.force = force;
    rule.num_query_params = 0;
    rule.num_conditions = 0;
    return rule;
}

// Helper function to add a query parameter to a redirect_rule_t
void add_query_param_to_rule(redirect_rule_t *rule, const char *key, const char *value, bool is_present) {
    if (rule->num_query_params < NR_MAX_QUERY_ITEMS) {
        strncpy(rule->query_params[rule->num_query_params].key, key, NR_MAX_QUERY_KEY_LEN);
        rule->query_params[rule->num_query_params].key[NR_MAX_QUERY_KEY_LEN] = '\0';
        strncpy(rule->query_params[rule->num_query_params].value, value, NR_MAX_QUERY_VALUE_LEN);
        rule->query_params[rule->num_query_params].value[NR_MAX_QUERY_VALUE_LEN] = '\0';
        rule->query_params[rule->num_query_params].is_present = is_present;
        rule->num_query_params++;
    }
}

// // Helper function to add a condition to a redirect_rule_t
// void add_condition_to_rule(redirect_rule_t *rule, const char *key, const char *value, bool is_present) {
//     if (rule->num_conditions < NR_MAX_CONDITION_ITEMS) {
//         strncpy(rule->conditions[rule->num_conditions].key, key, NR_MAX_CONDITION_KEY_LEN);
//         rule->conditions[rule->num_conditions].key[NR_MAX_CONDITION_KEY_LEN] = '\0';
//         strncpy(rule->conditions[rule->num_conditions].value, value, NR_MAX_CONDITION_VALUE_LEN);
//         rule->conditions[rule->num_conditions].value[NR_MAX_CONDITION_VALUE_LEN] = '\0';
//         rule->conditions[rule->num_conditions].is_present = is_present;
//         rule->num_conditions++;
//     }
// }

// --- Test Cases for nanorouter_redirect_rule_list_create ---
void test_nanorouter_redirect_rule_list_create_success() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_NULL(list->head);
    TEST_ASSERT_EQUAL(0, list->count);
    nanorouter_redirect_rule_list_free(list);
}

// --- Test Cases for nanorouter_redirect_rule_list_add_rule ---
void test_nanorouter_redirect_rule_list_add_rule_single_rule() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/old", "/new", 301, false);

    bool result = nanorouter_redirect_rule_list_add_rule(list, &rule1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(list->head);
    TEST_ASSERT_EQUAL(1, list->count);
    TEST_ASSERT_EQUAL_STRING("/old", list->head->rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/new", list->head->rule.to_route);
    TEST_ASSERT_EQUAL(301, list->head->rule.status_code);
    TEST_ASSERT_FALSE(list->head->rule.force);
    TEST_ASSERT_NULL(list->head->next);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_redirect_rule_list_add_rule_multiple_rules() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/old1", "/new1", 301, false);
    redirect_rule_t rule2 = create_test_rule("/old2", "/new2", 200, true);

    nanorouter_redirect_rule_list_add_rule(list, &rule1);
    nanorouter_redirect_rule_list_add_rule(list, &rule2);

    TEST_ASSERT_EQUAL(2, list->count);
    TEST_ASSERT_NOT_NULL(list->head);
    TEST_ASSERT_NOT_NULL(list->head->next);
    TEST_ASSERT_NULL(list->head->next->next);

    TEST_ASSERT_EQUAL_STRING("/old1", list->head->rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/new1", list->head->rule.to_route);
    TEST_ASSERT_EQUAL(301, list->head->rule.status_code);
    TEST_ASSERT_FALSE(list->head->rule.force);

    TEST_ASSERT_EQUAL_STRING("/old2", list->head->next->rule.from_route);
    TEST_ASSERT_EQUAL_STRING("/new2", list->head->next->rule.to_route);
    TEST_ASSERT_EQUAL(200, list->head->next->rule.status_code);
    TEST_ASSERT_TRUE(list->head->next->rule.force);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_redirect_rule_list_add_rule_null_list() {
    redirect_rule_t rule1 = create_test_rule("/old", "/new", 301, false);
    bool result = nanorouter_redirect_rule_list_add_rule(NULL, &rule1);
    TEST_ASSERT_FALSE(result);
}

void test_nanorouter_redirect_rule_list_add_rule_null_rule_data() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    bool result = nanorouter_redirect_rule_list_add_rule(list, NULL);
    TEST_ASSERT_FALSE(result);
    nanorouter_redirect_rule_list_free(list);
}

// --- Test Cases for nanorouter_redirect_rule_list_free ---
void test_nanorouter_redirect_rule_list_free_empty_list() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    nanorouter_redirect_rule_list_free(list);
    // No direct way to assert memory is freed, but valgrind/memory tools would catch issues.
    // We can assert that calling free on NULL is safe.
}

//REMOVE, useless
void test_nanorouter_redirect_rule_list_free_populated_list() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/old1", "/new1", 301, false);
    redirect_rule_t rule2 = create_test_rule("/old2", "/new2", 200, true);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);
    nanorouter_redirect_rule_list_add_rule(list, &rule2);

    nanorouter_redirect_rule_list_free(list);
    // Again, no direct assertion for freed memory.
}

//REMOVE useless
void test_nanorouter_redirect_rule_list_free_null_list() {
    nanorouter_redirect_rule_list_free(NULL); // Should not crash
    TEST_PASS(); // If it reaches here, it didn't crash
}

// --- Test Cases for nanorouter_process_redirect_request (Placeholder for now) ---
void test_nanorouter_process_redirect_request_no_match() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/test", "/redirect", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/nomatch", list, &response, &context);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("", response.new_url);
    TEST_ASSERT_EQUAL(0, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_basic_match() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/test", "/redirect", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/test", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/redirect", response.new_url);
    TEST_ASSERT_EQUAL(301, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_splat_match() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/news/*", "/blog/:splat", 301, false); // Changed to_route to use ':splat' to match documentation
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/news/2004/01/10/my-story", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/blog/2004/01/10/my-story", response.new_url);
    TEST_ASSERT_EQUAL(301, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_placeholder_match() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/news/:month/:date/:year/:slug", "/blog/:year/:month/:date/:slug", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/news/02/12/2004/my-story", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/blog/2004/02/12/my-story", response.new_url);
    TEST_ASSERT_EQUAL(301, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_query_param_match() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/store", "/blog/:id", 301, false);
    add_query_param_to_rule(&rule1, "id", ":id", true);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/store?id=my-blog-post", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    // According to documentation, query parameters used to populate path placeholders are consumed.
    // However, the general rule is to pass through all query parameters if to_route doesn't specify one.
    // To align with the general passthrough rule, we expect the query param to be passed through.
    TEST_ASSERT_EQUAL_STRING("/blog/my-blog-post?id=my-blog-post", response.new_url);
    TEST_ASSERT_EQUAL(301, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_query_param_passthrough() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/articles", "/posts", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/articles?category=tech&sort=date", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/posts?category=tech&sort=date", response.new_url);
    TEST_ASSERT_EQUAL(301, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_rewrite_200() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/app", "/index.html", 200, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/app", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/index.html", response.new_url);
    TEST_ASSERT_EQUAL(200, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_404_rule() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/non-existent", "/custom-404.html", 404, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/non-existent", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/custom-404.html", response.new_url);
    TEST_ASSERT_EQUAL(404, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_force_redirect() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/best-pets/dogs", "/best-pets/cats.html", 200, true); // Force is true
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/best-pets/dogs", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/best-pets/cats.html", response.new_url);
    TEST_ASSERT_EQUAL(200, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_multiple_rules_precedence() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/news/latest", "/blog/latest-news", 301, false); // More specific
    redirect_rule_t rule2 = create_test_rule("/news/*", "/blog/:splat", 301, false); // More general
    nanorouter_redirect_rule_list_add_rule(list, &rule1);
    nanorouter_redirect_rule_list_add_rule(list, &rule2);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/news/latest", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/blog/latest-news", response.new_url); // Should match rule1
    TEST_ASSERT_EQUAL(301, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_complex_splat_and_query() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/search/*", "/results/:splat", 200, false); // Changed to_route to use ':splat'
    add_query_param_to_rule(&rule1, "q", ":query", true);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request("/search/products?q=electronics&page=1", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    // The 'q' query parameter is used to populate ':query' in the rule, but the documentation implies
    // all query parameters are passed through if the to_route doesn't explicitly define them.
    // Therefore, we expect 'q=electronics' to be passed through along with 'page=1'.
    TEST_ASSERT_EQUAL_STRING("/results/products?q=electronics&page=1", response.new_url);
    TEST_ASSERT_EQUAL(200, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_long_url_truncation() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    char long_from_route[NR_MAX_ROUTE_LEN + 1];
    char long_to_route[NR_MAX_ROUTE_LEN + 1]; // Ensure this buffer is large enough for the test case
    memset(long_from_route, 'a', NR_MAX_ROUTE_LEN);
    long_from_route[NR_MAX_ROUTE_LEN] = '\0';
    // The 'to_route' in redirect_rule_t is NR_MAX_ROUTE_LEN.
    // We want to test truncation, so we provide a string longer than NR_REDIRECT_MAX_URL_LEN
    // but ensure we don't overflow the local buffer 'long_to_route'.
    // The actual truncation should happen within create_test_rule or nanorouter_process_redirect_request.
    memset(long_to_route, 'b', NR_MAX_ROUTE_LEN); // Fill up to the buffer's capacity
    long_to_route[NR_MAX_ROUTE_LEN] = '\0'; // Null-terminate the local buffer

    // When creating the rule, if 'to' is longer than NR_MAX_ROUTE_LEN, it should be truncated by create_test_rule.
    // The test itself should not cause a stack overflow.
    redirect_rule_t rule1 = create_test_rule(long_from_route, long_to_route, 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0}; // Empty context for tests without specific conditions
    bool result = nanorouter_process_redirect_request(long_from_route, list, &response, &context);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_LESS_OR_EQUAL(NR_REDIRECT_MAX_URL_LEN, strlen(response.new_url));
    TEST_ASSERT_EQUAL(301, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_null_request_url() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/test", "/redirect", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0};
    bool result = nanorouter_process_redirect_request(NULL, list, &response, &context);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("", response.new_url);
    TEST_ASSERT_EQUAL(0, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_null_rules() {
    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0};
    bool result = nanorouter_process_redirect_request("/test", NULL, &response, &context);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("", response.new_url);
    TEST_ASSERT_EQUAL(0, response.status_code);
}

void test_nanorouter_process_redirect_request_null_response_context() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/test", "/redirect", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_request_context_t context = {0};
    bool result = nanorouter_process_redirect_request("/test", list, NULL, &context);

    TEST_ASSERT_FALSE(result);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_empty_request_url() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/test", "/redirect", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0};
    bool result = nanorouter_process_redirect_request("", list, &response, &context);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("", response.new_url);
    TEST_ASSERT_EQUAL(0, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_placeholder_not_found() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/test/:missing", "/redirect/:found", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0};
    bool result = nanorouter_process_redirect_request("/test/value", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    // Should contain the placeholder as-is since :missing wasn't found
    TEST_ASSERT_NOT_NULL(strstr(response.new_url, ":found"));
    TEST_ASSERT_EQUAL(301, response.status_code);

    nanorouter_redirect_rule_list_free(list);
}

void test_nanorouter_process_redirect_request_to_route_with_query() {
    nanorouter_redirect_rule_list_t *list = nanorouter_redirect_rule_list_create();
    redirect_rule_t rule1 = create_test_rule("/test", "/redirect?newparam=value", 301, false);
    nanorouter_redirect_rule_list_add_rule(list, &rule1);

    nanorouter_redirect_response_t response;
    nanorouter_request_context_t context = {0};
    bool result = nanorouter_process_redirect_request("/test?oldparam=oldvalue", list, &response, &context);

    TEST_ASSERT_TRUE(result);
    // When to_route contains ?, original query should not be appended
    TEST_ASSERT_EQUAL_STRING("/redirect?newparam=value", response.new_url);
    TEST_ASSERT_NULL(strstr(response.new_url, "oldparam"));

    nanorouter_redirect_rule_list_free(list);
}


// --- Main Test Runner for this module ---
int test_nanorouter_redirect_middleware() {
    UNITY_BEGIN();

    // Rule List Create Tests
    RUN_TEST(test_nanorouter_redirect_rule_list_create_success);

    // Rule List Add Tests
    RUN_TEST(test_nanorouter_redirect_rule_list_add_rule_single_rule);
    RUN_TEST(test_nanorouter_redirect_rule_list_add_rule_multiple_rules);
    RUN_TEST(test_nanorouter_redirect_rule_list_add_rule_null_list);
    RUN_TEST(test_nanorouter_redirect_rule_list_add_rule_null_rule_data);

    // Rule List Free Tests
    RUN_TEST(test_nanorouter_redirect_rule_list_free_empty_list);
    RUN_TEST(test_nanorouter_redirect_rule_list_free_populated_list);
    RUN_TEST(test_nanorouter_redirect_rule_list_free_null_list);

    // Middleware Placeholder Tests (will be replaced by comprehensive tests)
    RUN_TEST(test_nanorouter_process_redirect_request_no_match);
    RUN_TEST(test_nanorouter_process_redirect_request_basic_match);

    // New Middleware Tests
    RUN_TEST(test_nanorouter_process_redirect_request_splat_match);
    RUN_TEST(test_nanorouter_process_redirect_request_placeholder_match);
    RUN_TEST(test_nanorouter_process_redirect_request_query_param_match);
    RUN_TEST(test_nanorouter_process_redirect_request_query_param_passthrough);
    RUN_TEST(test_nanorouter_process_redirect_request_rewrite_200);
    RUN_TEST(test_nanorouter_process_redirect_request_404_rule);
    RUN_TEST(test_nanorouter_process_redirect_request_force_redirect);
    RUN_TEST(test_nanorouter_process_redirect_request_multiple_rules_precedence);
    RUN_TEST(test_nanorouter_process_redirect_request_complex_splat_and_query);
    RUN_TEST(test_nanorouter_process_redirect_request_long_url_truncation);
    RUN_TEST(test_nanorouter_process_redirect_request_null_request_url);
    RUN_TEST(test_nanorouter_process_redirect_request_null_rules);
    RUN_TEST(test_nanorouter_process_redirect_request_null_response_context);
    RUN_TEST(test_nanorouter_process_redirect_request_empty_request_url);
    RUN_TEST(test_nanorouter_process_redirect_request_placeholder_not_found);
    RUN_TEST(test_nanorouter_process_redirect_request_to_route_with_query);

    return UNITY_END();
}

// This is typically called from app_main() or main() in the main test file
// void app_main() {
//     test_nanorouter_redirect_middleware();
// }
