#include "nanorouter_headers_middleware.h"
#include "nanorouter_header_rule_parser.h" // For nanorouter_header_rule_list_t and header_rule_t, NR_MAX_HEADER_VALUE_LEN
#include "nanorouter_condition_matching.h" // For nanorouter_request_context_t and nanorouter_match_conditions
#include "nanorouter_route_matcher.h" // For nanorouter_match_rule and nr_matched_params_t
#include "nanorouter_string_utils.h" // For string utility functions (nr_string_split, nr_trim_whitespace)
#include <stdlib.h> // For malloc, free
#include <string.h> // For strncpy, strlen, strcmp, strncat, strcasecmp
#include <stdio.h>  // For snprintf
#include <stdbool.h> // For bool type

// List of HTTP header names that are typically ignored by the middleware
// as they are managed by the underlying web server.
static const char* const IGNORED_HEADERS[] = {
    "Accept-Ranges",
    "Age",
    "Allow",
    "Alt-Svc",
    "Connection",
    "Content-Encoding",
    "Content-Length",
    "Content-Range",
    "Date",
    "Server",
    "Set-Cookie",
    "Trailer",
    "Transfer-Encoding",
    "Upgrade"
};
static const size_t NUM_IGNORED_HEADERS = sizeof(IGNORED_HEADERS) / sizeof(IGNORED_HEADERS[0]);

// Helper struct to pass data to the nr_string_split callback
typedef struct {
    const char *target_value;
    bool found;
} header_value_search_data_t;

// Callback function for nr_string_split to check for target_value
static void header_value_search_callback(const char *token, size_t token_len, size_t token_index, void *user_data) {
    (void)token_index; // Unused parameter
    header_value_search_data_t *search_data = (header_value_search_data_t *)user_data;

    // Create a temporary buffer for the token to trim it
    if (token_len < NR_MAX_HEADER_VALUE_LEN) {
        char temp_token[NR_MAX_HEADER_VALUE_LEN]; // NR_MAX_HEADER_VALUE_LEN is defined in nanorouter_header_rule_parser.h
        strncpy(temp_token, token, token_len);
        temp_token[token_len] = '\0';
        char *trimmed_token = nr_trim_whitespace(temp_token); // Use existing trim function

        if (strcasecmp(trimmed_token, search_data->target_value) == 0) {
            search_data->found = true;
        }
    }
}

/**
 * @brief Checks if a comma-separated header value string contains a specific target value.
 *
 * This function tokenizes the header_value_str by commas, trims whitespace from each token,
 * and performs a case-insensitive comparison with the target_value.
 *
 * @param header_value_str The comma-separated string of header values (e.g., "value1, value2,value3").
 * @param target_value The specific value to search for.
 * @return true if the target_value is found, false otherwise.
 */
static bool nr_header_value_contains(const char *header_value_str, const char *target_value) {
    if (header_value_str == NULL || target_value == NULL) {
        return false;
    }

    header_value_search_data_t search_data = {
        .target_value = target_value,
        .found = false
    };

    // Use nr_string_split to tokenize the header_value_str by comma
    nr_string_split(header_value_str, strlen(header_value_str), ",", header_value_search_callback, &search_data);

    return search_data.found;
}

/**
 * @brief Checks if a given header key should be ignored.
 *
 * @param header_key The header key to check.
 * @return true if the header should be ignored, false otherwise.
 */
static bool is_ignored_header(const char *header_key) {
    for (size_t i = 0; i < NUM_IGNORED_HEADERS; i++) {
        if (strcasecmp(header_key, IGNORED_HEADERS[i]) == 0) { // Case-insensitive comparison
            return true;
        }
    }
    return false;
}


/**
 * @brief Processes an incoming request URL against a list of header rules.
 *
 * If matching rules are found, the response_context will be populated with the
 * headers to be applied.
 *
 * @param request_url The incoming URL string.
 * @param rules The nanorouter_header_rule_list_t containing all loaded header rules.
 * @param response_context A pointer to a nanorouter_header_response_t structure to be populated.
 * @return true if any header rules were applied and response_context was updated, false otherwise.
 */
bool nanorouter_process_header_request(
    const char *request_url,
    nanorouter_header_rule_list_t *rules,
    nanorouter_header_response_t *response_context,
    const nanorouter_request_context_t *request_context
) {
    if (request_url == NULL || rules == NULL || response_context == NULL) {
        return false;
    }

    response_context->num_headers = 0; // Initialize to no headers

    nanorouter_header_rule_node_t *current_rule_node = rules->head;
    bool rule_applied = false;

    while (current_rule_node != NULL) {
        nr_matched_params_t matched_params;
        matched_params.num_params = 0; // Initialize matched_params

        // Create a dummy redirect_rule_t to pass to nanorouter_match_rule for path matching.
        // Header rules don't have query params or conditions in their definition,
        // but the matcher expects a redirect_rule_t.
        redirect_rule_t dummy_redirect_rule;
        memset(&dummy_redirect_rule, 0, sizeof(redirect_rule_t)); // Initialize to zeros
        strncpy(dummy_redirect_rule.from_route, current_rule_node->rule.from_route, NR_MAX_ROUTE_LEN);
        dummy_redirect_rule.from_route[NR_MAX_ROUTE_LEN] = '\0';

        if (nanorouter_match_rule(&dummy_redirect_rule, request_url, &matched_params)) {
            // For header rules, conditions are not defined in the _headers file itself.
            // However, if the request_context is provided, we should still pass it to
            // nanorouter_match_conditions, even if num_conditions is 0 for header rules.
            // This ensures consistency in the API.
            if (nanorouter_match_conditions(
                    dummy_redirect_rule.conditions, // This will be empty for header rules
                    dummy_redirect_rule.num_conditions, // This will be 0 for header rules
                    request_context
                )) {
                rule_applied = true;
                for (uint8_t i = 0; i < current_rule_node->rule.num_headers; i++) {
                    const nanorouter_header_entry_t *header_entry = &current_rule_node->rule.headers[i];

                    if (is_ignored_header(header_entry->key)) {
                        continue; // Skip ignored headers
                    }

                    // Check if this header key already exists in the response_context
                    bool header_exists = false;
                    for (uint8_t j = 0; j < response_context->num_headers; j++) {
                        if (strcasecmp(response_context->headers[j].key, header_entry->key) == 0) {
                            // Check if the exact value already exists in the concatenated string
                            if (!nr_header_value_contains(response_context->headers[j].value, header_entry->value)) {
                                // Multi-value header: concatenate values if the value is new
                                size_t current_value_len = strlen(response_context->headers[j].value);
                                size_t new_value_len = strlen(header_entry->value);
                                
                                if (current_value_len + 1 + new_value_len < NR_MAX_HEADER_VALUE_LEN) { // +1 for comma
                                    strncat(response_context->headers[j].value, ",", NR_MAX_HEADER_VALUE_LEN - current_value_len - 1);
                                    strncat(response_context->headers[j].value, header_entry->value, NR_MAX_HEADER_VALUE_LEN - (current_value_len + 1) - 1);
                                    response_context->headers[j].value[NR_MAX_HEADER_VALUE_LEN] = '\0';
                                }
                            }
                            header_exists = true; // Mark as existing, even if value wasn't concatenated
                            break;
                        }
                    }

                    if (!header_exists) {
                        // Add new header if space is available
                        if (response_context->num_headers < NR_HEADERS_MAX_ENTRIES_PER_RESPONSE) {
                            strncpy(response_context->headers[response_context->num_headers].key, header_entry->key, NR_MAX_HEADER_KEY_LEN);
                            response_context->headers[response_context->num_headers].key[NR_MAX_HEADER_KEY_LEN] = '\0';
                            strncpy(response_context->headers[response_context->num_headers].value, header_entry->value, NR_MAX_HEADER_VALUE_LEN);
                            response_context->headers[response_context->num_headers].value[NR_MAX_HEADER_VALUE_LEN] = '\0';
                            response_context->num_headers++;
                        }
                    }
                }
            }
        }
        current_rule_node = current_rule_node->next;
    }

    return rule_applied;
}
