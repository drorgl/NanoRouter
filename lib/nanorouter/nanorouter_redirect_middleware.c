#include "nanorouter_redirect_middleware.h"
#include "nanorouter_redirect_rule_parser.h" // For redirect_rule_t
#include "nanorouter_condition_matching.h" // For nanorouter_request_context_t and nanorouter_match_conditions
#include <stdlib.h> // For malloc, free
#include <string.h> // For strncpy, strlen, strncat
#include <stdio.h>  // For snprintf

#include "nanorouter_route_matcher.h" // For nanorouter_match_rule and nr_matched_params_t
#include "nanorouter_string_utils.h" // For string utility functions

/**
 * @brief Creates and initializes an empty nanorouter_redirect_rule_list_t.
 *
 * @return A pointer to the newly created list, or NULL if memory allocation fails.
 */
nanorouter_redirect_rule_list_t* nanorouter_redirect_rule_list_create() {
    nanorouter_redirect_rule_list_t *list = (nanorouter_redirect_rule_list_t*) malloc(sizeof(nanorouter_redirect_rule_list_t));
    if (list == NULL) {
        return NULL;
    }
    list->head = NULL;
    list->count = 0;
    return list;
}

/**
 * @brief Adds a new redirect_rule_t to the linked list.
 *
 * This function allocates a nanorouter_redirect_rule_t node, copies the rule_data into it,
 * and adds it to the end of the list.
 *
 * @param list A pointer to the nanorouter_redirect_rule_list_t.
 * @param rule_data A pointer to the redirect_rule_t data to be added.
 * @return true if the rule was successfully added, false otherwise (e.g., memory allocation failure).
 */
bool nanorouter_redirect_rule_list_add_rule(nanorouter_redirect_rule_list_t *list, const redirect_rule_t *rule_data) {
    if (list == NULL || rule_data == NULL) {
        return false;
    }

    nanorouter_redirect_rule_t *new_node = (nanorouter_redirect_rule_t*) malloc(sizeof(nanorouter_redirect_rule_t));
    if (new_node == NULL) {
        return false;
    }

    // Copy the rule data
    new_node->rule = *rule_data; // Direct copy since redirect_rule_t contains fixed-size arrays
    new_node->next = NULL;

    if (list->head == NULL) {
        list->head = new_node;
    } else {
        nanorouter_redirect_rule_t *current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    list->count++;
    return true;
}

/**
 * @brief Frees all memory associated with the redirect rule list and its contained nodes.
 *
 * @param list A pointer to the nanorouter_redirect_rule_list_t to be freed.
 */
void nanorouter_redirect_rule_list_free(nanorouter_redirect_rule_list_t *list) {
    if (list == NULL) {
        return;
    }

    nanorouter_redirect_rule_t *current = list->head;
    while (current != NULL) {
        nanorouter_redirect_rule_t *next = current->next;
        // No need to free individual members of current->rule as they are fixed-size arrays
        free(current);
        current = next;
    }
    free(list);
}

/**
 * @brief Safely appends a source string to a destination buffer, respecting buffer size.
 *
 * @param dest The destination buffer.
 * @param dest_size The total size of the destination buffer.
 * @param src The source string to append.
 */
static void nr_append_string_to_buffer(char *dest, size_t dest_size, const char *src) {
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    size_t copy_len = (dest_len + src_len < dest_size) ? src_len : (dest_size - dest_len - 1);
    strncat(dest, src, copy_len);
    dest[dest_size - 1] = '\0'; // Ensure null-termination
}

/**
 * @brief Extracts the query string part from a full URL.
 *
 * @param url The full URL string.
 * @param buffer The buffer to store the extracted query string.
 * @param buffer_size The size of the buffer.
 * @return A pointer to the buffer if a query string is found, otherwise NULL.
 */
static char* nr_extract_query_string(const char *url, char *buffer, size_t buffer_size) {
    const char *query_start = strchr(url, '?');
    if (query_start != NULL) {
        strncpy(buffer, query_start + 1, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return buffer;
    }
    return NULL;
}

// --- Middleware Function Implementation ---

/**
 * @brief Processes an incoming request URL against a list of redirect rules.
 *
 * If a matching rule is found, the response_context will be populated with the
 * new URL and status code.
 *
 * @param request_url The incoming URL string.
 * @param rules The nanorouter_redirect_rule_list_t containing all loaded redirect rules.
 * @param response_context A pointer to a nanorouter_redirect_response_t structure to be populated.
 * @return true if a redirect rule was applied and response_context was updated, false otherwise.
 */
bool nanorouter_process_redirect_request(
    const char *request_url,
    nanorouter_redirect_rule_list_t *rules,
    nanorouter_redirect_response_t *response_context,
    const nanorouter_request_context_t *request_context
) {
    // Initialize response_context to indicate no redirect by default
    if (response_context != NULL) {
        response_context->new_url[0] = '\0';
        response_context->status_code = 0;
    }

    if (request_url == NULL || rules == NULL || response_context == NULL) {
        return false;
    }

    nanorouter_redirect_rule_t *current_rule_node = rules->head;
    while (current_rule_node != NULL) {
        nr_matched_params_t matched_params;
        matched_params.num_params = 0; // Initialize matched_params

        // First, match the path and query parameters
        if (nanorouter_match_rule(&current_rule_node->rule, request_url, &matched_params)) {
            // If path and query match, then check conditions
            if (nanorouter_match_conditions(
                    current_rule_node->rule.conditions,
                    current_rule_node->rule.num_conditions,
                    request_context
                )) {
                // Both path/query and conditions match, apply the rule
                response_context->status_code = current_rule_node->rule.status_code;

                // Construct new_url from to_route and matched_params
                const char *to_route_ptr = current_rule_node->rule.to_route;
                char temp_new_url[NR_REDIRECT_MAX_URL_LEN + 1] = {0};
                size_t current_len = 0;

                while (*to_route_ptr != '\0' && current_len < NR_REDIRECT_MAX_URL_LEN) {
                    if (*to_route_ptr == ':' || *to_route_ptr == '*') {
                        // Found a placeholder or splat
                        const char *placeholder_or_splat_indicator = to_route_ptr; // Store ':' or '*'
                        const char *param_name_start_in_to_route = to_route_ptr + 1; // Start of actual name (e.g., "id" from ":id")

                        const char *param_name_end_in_to_route = param_name_start_in_to_route;
                        while (*param_name_end_in_to_route != '\0' && *param_name_end_in_to_route != '/' && *param_name_end_in_to_route != '?') {
                            param_name_end_in_to_route++;
                        }
                        size_t param_name_len = param_name_end_in_to_route - param_name_start_in_to_route;

                        char search_key[NR_MAX_MATCHED_KEY_LEN + 1];
                        if (*placeholder_or_splat_indicator == '*') {
                            strncpy(search_key, "*", NR_MAX_MATCHED_KEY_LEN); // Use "*" as key for splats
                            search_key[NR_MAX_MATCHED_KEY_LEN] = '\0';
                        } else { // It's a ':' placeholder
                            // Check if the placeholder name is "splat"
                            if (param_name_len == strlen("splat") && strncmp(param_name_start_in_to_route, "splat", param_name_len) == 0) {
                                strncpy(search_key, "*", NR_MAX_MATCHED_KEY_LEN); // Map :splat to * for lookup
                                search_key[NR_MAX_MATCHED_KEY_LEN] = '\0';
                            } else {
                                strncpy(search_key, param_name_start_in_to_route, param_name_len);
                                search_key[param_name_len] = '\0';
                            }
                        }

                        // Search for the matched parameter
                        bool found_param = false;
                        for (uint8_t i = 0; i < matched_params.num_params; i++) {
                            if (strcmp(matched_params.params[i].key, search_key) == 0) {
                                nr_append_string_to_buffer(temp_new_url, NR_REDIRECT_MAX_URL_LEN + 1, matched_params.params[i].value);
                                current_len = strlen(temp_new_url);
                                found_param = true;
                                break;
                            }
                        }
                        if (!found_param) {
                            // If param not found in matched_params, append the original placeholder/splat indicator and name
                            nr_append_string_to_buffer(temp_new_url, NR_REDIRECT_MAX_URL_LEN + 1, placeholder_or_splat_indicator);
                            nr_append_string_to_buffer(temp_new_url, NR_REDIRECT_MAX_URL_LEN + 1, param_name_start_in_to_route);
                            current_len = strlen(temp_new_url);
                        }
                        to_route_ptr = param_name_end_in_to_route;
                    } else {
                        // Append literal character
                        temp_new_url[current_len++] = *to_route_ptr++;
                        temp_new_url[current_len] = '\0';
                    }
                }

                // Append original query string if to_route doesn't specify one
                // This logic needs to be careful not to duplicate query parameters already handled by placeholders.
                // The rule is: if the to_route itself contains a '?', assume it explicitly defines its query params.
                // Otherwise, append the original query string from the request_url, excluding those already matched.
                if (strchr(current_rule_node->rule.to_route, '?') == NULL) {
                    char original_query_full_buffer[NR_MAX_ROUTE_LEN + 1]; // Buffer for the full original query string
                    char *original_query_str = nr_extract_query_string(request_url, original_query_full_buffer, sizeof(original_query_full_buffer));

                    if (original_query_str != NULL && strlen(original_query_str) > 0) {
                        char remaining_query_buffer[NR_MAX_ROUTE_LEN + 1] = {0};
                        char temp_original_query_copy[NR_MAX_ROUTE_LEN + 1]; // Copy for strtok_r
                        strncpy(temp_original_query_copy, original_query_str, sizeof(temp_original_query_copy) - 1);
                        temp_original_query_copy[sizeof(temp_original_query_copy) - 1] = '\0';

                        char *token_save_ptr = NULL;
                        char *current_param_pair = strtok_r(temp_original_query_copy, "&", &token_save_ptr);
                        bool first_param = true;

                        while (current_param_pair != NULL) {
                            char *equals_sign = strchr(current_param_pair, '=');
                            char param_key[NR_MAX_QUERY_KEY_LEN + 1];
                            
                            if (equals_sign) {
                                size_t key_len = equals_sign - current_param_pair;
                                strncpy(param_key, current_param_pair, key_len);
                                param_key[key_len] = '\0';
                            } else {
                                strncpy(param_key, current_param_pair, NR_MAX_QUERY_KEY_LEN);
                                param_key[NR_MAX_QUERY_KEY_LEN] = '\0';
                            }

                            // If to_route does not explicitly define query parameters,
                            // all original query parameters should be passed through.
                            // The 'handled_by_placeholder' check is removed here to ensure this.
                            if (!first_param) {
                                nr_append_string_to_buffer(remaining_query_buffer, sizeof(remaining_query_buffer), "&");
                            }
                            nr_append_string_to_buffer(remaining_query_buffer, sizeof(remaining_query_buffer), current_param_pair);
                            first_param = false;
                            current_param_pair = strtok_r(NULL, "&", &token_save_ptr);
                        }

                        if (strlen(remaining_query_buffer) > 0) {
                            if (strchr(temp_new_url, '?') == NULL) {
                                nr_append_string_to_buffer(temp_new_url, NR_REDIRECT_MAX_URL_LEN + 1, "?");
                            }
                            nr_append_string_to_buffer(temp_new_url, NR_REDIRECT_MAX_URL_LEN + 1, remaining_query_buffer);
                        }
                    }
                }
                
                strncpy(response_context->new_url, temp_new_url, NR_REDIRECT_MAX_URL_LEN);
                response_context->new_url[NR_REDIRECT_MAX_URL_LEN] = '\0'; // Ensure null-termination

                return true; // Rule applied
            }
        }
        current_rule_node = current_rule_node->next;
    }

    return false; // No rule applied
}
