#include "test_string_utils_edge_cases.h"
#include "unity.h"
#include "nanorouter_string_utils.h"
#include <string.h>
#include <stdlib.h>

// Test counter for callback verification
static int callback_call_count;
static char callback_tokens[10][100];
static size_t callback_token_lengths[10];

// Callback function for testing nr_string_split
static void test_split_callback(const char *token, size_t token_len, size_t token_index, void *user_data) {
    if (token_index < 10) {
        strncpy(callback_tokens[token_index], token, token_len);
        callback_tokens[token_index][token_len] = '\0';
        callback_token_lengths[token_index] = token_len;
        callback_call_count++;
    }
}

// Test edge cases for nr_trim_string
void test_nanorouter_trim_string_null_input() {
    nr_trim_string(NULL, 10);
    // Should not crash
    TEST_PASS();
}

void test_nanorouter_trim_string_zero_length() {
    char str[] = "test";
    nr_trim_string(str, 0);
    TEST_ASSERT_EQUAL_STRING("test", str); // Should remain unchanged
}

void test_nanorouter_trim_string_all_spaces() {
    char str[] = "     ";
    nr_trim_string(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("", str);
}

void test_nanorouter_trim_string_leading_spaces() {
    char str[] = "   hello world";
    nr_trim_string(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("hello world", str);
}

void test_nanorouter_trim_string_trailing_spaces() {
    char str[] = "hello world   ";
    nr_trim_string(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("hello world", str);
}

void test_nanorouter_trim_string_multiple_spaces() {
    char str[] = "hello    world";
    nr_trim_string(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("hello world", str);
}

void test_nanorouter_trim_string_tabs_and_spaces() {
    char str[] = "\t  hello \t world  \t";
    nr_trim_string(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("hello world", str);
}

// Test edge cases for nr_trim_whitespace
void test_nanorouter_trim_whitespace_null_input() {
    char *result = nr_trim_whitespace(NULL);
    TEST_ASSERT_NULL(result);
}

void test_nanorouter_trim_whitespace_empty_string() {
    char str[] = "";
    char *result = nr_trim_whitespace(str);
    TEST_ASSERT_EQUAL_STRING("", result);
    TEST_ASSERT_EQUAL_PTR(str, result); // Should return same pointer
}

void test_nanorouter_trim_whitespace_all_spaces() {
    char str[] = "     ";
    char *result = nr_trim_whitespace(str);
    TEST_ASSERT_EQUAL_STRING("", result);
}

void test_nanorouter_trim_whitespace_leading_spaces() {
    char str[] = "   hello world";
    char *result = nr_trim_whitespace(str);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    TEST_ASSERT_EQUAL_PTR(str + 3, result); // Should point to first non-space
}

void test_nanorouter_trim_whitespace_trailing_spaces() {
    char str[] = "hello world   ";
    char *result = nr_trim_whitespace(str);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    TEST_ASSERT_EQUAL_PTR(str, result); // Should return original pointer
}

void test_nanorouter_trim_whitespace_both_ends() {
    char str[] = "  hello world  ";
    char *result = nr_trim_whitespace(str);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    TEST_ASSERT_EQUAL_PTR(str + 2, result); // Should point to first non-space
}

void test_nanorouter_trim_whitespace_tabs() {
    char str[] = "\t\thello world\t\t";
    char *result = nr_trim_whitespace(str);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
}

void test_nanorouter_trim_whitespace_mixed_whitespace() {
    char str[] = " \t hello world \n\r\t ";
    char *result = nr_trim_whitespace(str);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
}

// Test edge cases for nr_string_split
void test_nanorouter_string_split_null_string() {
    callback_call_count = 0;
    nr_string_split(NULL, 10, ",", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(0, callback_call_count);
}

void test_nanorouter_string_split_zero_length() {
    callback_call_count = 0;
    nr_string_split("test", 0, ",", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(0, callback_call_count);
}

void test_nanorouter_string_split_null_delimiter() {
    callback_call_count = 0;
    nr_string_split("test", 4, NULL, test_split_callback, NULL);
    TEST_ASSERT_EQUAL(0, callback_call_count);
}

void test_nanorouter_string_split_null_callback() {
    callback_call_count = 0;
    nr_string_split("test", 4, ",", NULL, NULL);
    TEST_ASSERT_EQUAL(0, callback_call_count);
}

void test_nanorouter_string_split_empty_string() {
    callback_call_count = 0;
    nr_string_split("", 0, ",", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(0, callback_call_count);
}

void test_nanorouter_string_split_no_delimiters() {
    callback_call_count = 0;
    nr_string_split("hello world", 11, ",", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(1, callback_call_count);
    TEST_ASSERT_EQUAL_STRING("hello world", callback_tokens[0]);
    TEST_ASSERT_EQUAL(11, callback_token_lengths[0]);
}

void test_nanorouter_string_split_leading_delimiters() {
    callback_call_count = 0;
    nr_string_split(",,hello,world", 13, ",", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(2, callback_call_count); // Should skip leading delimiters
    TEST_ASSERT_EQUAL_STRING("hello", callback_tokens[0]);
    TEST_ASSERT_EQUAL_STRING("world", callback_tokens[1]);
}

void test_nanorouter_string_split_trailing_delimiters() {
    callback_call_count = 0;
    nr_string_split("hello,world,,", 12, ",", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(2, callback_call_count);
    TEST_ASSERT_EQUAL_STRING("hello", callback_tokens[0]);
    TEST_ASSERT_EQUAL_STRING("world", callback_tokens[1]);
}

void test_nanorouter_string_split_multiple_consecutive_delimiters() {
    callback_call_count = 0;
    nr_string_split("hello,,,world", strlen("hello,,,world"), ",", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(2, callback_call_count); // Should skip empty tokens
    TEST_ASSERT_EQUAL_STRING("hello", callback_tokens[0]);
    TEST_ASSERT_EQUAL_STRING("world", callback_tokens[1]);
}

void test_nanorouter_string_split_empty_tokens() {
    callback_call_count = 0;
    nr_string_split(",hello,,world,", 13, ",", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(2, callback_call_count); // Should skip empty tokens
    TEST_ASSERT_EQUAL_STRING("hello", callback_tokens[0]);
    TEST_ASSERT_EQUAL_STRING("world", callback_tokens[1]);
}

void test_nanorouter_string_split_multi_char_delimiter() {
    callback_call_count = 0;
    nr_string_split("hello;;world;;test", strlen("hello;;world;;test"), ";;", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(3, callback_call_count);
    TEST_ASSERT_EQUAL_STRING("hello", callback_tokens[0]);
    TEST_ASSERT_EQUAL_STRING("world", callback_tokens[1]);
    TEST_ASSERT_EQUAL_STRING("test", callback_tokens[2]);
}

void test_nanorouter_string_split_partial_length() {
    callback_call_count = 0;
    nr_string_split("hello,world,test", 8, ",", test_split_callback, NULL);
    // Should only process first 8 characters: "hello,wo"
    TEST_ASSERT_EQUAL(2, callback_call_count);
    TEST_ASSERT_EQUAL_STRING("hello", callback_tokens[0]);
    TEST_ASSERT_EQUAL_STRING("wo", callback_tokens[1]);
}

void test_nanorouter_string_split_whitespace_delimiter() {
    callback_call_count = 0;
    nr_string_split("hello world test", 17, " ", test_split_callback, NULL);
    TEST_ASSERT_EQUAL(3, callback_call_count);
    TEST_ASSERT_EQUAL_STRING("hello", callback_tokens[0]);
    TEST_ASSERT_EQUAL_STRING("world", callback_tokens[1]);
    TEST_ASSERT_EQUAL_STRING("test", callback_tokens[2]);
}

// Main test runner
int test_string_utils_edge_cases() {
    UNITY_BEGIN();
    
    // nr_trim_string tests
    RUN_TEST(test_nanorouter_trim_string_null_input);
    RUN_TEST(test_nanorouter_trim_string_zero_length);
    RUN_TEST(test_nanorouter_trim_string_all_spaces);
    RUN_TEST(test_nanorouter_trim_string_leading_spaces);
    RUN_TEST(test_nanorouter_trim_string_trailing_spaces);
    RUN_TEST(test_nanorouter_trim_string_multiple_spaces);
    RUN_TEST(test_nanorouter_trim_string_tabs_and_spaces);
    
    // nr_trim_whitespace tests
    RUN_TEST(test_nanorouter_trim_whitespace_null_input);
    RUN_TEST(test_nanorouter_trim_whitespace_empty_string);
    RUN_TEST(test_nanorouter_trim_whitespace_all_spaces);
    RUN_TEST(test_nanorouter_trim_whitespace_leading_spaces);
    RUN_TEST(test_nanorouter_trim_whitespace_trailing_spaces);
    RUN_TEST(test_nanorouter_trim_whitespace_both_ends);
    RUN_TEST(test_nanorouter_trim_whitespace_tabs);
    RUN_TEST(test_nanorouter_trim_whitespace_mixed_whitespace);
    
    // nr_string_split tests
    RUN_TEST(test_nanorouter_string_split_null_string);
    RUN_TEST(test_nanorouter_string_split_zero_length);
    RUN_TEST(test_nanorouter_string_split_null_delimiter);
    RUN_TEST(test_nanorouter_string_split_null_callback);
    RUN_TEST(test_nanorouter_string_split_empty_string);
    RUN_TEST(test_nanorouter_string_split_no_delimiters);
    RUN_TEST(test_nanorouter_string_split_leading_delimiters);
    RUN_TEST(test_nanorouter_string_split_trailing_delimiters);
    RUN_TEST(test_nanorouter_string_split_multiple_consecutive_delimiters);
    RUN_TEST(test_nanorouter_string_split_empty_tokens);
    RUN_TEST(test_nanorouter_string_split_multi_char_delimiter);
    RUN_TEST(test_nanorouter_string_split_partial_length);
    RUN_TEST(test_nanorouter_string_split_whitespace_delimiter);
    
    return UNITY_END();
}
