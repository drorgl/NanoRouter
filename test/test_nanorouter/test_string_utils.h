#ifndef TEST_STRING_UTILS_H
#define TEST_STRING_UTILS_H

#include "unity.h"
#include "nanorouter_string_utils.h" // Assuming this header is needed for nr_trim_string and nr_string_split
#include <string.h> // For strlen
#include <stdlib.h> // For free
#include <stdbool.h> // For bool type

// Helper struct for individual token data in split tests
typedef struct {
    const char *expected_token;
    size_t expected_len;
    size_t expected_index;
} split_token_data_t;

// Combined user data for string split tests
typedef struct {
    split_token_data_t *expected_tokens;
    size_t expected_num_tokens;
    size_t actual_call_count;
} split_user_data_t;

// Callback function for string split tests
void split_callback(const char *token, size_t token_len, size_t token_index, void *user_data) {
    split_user_data_t *data = (split_user_data_t *)user_data;

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_MESSAGE(data->expected_num_tokens, token_index, "Token index out of bounds");

    TEST_ASSERT_EQUAL_STRING_LEN(data->expected_tokens[token_index].expected_token, token, token_len);
    TEST_ASSERT_EQUAL(data->expected_tokens[token_index].expected_len, token_len);
    TEST_ASSERT_EQUAL(data->expected_tokens[token_index].expected_index, token_index);

    data->actual_call_count++;
}

void test_nr_trim_string_no_extra_spaces(void) {
    // Arrange
    char str[] = "a b c d";
    size_t len = strlen(str);
    const char *expected = "a b c d";

    // Act
    nr_trim_string(str, len);

    // Assert
    TEST_ASSERT_EQUAL_STRING(expected, str);
}

void test_nr_trim_string_middle_spaces(void) {
    // Arrange
    char str[] = "aa   bbb    cccc     ddddd";
    size_t len = strlen(str);
    const char *expected = "aa bbb cccc ddddd";

    // Act
    nr_trim_string(str, len);

    // Assert
    TEST_ASSERT_EQUAL_STRING(expected, str);
}

void test_nr_trim_string_beginning_and_middle_spaces(void) {
    // Arrange
    char str[] = "   aa bb cc dd";
    size_t len = strlen(str);
    const char *expected = "aa bb cc dd";

    // Act
    nr_trim_string(str, len);

    // Assert
    TEST_ASSERT_EQUAL_STRING(expected, str);
}

void test_nr_trim_string_middle_and_end_spaces(void) {
    // Arrange
    char str[] = "aa bb cc dd       ";
    size_t len = strlen(str);
    const char *expected = "aa bb cc dd";

    // Act
    nr_trim_string(str, len);

    // Assert
    TEST_ASSERT_EQUAL_STRING(expected, str);
}

void test_nr_trim_string_beginning_middle_and_end_spaces(void) {
    // Arrange
    char str[] = "   aa bb cc dd       ";
    size_t len = strlen(str);
    const char *expected = "aa bb cc dd";

    // Act
    nr_trim_string(str, len);

    // Assert
    TEST_ASSERT_EQUAL_STRING(expected, str);
}

void test_nr_trim_string_with_tabs(void) {
    // Arrange
    char str[] = "aa\tbbb  \tcccc    \t    ddddd     ";
    size_t len = strlen(str);
    const char *expected = "aa bbb cccc ddddd";

    // Act
    nr_trim_string(str, len);

    // Assert
    TEST_ASSERT_EQUAL_STRING(expected, str);
}

void test_nr_string_split_basic_spaces(void) {
    // Arrange
    const char *str = "aa bb cc";
    size_t str_len = strlen(str);
    const char *delimiter = " ";
    split_token_data_t expected_tokens[] = {
        {"aa", 2, 0},
        {"bb", 2, 1},
        {"cc", 2, 2}
    };
    split_user_data_t user_data = {
        .expected_tokens = expected_tokens,
        .expected_num_tokens = sizeof(expected_tokens) / sizeof(expected_tokens[0]),
        .actual_call_count = 0
    };

    // Act
    nr_string_split(str, str_len, delimiter, split_callback, &user_data);

    // Assert
    TEST_ASSERT_EQUAL(user_data.expected_num_tokens, user_data.actual_call_count);
}

void test_nr_string_split_multiple_delimiters(void) {
    // Arrange
    const char *str = "aa   bb    cc";
    size_t str_len = strlen(str);
    const char *delimiter = " ";
    split_token_data_t expected_tokens[] = {
        {"aa", 2, 0},
        {"bb", 2, 1},
        {"cc", 2, 2}
    };
    split_user_data_t user_data = {
        .expected_tokens = expected_tokens,
        .expected_num_tokens = sizeof(expected_tokens) / sizeof(expected_tokens[0]),
        .actual_call_count = 0
    };

    // Act
    nr_string_split(str, str_len, delimiter, split_callback, &user_data);

    // Assert
    TEST_ASSERT_EQUAL(user_data.expected_num_tokens, user_data.actual_call_count);
}

void test_nr_string_split_leading_trailing_delimiters(void) {
    // Arrange
    const char *str = "   aa bb cc   ";
    size_t str_len = strlen(str);
    const char *delimiter = " ";
    split_token_data_t expected_tokens[] = {
        {"aa", 2, 0},
        {"bb", 2, 1},
        {"cc", 2, 2}
    };
    split_user_data_t user_data = {
        .expected_tokens = expected_tokens,
        .expected_num_tokens = sizeof(expected_tokens) / sizeof(expected_tokens[0]),
        .actual_call_count = 0
    };

    // Act
    nr_string_split(str, str_len, delimiter, split_callback, &user_data);

    // Assert
    TEST_ASSERT_EQUAL(user_data.expected_num_tokens, user_data.actual_call_count);
}

void test_nr_string_split_empty_string(void) {
    // Arrange
    const char *str = "";
    size_t str_len = strlen(str);
    const char *delimiter = " ";
    split_token_data_t expected_tokens[] = {}; // No expected tokens
    split_user_data_t user_data = {
        .expected_tokens = expected_tokens,
        .expected_num_tokens = 0,
        .actual_call_count = 0
    };

    // Act
    nr_string_split(str, str_len, delimiter, split_callback, &user_data);

    // Assert
    TEST_ASSERT_EQUAL(user_data.expected_num_tokens, user_data.actual_call_count);
}

void test_nr_string_split_no_delimiter(void) {
    // Arrange
    const char *str = "singletoken";
    size_t str_len = strlen(str);
    const char *delimiter = " ";
    split_token_data_t expected_tokens[] = {
        {"singletoken", 11, 0}
    };
    split_user_data_t user_data = {
        .expected_tokens = expected_tokens,
        .expected_num_tokens = sizeof(expected_tokens) / sizeof(expected_tokens[0]),
        .actual_call_count = 0
    };

    // Act
    nr_string_split(str, str_len, delimiter, split_callback, &user_data);

    // Assert
    TEST_ASSERT_EQUAL(user_data.expected_num_tokens, user_data.actual_call_count);
}

void test_nr_string_split_different_delimiter(void) {
    // Arrange
    const char *str = "one,two,three";
    size_t str_len = strlen(str);
    const char *delimiter = ",";
    split_token_data_t expected_tokens[] = {
        {"one", 3, 0},
        {"two", 3, 1},
        {"three", 5, 2}
    };
    split_user_data_t user_data = {
        .expected_tokens = expected_tokens,
        .expected_num_tokens = sizeof(expected_tokens) / sizeof(expected_tokens[0]),
        .actual_call_count = 0
    };

    // Act
    nr_string_split(str, str_len, delimiter, split_callback, &user_data);

    // Assert
    TEST_ASSERT_EQUAL(user_data.expected_num_tokens, user_data.actual_call_count);
}

void test_nr_string_split_with_str_len_limit(void) {
    // Arrange
    const char *str = "aa bb cc dd";
    size_t str_len = 5; // Only "aa bb" should be processed
    const char *delimiter = " ";
    split_token_data_t expected_tokens[] = {
        {"aa", 2, 0},
        {"bb", 2, 1}
    };
    split_user_data_t user_data = {
        .expected_tokens = expected_tokens,
        .expected_num_tokens = sizeof(expected_tokens) / sizeof(expected_tokens[0]),
        .actual_call_count = 0
    };

    // Act
    nr_string_split(str, str_len, delimiter, split_callback, &user_data);

    // Assert
    TEST_ASSERT_EQUAL(user_data.expected_num_tokens, user_data.actual_call_count);
}

void test_string_utils(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nr_trim_string_no_extra_spaces);
    RUN_TEST(test_nr_trim_string_middle_spaces);
    RUN_TEST(test_nr_trim_string_beginning_and_middle_spaces);
    RUN_TEST(test_nr_trim_string_middle_and_end_spaces);
    RUN_TEST(test_nr_trim_string_beginning_middle_and_end_spaces);
    RUN_TEST(test_nr_trim_string_with_tabs);

    RUN_TEST(test_nr_string_split_basic_spaces);
    RUN_TEST(test_nr_string_split_multiple_delimiters);
    RUN_TEST(test_nr_string_split_leading_trailing_delimiters);
    RUN_TEST(test_nr_string_split_empty_string);
    RUN_TEST(test_nr_string_split_no_delimiter);
    RUN_TEST(test_nr_string_split_different_delimiter);
    RUN_TEST(test_nr_string_split_with_str_len_limit);
    UNITY_END();
}

#endif // TEST_STRING_UTILS_H
