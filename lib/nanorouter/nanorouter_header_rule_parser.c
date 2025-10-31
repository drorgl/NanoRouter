#include "nanorouter_header_rule_parser.h"
#include "nanorouter_string_utils.h" // For nr_trim_whitespace
#include <string.h> // For strncpy, strlen, strchr, strstr
#include <stdio.h>  // For sscanf, snprintf
#include <stdlib.h> // For malloc, free

/**
 * @brief Creates and initializes an empty nanorouter_header_rule_list_t.
 *
 * @return A pointer to the newly created list, or NULL if memory allocation fails.
 */
nanorouter_header_rule_list_t* nanorouter_header_rule_list_create() {
    nanorouter_header_rule_list_t *list = (nanorouter_header_rule_list_t*) malloc(sizeof(nanorouter_header_rule_list_t));
    if (list == NULL) {
        return NULL;
    }
    list->head = NULL;
    list->count = 0;
    return list;
}

/**
 * @brief Adds a new header_rule_t to the linked list.
 *
 * This function allocates a nanorouter_header_rule_node_t node, copies the rule_data into it,
 * and adds it to the end of the list.
 *
 * @param list A pointer to the nanorouter_header_rule_list_t.
 * @param rule_data A pointer to the header_rule_t data to be added.
 * @return true if the rule was successfully added, false otherwise (e.g., memory allocation failure).
 */
bool nanorouter_header_rule_list_add_rule(nanorouter_header_rule_list_t *list, const header_rule_t *rule_data) {
    if (list == NULL || rule_data == NULL) {
        return false;
    }

    nanorouter_header_rule_node_t *new_node = (nanorouter_header_rule_node_t*) malloc(sizeof(nanorouter_header_rule_node_t));
    if (new_node == NULL) {
        return false;
    }

    // Copy the rule data
    new_node->rule = *rule_data; // Direct copy
    new_node->next = NULL;

    if (list->head == NULL) {
        list->head = new_node;
    } else {
        nanorouter_header_rule_node_t *current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    list->count++;
    return true;
}

/**
 * @brief Frees all memory associated with the header rule list and its contained nodes.
 *
 * @param list A pointer to the nanorouter_header_rule_list_t to be freed.
 */
void nanorouter_header_rule_list_free(nanorouter_header_rule_list_t *list) {
    if (list == NULL) {
        return;
    }

    nanorouter_header_rule_node_t *current = list->head;
    while (current != NULL) {
        nanorouter_header_rule_node_t *next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

/**
 * @brief Parses a _headers file content and populates a list of header_rule_t.
 *
 * @param file_content The content of the _headers file as a string.
 * @param rule_list A pointer to the nanorouter_header_rule_list_t to populate.
 * @return true if parsing was successful, false otherwise.
 */
bool nanorouter_parse_headers_file(const char *file_content, nanorouter_header_rule_list_t *rule_list) {
    if (file_content == NULL || rule_list == NULL) {
        return false;
    }

    char line_buffer[NR_MAX_HEADER_VALUE_LEN + NR_MAX_HEADER_KEY_LEN + 2]; // Max possible line length
    const char *current_pos = file_content;
    header_rule_t current_rule = {0};
    bool in_rule_block = false;

    while (*current_pos != '\0') {
        const char *line_end = strchr(current_pos, '\n');
        size_t line_len;
        if (line_end == NULL) {
            line_len = strlen(current_pos);
        } else {
            line_len = line_end - current_pos;
        }

        if (line_len >= sizeof(line_buffer)) {
            // Line too long, skip or handle error
            if (line_end == NULL) break; // End of content
            current_pos = line_end + 1;
            continue;
        }
        strncpy(line_buffer, current_pos, line_len);
        line_buffer[line_len] = '\0';
        
        char *trimmed_line = nr_trim_whitespace(line_buffer);

        if (strlen(trimmed_line) == 0 || trimmed_line[0] == '#') {
            // Empty line or comment, skip
        } else if (trimmed_line[0] == '/') {
            // New route definition
            if (in_rule_block) {
                // Add the previous rule to the list
                if (!nanorouter_header_rule_list_add_rule(rule_list, &current_rule)) {
                    return false; // Failed to add rule
                }
            }
            // Start a new rule
            memset(&current_rule, 0, sizeof(header_rule_t));
            strncpy(current_rule.from_route, trimmed_line, NR_MAX_ROUTE_LEN);
            current_rule.from_route[NR_MAX_ROUTE_LEN] = '\0';
            current_rule.num_headers = 0;
            in_rule_block = true;
        } else if (in_rule_block) {
            // Header key-value pair
            char *colon_pos = strchr(trimmed_line, ':');
            if (colon_pos != NULL) {
                if (current_rule.num_headers < NR_MAX_HEADERS_PER_RULE) {
                    // Extract key
                    size_t key_len = colon_pos - trimmed_line;
                    strncpy(current_rule.headers[current_rule.num_headers].key, trimmed_line, key_len);
                    current_rule.headers[current_rule.num_headers].key[key_len] = '\0';
                    nr_trim_whitespace(current_rule.headers[current_rule.num_headers].key);

                    // Extract value
                    char *value_start = colon_pos + 1;
                    char temp_value_buffer[NR_MAX_HEADER_VALUE_LEN + 1];
                    strncpy(temp_value_buffer, value_start, NR_MAX_HEADER_VALUE_LEN);
                    temp_value_buffer[NR_MAX_HEADER_VALUE_LEN] = '\0';
                    char *trimmed_value = nr_trim_whitespace(temp_value_buffer); // Trim the temporary buffer

                    strncpy(current_rule.headers[current_rule.num_headers].value, trimmed_value, NR_MAX_HEADER_VALUE_LEN);
                    current_rule.headers[current_rule.num_headers].value[NR_MAX_HEADER_VALUE_LEN] = '\0';

                    current_rule.num_headers++;
                } else {
                    // Too many headers for this rule, log warning or handle error
                }
            } else {
                // Malformed header line, log warning or handle error
            }
        } else {
            // Line outside a rule block and not a route, comment, or empty.
            // This could be an error or unexpected format.
        }

        if (line_end == NULL) {
            break; // End of content
        }
        current_pos = line_end + 1;
    }

    // Add the last rule if it was being built
    if (in_rule_block) {
        if (!nanorouter_header_rule_list_add_rule(rule_list, &current_rule)) {
            return false; // Failed to add last rule
        }
    }

    return true;
}
