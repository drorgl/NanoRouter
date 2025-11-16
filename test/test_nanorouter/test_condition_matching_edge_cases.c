#include "test_condition_matching_edge_cases.h"

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

// Test edge cases for condition matching
void test_nanorouter_match_conditions_unknown_condition_key() {
    nr_condition_item_t conditions[] = {
        create_condition_item("UnknownCondition", "some_value")
    };
    nanorouter_request_context_t context = create_test_context("example.com", "us", "en");
    
    // Unknown condition key should result in false
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_empty_condition_value() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "")
    };
    nanorouter_request_context_t context = create_test_context(NULL, "us", NULL);
    
    // Empty condition value should not match
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_empty_context_country() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us")
    };
    nanorouter_request_context_t context = create_test_context(NULL, "", NULL);
    
    // Empty context country should not match
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_empty_context_language() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en")
    };
    nanorouter_request_context_t context = create_test_context(NULL, NULL, "");
    
    // Empty context language should not match
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_empty_context_domain() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Domain", "example.com")
    };
    nanorouter_request_context_t context = create_test_context("", NULL, NULL);
    
    // Empty context domain should not match
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_null_condition_key() {
    nr_condition_item_t conditions[] = {
        create_condition_item("", "value")
    };
    nanorouter_request_context_t context = create_test_context("example.com", "us", "en");
    
    // Empty condition key should not match
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_condition_with_null_terminator() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us")
    };
    nanorouter_request_context_t context = create_test_context(NULL, "us", NULL);
    
    // Normal case should still work
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_multiple_conditions_first_fails() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "gb"), // Will fail
        create_condition_item("Language", "en") // Should not be checked
    };
    nanorouter_request_context_t context = create_test_context(NULL, "us", "en");
    
    // First condition fails, should return false immediately
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 2, &context));
}

void test_nanorouter_match_conditions_language_empty_extracted_tags() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en")
    };
    nanorouter_request_context_t context = create_test_context(NULL, NULL, ""); // Empty language header
    
    // Empty language header should not match
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_language_malformed_header() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en")
    };
    // Malformed language header with no actual language codes
    nanorouter_request_context_t context = create_test_context(NULL, NULL, ",,,");
    
    // Should not match when no valid language tags are extracted
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_language_q_values_only() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en")
    };
    // Language header with only q-values, no actual language codes
    nanorouter_request_context_t context = create_test_context(NULL, NULL, ";q=0.9,;q=0.8");
    
    // Should not match when no valid language tags are extracted
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_case_variations() {
    nr_condition_item_t conditions[] = {
        create_condition_item("CoUnTrY", "US"),
        create_condition_item("LaNgUaGe", "EN"),
        create_condition_item("DoMaIn", "EXAMPLE.COM")
    };
    nanorouter_request_context_t context = create_test_context("example.com", "us", "en");
    
    // Case-insensitive matching should work
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 3, &context));
}

void test_nanorouter_match_conditions_country_list_with_spaces() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Country", "us, gb, au") // Spaces in the list
    };
    nanorouter_request_context_t context_us = create_test_context(NULL, "us", NULL);
    nanorouter_request_context_t context_gb = create_test_context(NULL, "gb", NULL);
    nanorouter_request_context_t context_au = create_test_context(NULL, "au", NULL);
    
    // Should match despite spaces in the rule
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context_us));
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context_gb));
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context_au));
}

void test_nanorouter_match_conditions_language_partial_match() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en") // Just "en"
    };
    nanorouter_request_context_t context = create_test_context(NULL, NULL, "en-US,en;q=0.9,fr;q=0.8");
    
    // Should match "en" from "en-US" prefix
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context));
}

void test_nanorouter_match_conditions_language_exact_match_only() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en-US") // Exact "en-US"
    };
    nanorouter_request_context_t context1 = create_test_context(NULL, NULL, "en-US,en;q=0.9");
    nanorouter_request_context_t context2 = create_test_context(NULL, NULL, "en,en-US;q=0.9");
    
    // Should match exact "en-US"
    TEST_ASSERT_EQUAL_STRING("en-US,en;q=0.9", context1.language);
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context1));
    TEST_ASSERT_TRUE(nanorouter_match_conditions(conditions, 1, &context2));
}

void test_nanorouter_match_conditions_language_no_match_similar() {
    nr_condition_item_t conditions[] = {
        create_condition_item("Language", "en") // Just "en"
    };
    nanorouter_request_context_t context = create_test_context(NULL, NULL, "fr,de,es");
    
    // Should not match when languages are completely different
    TEST_ASSERT_FALSE(nanorouter_match_conditions(conditions, 1, &context));
}

// Main test runner
int test_condition_matching_edge_cases() {
    UNITY_BEGIN();
    
    RUN_TEST(test_nanorouter_match_conditions_unknown_condition_key);
    RUN_TEST(test_nanorouter_match_conditions_empty_condition_value);
    RUN_TEST(test_nanorouter_match_conditions_empty_context_country);
    RUN_TEST(test_nanorouter_match_conditions_empty_context_language);
    RUN_TEST(test_nanorouter_match_conditions_empty_context_domain);
    RUN_TEST(test_nanorouter_match_conditions_null_condition_key);
    RUN_TEST(test_nanorouter_match_conditions_condition_with_null_terminator);
    RUN_TEST(test_nanorouter_match_conditions_multiple_conditions_first_fails);
    RUN_TEST(test_nanorouter_match_conditions_language_empty_extracted_tags);
    RUN_TEST(test_nanorouter_match_conditions_language_malformed_header);
    RUN_TEST(test_nanorouter_match_conditions_language_q_values_only);
    RUN_TEST(test_nanorouter_match_conditions_case_variations);
    RUN_TEST(test_nanorouter_match_conditions_country_list_with_spaces);
    RUN_TEST(test_nanorouter_match_conditions_language_partial_match);
    RUN_TEST(test_nanorouter_match_conditions_language_exact_match_only);
    RUN_TEST(test_nanorouter_match_conditions_language_no_match_similar);
    
    return UNITY_END();
}
