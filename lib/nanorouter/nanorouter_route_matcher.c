#include "nanorouter_route_matcher.h"
#include "nanorouter_string_utils.h"
#include <string.h>
// #include <stdio.h> // Removed: For debugging, remove later

// Helper to add a matched parameter
static bool add_matched_param(nr_matched_params_t *matched_params, const char *key, size_t key_len, const char *value, size_t value_len) {
    if (matched_params->num_params >= NR_MAX_MATCHED_PARAMS) {
        return false; // No space left
    }
    nr_matched_param_t *param = &matched_params->params[matched_params->num_params++];
    strncpy(param->key, key, key_len < NR_MAX_MATCHED_KEY_LEN ? key_len : NR_MAX_MATCHED_KEY_LEN);
    param->key[key_len < NR_MAX_MATCHED_KEY_LEN ? key_len : NR_MAX_MATCHED_KEY_LEN] = '\0';
    strncpy(param->value, value, value_len < NR_MAX_MATCHED_VALUE_LEN ? value_len : NR_MAX_MATCHED_VALUE_LEN);
    param->value[value_len < NR_MAX_MATCHED_VALUE_LEN ? value_len : NR_MAX_MATCHED_VALUE_LEN] = '\0';
    return true;
}

// Helper to parse URL into path and query string
static void nr_parse_url_path_and_query(const char *url, char *path_buffer, size_t path_buffer_len, char *query_buffer, size_t query_buffer_len) {
    const char *query_start = strchr(url, '?');
    if (query_start) {
        size_t path_len = query_start - url;
        strncpy(path_buffer, url, path_len < path_buffer_len ? path_len : path_buffer_len - 1);
        path_buffer[path_len < path_buffer_len ? path_len : path_buffer_len - 1] = '\0';

        size_t query_len = strlen(query_start + 1);
        strncpy(query_buffer, query_start + 1, query_len < query_buffer_len ? query_len : query_buffer_len - 1);
        query_buffer[query_len < query_buffer_len ? query_len : query_buffer_len - 1] = '\0';
    } else {
        strncpy(path_buffer, url, path_buffer_len - 1);
        path_buffer[path_buffer_len - 1] = '\0';
        query_buffer[0] = '\0';
    }

    // Normalize path by removing trailing slash if not root
    size_t path_len = strlen(path_buffer);
    if (path_len > 1 && path_buffer[path_len - 1] == '/') {
        path_buffer[path_len - 1] = '\0';
    }
}

bool nr_match_path_pattern(const char *url_path, const char *from_route_pattern, nr_matched_params_t *matched_params) {
    matched_params->num_params = 0;

    const char *url_curr = url_path;
    const char *pattern_curr = from_route_pattern;

    // Handle root path special case
    if (strcmp(url_path, "/") == 0 && strcmp(from_route_pattern, "/") == 0) {
        return true;
    }
    // Handle root wildcard special case (e.g., "/*" matching "/any/path")
    if (strcmp(from_route_pattern, "/*") == 0) {
        if (strlen(url_path) > 1) { // If URL is not just "/"
            add_matched_param(matched_params, "*", 1, url_path + 1, strlen(url_path) - 1);
        } else { // If URL is just "/"
            add_matched_param(matched_params, "*", 1, "", 0);
        }
        return true;
    }

    // Skip leading '/' for easier segment processing
    if (*url_curr == '/') url_curr++;
    if (*pattern_curr == '/') pattern_curr++;

    while (*url_curr != '\0' && *pattern_curr != '\0') {
        if (*pattern_curr == ':') { // Placeholder or named splat
            const char *placeholder_name_start = pattern_curr + 1;
            const char *placeholder_name_end = strchr(placeholder_name_start, '/');
            if (!placeholder_name_end) {
                placeholder_name_end = strchr(placeholder_name_start, '\0');
            }
            size_t placeholder_name_len = placeholder_name_end - placeholder_name_start;

            const char *url_segment_start = url_curr;
            const char *url_segment_end = strchr(url_curr, '/');
            if (!url_segment_end) {
                url_segment_end = strchr(url_curr, '\0');
            }
            size_t url_segment_len = url_segment_end - url_segment_start;

            // Check if it's a named splat (i.e., it's the last segment in the pattern)
            if (*placeholder_name_end == '\0') { // This is the last segment in the pattern
                add_matched_param(matched_params, placeholder_name_start, placeholder_name_len, url_curr, strlen(url_curr));
                return true; // Named splat matches the rest
            } else { // Regular placeholder (matches a single segment)
                if (url_segment_len == 0) return false; // Placeholder must match something
                add_matched_param(matched_params, placeholder_name_start, placeholder_name_len, url_segment_start, url_segment_len);
            }

            url_curr = url_segment_end;
            if (*url_curr == '/') url_curr++;
            pattern_curr = placeholder_name_end;
            if (*pattern_curr == '/') pattern_curr++;

        } else if (*pattern_curr == '*') { // Unnamed splat
            // According to documentation, '*' can only be at the end of a path segment.
            // If we find it in the middle, it's a mismatch.
            if (*(pattern_curr + 1) != '\0' && *(pattern_curr + 1) != '/') {
                return false; // '*' in middle of segment or followed by non-slash
            }
            // If '*' is the last segment in the pattern, it matches the rest of the URL
            if (*(pattern_curr + 1) == '\0') {
                add_matched_param(matched_params, "*", 1, url_curr, strlen(url_curr));
                return true; // Splat matches the rest
            } else { // '*' followed by '/', meaning it's a single segment wildcard
                // This case is problematic based on documentation. For now, treat as mismatch.
                // If the intent was to match a single segment, it should be a placeholder like ':segment'.
                return false;
            }
        } else { // Literal segment
            const char *pattern_segment_start = pattern_curr;
            const char *pattern_segment_end = strchr(pattern_curr, '/');
            if (!pattern_segment_end) {
                pattern_segment_end = strchr(pattern_curr, '\0');
            }
            size_t pattern_segment_len = pattern_segment_end - pattern_segment_start;

            const char *url_segment_start = url_curr;
            const char *url_segment_end = strchr(url_curr, '/');
            if (!url_segment_end) {
                url_segment_end = strchr(url_curr, '\0');
            }
            size_t url_segment_len = url_segment_end - url_segment_start;

            if (pattern_segment_len != url_segment_len || strncmp(pattern_segment_start, url_segment_start, pattern_segment_len) != 0) {
                return false; // Mismatch
            }

            url_curr = url_segment_end;
            if (*url_curr == '/') url_curr++;
            pattern_curr = pattern_segment_end;
            if (*pattern_curr == '/') pattern_curr++;
        }
    }

    // If both reached end simultaneously, it's a match
    return *url_curr == '\0' && *pattern_curr == '\0';
}

bool nr_match_query_params(const char *url_query, const nr_key_value_item_t *rule_query_params, uint8_t num_rule_query_params, nr_matched_params_t *matched_params) {
    if (num_rule_query_params == 0) {
        return true; // No query params to match in the rule
    }
    if (url_query == NULL || strlen(url_query) == 0) {
        return false; // Rule has query params, but URL doesn't
    }

    // Create a mutable copy of the URL query string for strtok_r
    char url_query_copy[NR_MAX_ROUTE_LEN + 1];
    strncpy(url_query_copy, url_query, NR_MAX_ROUTE_LEN);
    url_query_copy[NR_MAX_ROUTE_LEN] = '\0';

    char *rest_of_url_query = url_query_copy;

    for (uint8_t i = 0; i < num_rule_query_params; ++i) {
        const nr_key_value_item_t *rule_param = &rule_query_params[i];
        bool found_match = false;

        // Use a temporary copy for strtok_r to avoid modifying the original for subsequent rule params
        char temp_query_for_strtok[NR_MAX_ROUTE_LEN + 1];
        strncpy(temp_query_for_strtok, url_query, NR_MAX_ROUTE_LEN);
        temp_query_for_strtok[NR_MAX_ROUTE_LEN] = '\0';
        char *token_save_ptr = NULL;
        char *current_url_param = strtok_r(temp_query_for_strtok, "&", &token_save_ptr);

        while (current_url_param != NULL) {
            char *equals_sign = strchr(current_url_param, '=');
            if (equals_sign) {
                *equals_sign = '\0'; // Temporarily null-terminate key
                const char *url_key = current_url_param;
                const char *url_value = equals_sign + 1;

                if (strcmp(rule_param->key, url_key) == 0) {
                    if (rule_param->is_present) { // Placeholder like 'id=:id'
                        add_matched_param(matched_params, rule_param->key, strlen(rule_param->key), url_value, strlen(url_value));
                        found_match = true;
                        break;
                    } else { // Exact match like 'id=123'
                        if (strcmp(rule_param->value, url_value) == 0) {
                            found_match = true;
                            break;
                        }
                    }
                }
            } else { // Query param without value, e.g., "?param"
                if (strcmp(rule_param->key, current_url_param) == 0 && !rule_param->is_present && strlen(rule_param->value) == 0) {
                    found_match = true;
                    break;
                }
            }
            current_url_param = strtok_r(NULL, "&", &token_save_ptr);
        }

        if (!found_match) {
            return false; // A rule query param was not matched
        }
    }

    return true;
}

bool nanorouter_match_rule(const redirect_rule_t *rule, const char *url, nr_matched_params_t *matched_params) {
    char path_buffer[NR_MAX_ROUTE_LEN + 1];
    char query_buffer[NR_MAX_ROUTE_LEN + 1];

    nr_parse_url_path_and_query(url, path_buffer, sizeof(path_buffer), query_buffer, sizeof(query_buffer));

    // 1. Match path pattern
    if (!nr_match_path_pattern(path_buffer, rule->from_route, matched_params)) {
        return false;
    }

    // 2. Match query parameters
    if (rule->num_query_params > 0) {
        if (!nr_match_query_params(query_buffer, rule->query_params, rule->num_query_params, matched_params)) {
            return false;
        }
    }

    return true;
}
