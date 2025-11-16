#include "nanorouter_string_utils.h"
#include <stdbool.h>
#include <string.h> // For strlen, strncmp
#include <ctype.h>  // For isspace
#include "nanorouter_header_rule_parser.h" // For NR_MAX_HEADER_VALUE_LEN

void nr_trim_string(char *str, size_t len) {
    if (str == NULL || len == 0) {
        return;
    }

    size_t read_idx = 0;
    size_t write_idx = 0;
    bool space_encountered = false;

    // Trim leading spaces
    while (read_idx < len && isspace((unsigned char)str[read_idx])) {
        read_idx++;
    }

    for (; read_idx < len; read_idx++) {
        if (!isspace((unsigned char)str[read_idx])) {
            if (space_encountered) {
                str[write_idx++] = ' '; // Add a single space if one was encountered
                space_encountered = false;
            }
            str[write_idx++] = str[read_idx];
        } else {
            space_encountered = true;
        }
    }

    // Null-terminate the string
    str[write_idx] = '\0';
}

char* nr_trim_whitespace(char *str) {
    if (str == NULL) {
        return NULL;
    }

    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == 0) { // All spaces or empty string
        return str;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    // Write new null terminator
    *(end + 1) = '\0';

    return str;
}

void nr_string_split(const char *str, size_t str_len, const char *delimiter, nr_string_split_callback_t callback, void *user_data) {
    if (str == NULL || str_len == 0 || delimiter == NULL || callback == NULL) {
        return;
    }

    size_t current_pos = 0;
    size_t token_index = 0;
    size_t delimiter_len = strlen(delimiter);

    while (current_pos < str_len) {
        // Skip leading delimiters
        bool is_delimiter_found = false;
        if (current_pos + delimiter_len <= str_len && strncmp(&str[current_pos], delimiter, delimiter_len) == 0) {
            is_delimiter_found = true;
        }

        while (current_pos < str_len && is_delimiter_found) {
            current_pos += delimiter_len;
            is_delimiter_found = false; // Reset for next check
            if (current_pos + delimiter_len <= str_len && strncmp(&str[current_pos], delimiter, delimiter_len) == 0) {
                is_delimiter_found = true;
            }
        }

        if (current_pos >= str_len) {
            break; // Reached end of string after skipping delimiters
        }

        // Find the end of the token
        size_t token_start = current_pos;
        size_t token_end = current_pos;

        while (token_end < str_len) {
            if (token_end + delimiter_len <= str_len && strncmp(&str[token_end], delimiter, delimiter_len) == 0) {
                break; // Found a delimiter
            }
            token_end++;
        }

        // Call the callback with the token
        if (token_end > token_start) {
            callback(&str[token_start], token_end - token_start, token_index++, user_data);
        }

        if (token_end < str_len) {
            current_pos = token_end + delimiter_len;
        } else {
            current_pos = token_end;
        }
    }
}
