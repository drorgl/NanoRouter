#include "unity.h"
#include "nanorouter_route_matcher.h"
#include "nanorouter_redirect_rule_parser.h" // For redirect_rule_t
#include "nanorouter_string_utils.h" // For string utilities if needed
#include <string.h>
#include <stdio.h> // For printf debugging, remove later

// Helper to create a simple redirect_rule_t for testing
static redirect_rule_t create_test_rule(const char *from_route, const char *to_route, uint16_t status_code, bool force) {
    redirect_rule_t rule;
    memset(&rule, 0, sizeof(redirect_rule_t));
    strncpy(rule.from_route, from_route, NR_MAX_ROUTE_LEN);
    rule.from_route[NR_MAX_ROUTE_LEN] = '\0';
    strncpy(rule.to_route, to_route, NR_MAX_ROUTE_LEN);
    rule.to_route[NR_MAX_ROUTE_LEN] = '\0';
    rule.status_code = status_code;
    rule.force = force;
    rule.num_query_params = 0;
    rule.num_conditions = 0;
    return rule;
}

// Helper to add a query parameter to a rule
static void add_rule_query_param(redirect_rule_t *rule, const char *key, const char *value, bool is_present) {
    if (rule->num_query_params < NR_MAX_QUERY_ITEMS) {
        nr_key_value_item_t *param = &rule->query_params[rule->num_query_params++];
        strncpy(param->key, key, NR_MAX_QUERY_KEY_LEN);
        param->key[NR_MAX_QUERY_KEY_LEN] = '\0';
        strncpy(param->value, value, NR_MAX_QUERY_VALUE_LEN);
        param->value[NR_MAX_QUERY_VALUE_LEN] = '\0';
        param->is_present = is_present;
    }
}

// Helper to find a matched parameter by key
static const char* find_matched_param(const nr_matched_params_t *matched_params, const char *key) {
    for (uint8_t i = 0; i < matched_params->num_params; ++i) {
        if (strcmp(matched_params->params[i].key, key) == 0) {
            return matched_params->params[i].value;
        }
    }
    return NULL;
}

// --- Path Matching Tests (nr_match_path_pattern) ---

void test_match_path_exact(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/foo/bar", "/foo/bar", &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

void test_match_path_wildcard_at_end(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/foo/bar/baz", "/foo/*", &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("bar/baz", find_matched_param(&matched_params, "*"));
}

void test_match_path_wildcard_in_middle(void) {
    nr_matched_params_t matched_params;
    // According to documentation, '*' can only be at the end of a path segment.
    // So, "/foo/*/baz" should not match "/foo/123/baz".
    TEST_ASSERT_FALSE(nr_match_path_pattern("/foo/123/baz", "/foo/*/baz", &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

void test_match_path_placeholder_single_segment(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/foo/123", "/foo/:id", &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("123", find_matched_param(&matched_params, "id"));
}

void test_match_path_placeholder_multiple_segments(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/foo/2023/10", "/foo/:year/:month", &matched_params));
    TEST_ASSERT_EQUAL(2, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("2023", find_matched_param(&matched_params, "year"));
    TEST_ASSERT_EQUAL_STRING("10", find_matched_param(&matched_params, "month"));
}

void test_match_path_splat_placeholder_named(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/blog/2023/10/my-post", "/blog/:splat", &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("2023/10/my-post", find_matched_param(&matched_params, "splat"));
}

void test_match_path_unnamed_splat_wildcard(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/docs/api/v1/users", "/docs/*", &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("api/v1/users", find_matched_param(&matched_params, "*"));
}

void test_match_path_no_match(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_FALSE(nr_match_path_pattern("/foo/bar", "/foo/baz", &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

void test_match_path_trailing_slash_normalization(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/foo/bar/", "/foo/bar", &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
    TEST_ASSERT_TRUE(nr_match_path_pattern("/foo/bar", "/foo/bar/", &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

void test_match_path_root_wildcard(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/any/path", "/*", &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("any/path", find_matched_param(&matched_params, "*"));
}

void test_match_path_root_exact(void) {
    nr_matched_params_t matched_params;
    TEST_ASSERT_TRUE(nr_match_path_pattern("/", "/", &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

// --- Query Parameter Matching Tests (nr_match_query_params) ---

void test_match_query_exact_match(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "123", false);
    TEST_ASSERT_TRUE(nr_match_query_params("id=123", rule.query_params, rule.num_query_params, &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

void test_match_query_placeholder_capture(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "", true); // is_present = true for placeholder
    TEST_ASSERT_TRUE(nr_match_query_params("id=456", rule.query_params, rule.num_query_params, &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("456", find_matched_param(&matched_params, "id"));
}

void test_match_query_multiple_params_exact(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "123", false);
    add_rule_query_param(&rule, "tag", "test", false);
    TEST_ASSERT_TRUE(nr_match_query_params("id=123&tag=test", rule.query_params, rule.num_query_params, &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

void test_match_query_multiple_params_mixed(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "", true);
    add_rule_query_param(&rule, "tag", "test", false);
    TEST_ASSERT_TRUE(nr_match_query_params("id=456&tag=test", rule.query_params, rule.num_query_params, &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("456", find_matched_param(&matched_params, "id"));
}

void test_match_query_no_match_value(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "123", false);
    TEST_ASSERT_FALSE(nr_match_query_params("id=456", rule.query_params, rule.num_query_params, &matched_params));
}

void test_match_query_no_match_key(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "123", false);
    TEST_ASSERT_FALSE(nr_match_query_params("uid=123", rule.query_params, rule.num_query_params, &matched_params));
}

void test_match_query_rule_has_param_url_does_not(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "123", false);
    TEST_ASSERT_FALSE(nr_match_query_params("", rule.query_params, rule.num_query_params, &matched_params));
}

void test_match_query_url_has_extra_params(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "123", false);
    TEST_ASSERT_TRUE(nr_match_query_params("id=123&extra=param", rule.query_params, rule.num_query_params, &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

// --- Combined nanorouter_match_rule Tests ---

void test_nanorouter_match_rule_path_only(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/foo/bar", "/newpath", 200, false);
    TEST_ASSERT_TRUE(nanorouter_match_rule(&rule, "/foo/bar", &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

void test_nanorouter_match_rule_path_and_exact_query(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "123", false);
    TEST_ASSERT_TRUE(nanorouter_match_rule(&rule, "/path?id=123", &matched_params));
    TEST_ASSERT_EQUAL(0, matched_params.num_params);
}

void test_nanorouter_match_rule_path_and_placeholder_query(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "", true);
    TEST_ASSERT_TRUE(nanorouter_match_rule(&rule, "/path?id=456", &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("456", find_matched_param(&matched_params, "id"));
}

void test_nanorouter_match_rule_path_with_splat_and_query(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/docs/*", "/newpath", 200, false);
    add_rule_query_param(&rule, "version", "v1", false);
    TEST_ASSERT_TRUE(nanorouter_match_rule(&rule, "/docs/api/users?version=v1", &matched_params));
    TEST_ASSERT_EQUAL(1, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("api/users", find_matched_param(&matched_params, "*"));
}

void test_nanorouter_match_rule_full_url_match_and_capture(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/blog/:year/:month/:splat", "/newpath", 200, false);
    add_rule_query_param(&rule, "author", "", true);
    add_rule_query_param(&rule, "category", "tech", false);
    TEST_ASSERT_TRUE(nanorouter_match_rule(&rule, "/blog/2023/10/my-post?author=john&category=tech", &matched_params));
    // Expected 4 params: year, month, splat (from path), and author (from query)
    TEST_ASSERT_EQUAL(4, matched_params.num_params);
    TEST_ASSERT_EQUAL_STRING("2023", find_matched_param(&matched_params, "year"));
    TEST_ASSERT_EQUAL_STRING("10", find_matched_param(&matched_params, "month"));
    TEST_ASSERT_EQUAL_STRING("my-post", find_matched_param(&matched_params, "splat"));
    TEST_ASSERT_EQUAL_STRING("john", find_matched_param(&matched_params, "author"));
}

void test_nanorouter_match_rule_no_match_path(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/foo/bar", "/newpath", 200, false);
    TEST_ASSERT_FALSE(nanorouter_match_rule(&rule, "/foo/baz", &matched_params));
}

void test_nanorouter_match_rule_no_match_query(void) {
    nr_matched_params_t matched_params;
    redirect_rule_t rule = create_test_rule("/path", "/newpath", 200, false);
    add_rule_query_param(&rule, "id", "123", false);
    TEST_ASSERT_FALSE(nanorouter_match_rule(&rule, "/path?id=456", &matched_params));
}

// --- Test Runner ---
void test_matcher(void) {
    UNITY_BEGIN();

    // Path Matching Tests
    RUN_TEST(test_match_path_exact);
    RUN_TEST(test_match_path_wildcard_at_end);
    RUN_TEST(test_match_path_wildcard_in_middle);
    RUN_TEST(test_match_path_placeholder_single_segment);
    RUN_TEST(test_match_path_placeholder_multiple_segments);
    RUN_TEST(test_match_path_splat_placeholder_named);
    RUN_TEST(test_match_path_unnamed_splat_wildcard);
    RUN_TEST(test_match_path_no_match);
    RUN_TEST(test_match_path_trailing_slash_normalization);
    RUN_TEST(test_match_path_root_wildcard);
    RUN_TEST(test_match_path_root_exact);

    // Query Parameter Matching Tests
    RUN_TEST(test_match_query_exact_match);
    RUN_TEST(test_match_query_placeholder_capture);
    RUN_TEST(test_match_query_multiple_params_exact);
    RUN_TEST(test_match_query_multiple_params_mixed);
    RUN_TEST(test_match_query_no_match_value);
    RUN_TEST(test_match_query_no_match_key);
    RUN_TEST(test_match_query_rule_has_param_url_does_not);
    RUN_TEST(test_match_query_url_has_extra_params);

    // Combined nanorouter_match_rule Tests
    RUN_TEST(test_nanorouter_match_rule_path_only);
    RUN_TEST(test_nanorouter_match_rule_path_and_exact_query);
    RUN_TEST(test_nanorouter_match_rule_path_and_placeholder_query);
    RUN_TEST(test_nanorouter_match_rule_path_with_splat_and_query);
    RUN_TEST(test_nanorouter_match_rule_full_url_match_and_capture);
    RUN_TEST(test_nanorouter_match_rule_no_match_path);
    RUN_TEST(test_nanorouter_match_rule_no_match_query);

    UNITY_END();
}
