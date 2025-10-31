#include "unity.h"
#include "nanorouter_condition_matching.h"
#include "nanorouter_redirect_rule_parser.h" // For nr_condition_item_t
#include <string.h>
#include <stdlib.h>

// Helper function to initialize a nanorouter_request_context_t
static nanorouter_request_context_t create_test_context(const char *domain, const char *country, const char *language) {
    nanorouter_request_context_t context = {0};
    if (domain) strncpy(context.domain, domain, NR_MAX_DOMAIN_LEN);
    if (country) strncpy(context.country, country, NR_MAX_COUNTRY_LEN);
    if (language) strncpy(context.language, language, NR_MAX_LANGUAGE_LEN);
    return context;
}

// Helper function to create a condition item
static nr_condition_item_t create_condition_item(const char *key, const char *value) {
    nr_condition_item_t item = {0};
    strncpy(item.key, key, NR_MAX_CONDITION_KEY_LEN);
    strncpy(item.value, value, NR_MAX_CONDITION_VALUE_LEN);
    item.is_present = true; // Always true for conditions from parser
    return item;
}

// --- Test Cases ---

void test_nanorouter_match_conditions_no_conditions(void) {
    nanorouter_request_context_t context = create_test_context("example.com", "us", "en");
    TEST_ASSERT_TRUE(nanorouter_match_conditions(NULL, 0, &context));
    TEST_ASSERT_TRUE(nanorouter_match_conditions(NULL, 0, NULL)); // No conditions, no context, should still be true
}

void test_nanorouter_match_conditions_null_context_with_conditions(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us")
    };
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, NULL));
}

void test_nanorouter_match_conditions_country_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us")
    };
    nanorouter_request_context_t context = create_test_context(NULL, "us", NULL);
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_country_no_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us")
    };
    nanorouter_request_context_t context = create_test_context(NULL, "gb", NULL);
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_country_multiple_rule_values_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "au,nz")
    };
    nanorouter_request_context_t context_au = create_test_context(NULL, "au", NULL);
    nanorouter_request_context_t context_nz = create_test_context(NULL, "nz", NULL);
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context_au));
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context_nz));
}

void test_nanorouter_match_conditions_country_multiple_rule_values_no_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "au,nz")
    };
    nanorouter_request_context_t context_us = create_test_context(NULL, "us", NULL);
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context_us));
}

void test_nanorouter_match_conditions_language_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en")
    };
    nanorouter_request_context_t context = create_test_context(NULL, NULL, "en-US,en;q=0.9");
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_language_no_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "fr")
    };
    nanorouter_request_context_t context = create_test_context(NULL, NULL, "en-US,en;q=0.9");
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_language_multiple_rule_values_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en,es")
    };
    nanorouter_request_context_t context_en = create_test_context(NULL, NULL, "en-GB");
    nanorouter_request_context_t context_es = create_test_context(NULL, NULL, "es-MX");
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context_en));
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context_es));
}

void test_nanorouter_match_conditions_domain_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Domain", "blog.example.com")
    };
    nanorouter_request_context_t context = create_test_context("blog.example.com", NULL, NULL);
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_domain_no_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Domain", "blog.example.com")
    };
    nanorouter_request_context_t context = create_test_context("www.example.com", NULL, NULL);
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_multiple_conditions_all_match(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us"),
        create_condition_item("Language", "en")
    };
    nanorouter_request_context_t context = create_test_context(NULL, "us", "en-GB");
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 2, &context));
}

void test_nanorouter_match_conditions_multiple_conditions_one_fails(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us"),
        create_condition_item("Language", "fr")
    };
    nanorouter_request_context_t context = create_test_context(NULL, "us", "en-GB");
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 2, &context));
}

void test_nanorouter_match_conditions_context_field_empty_rule_has_condition(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us")
    };
    nanorouter_request_context_t context = create_test_context(NULL, "", NULL); // Empty country in context
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_case_insensitivity(void) {
    nr_condition_item_t conditions[] = {
        create_condition_item("country", "US"), // Rule key and value case-insensitive
        create_condition_item("language", "EN"),
        create_condition_item("domain", "BLOG.EXAMPLE.COM")
    };
    nanorouter_request_context_t context = create_test_context("blog.example.com", "us", "en");
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 3, &context));
}

void test_nanorouter_condition_matching(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nanorouter_match_conditions_no_conditions);
    RUN_TEST(test_nanorouter_match_conditions_null_context_with_conditions);
    RUN_TEST(test_nanorouter_match_conditions_country_match);
    RUN_TEST(test_nanorouter_match_conditions_country_no_match);
    RUN_TEST(test_nanorouter_match_conditions_country_multiple_rule_values_match);
    RUN_TEST(test_nanorouter_match_conditions_country_multiple_rule_values_no_match);
    RUN_TEST(test_nanorouter_match_conditions_language_match);
    RUN_TEST(test_nanorouter_match_conditions_language_no_match);
    RUN_TEST(test_nanorouter_match_conditions_language_multiple_rule_values_match);
    RUN_TEST(test_nanorouter_match_conditions_domain_match);
    RUN_TEST(test_nanorouter_match_conditions_domain_no_match);
    RUN_TEST(test_nanorouter_match_conditions_multiple_conditions_all_match);
    RUN_TEST(test_nanorouter_match_conditions_multiple_conditions_one_fails);
    RUN_TEST(test_nanorouter_match_conditions_context_field_empty_rule_has_condition);
    RUN_TEST(test_nanorouter_match_conditions_case_insensitivity);
    UNITY_END();
}

// Not adding a main function here, as Unity tests are typically run via a test runner.
