#include "nanorouter_condition_matching.h"
#include "nanorouter_string_utils.h" // For nr_string_split, nr_trim_whitespace
#include <string.h> // For strcmp, strcasecmp, strncpy
#include <stdlib.h> // For malloc, free

// Helper struct for nr_string_split callback to check if a value is present
typedef struct {
    const char *target_value;
    bool found;
    bool is_language_match; // New flag for language-specific matching
} value_search_data_t;

// Callback function for nr_string_split to check for target_value
static void value_search_callback(const char *token, size_t token_len, size_t token_index, void *user_data) {
    (void)token_index; // Unused parameter
    value_search_data_t *search_data = (value_search_data_t *)user_data;

    // Create a temporary buffer for the token to trim it
    char *temp_token = (char*)malloc(token_len + 1);
    if (temp_token == NULL) {
        // Handle allocation error, cannot proceed with comparison
        return;
    }
    strncpy(temp_token, token, token_len);
    temp_token[token_len] = '\0';
    char *trimmed_token = nr_trim_whitespace(temp_token);

    if (search_data->is_language_match) {
        // For language matching, check if the target value starts with the trimmed token
        // e.g., rule "en" should match context "en-US"
        size_t rule_lang_len = strlen(trimmed_token);
        if (strncasecmp(search_data->target_value, trimmed_token, rule_lang_len) == 0) {
            // Ensure it's a full match or followed by a hyphen
            if (strlen(search_data->target_value) == rule_lang_len || (search_data->target_value[rule_lang_len] == '-' && strlen(search_data->target_value) > rule_lang_len)) {
                search_data->found = true;
            }
        }
    } else {
        // For other types, perform exact match
        if (strcasecmp(trimmed_token, search_data->target_value) == 0) {
            search_data->found = true;
        }
    }
    free(temp_token);
}

// Helper struct for nr_string_split callback to collect language tags
typedef struct {
    char **tags;
    uint8_t count;
    uint8_t capacity;
} language_tags_collection_t;

// Callback function for nr_string_split to collect language tags
static void language_tag_collection_callback(const char *token, size_t token_len, size_t token_index, void *user_data) {
    (void)token_index; // Unused parameter
    language_tags_collection_t *collection = (language_tags_collection_t *)user_data;

    // Ignore q-value parts (e.g., ";q=0.9")
    const char *semicolon_pos = strchr(token, ';');
    size_t effective_len = (semicolon_pos != NULL) ? (size_t)(semicolon_pos - token) : token_len;

    // Create a temporary buffer for the token to trim it
    char *temp_token = (char*)malloc(effective_len + 1);
    if (temp_token == NULL) {
        // Handle allocation error
        return;
    }
    strncpy(temp_token, token, effective_len);
    temp_token[effective_len] = '\0';
    char *trimmed_token = nr_trim_whitespace(temp_token);

    if (strlen(trimmed_token) > 0) {
        if (collection->count >= collection->capacity) {
            // Resize array if needed (simple realloc for now, could be more robust)
            collection->capacity *= 2;
            collection->tags = (char**)realloc(collection->tags, collection->capacity * sizeof(char*));
            if (collection->tags == NULL) {
                free(temp_token);
                return; // Handle realloc error
            }
        }
        collection->tags[collection->count++] = strdup(trimmed_token); // Duplicate to store
    }
    free(temp_token);
}

/**
 * @brief Extracts primary language tags from an Accept-Language header string.
 *        E.g., "en-US,en;q=0.9,fr;q=0.8" -> ["en-US", "en", "fr"]
 *
 * @param accept_language_header The full Accept-Language header string.
 * @param out_tags A pointer to an array of char* that will be allocated and populated.
 *                 The caller is responsible for freeing each string and the array itself.
 * @param out_count A pointer to a uint8_t that will store the number of extracted tags.
 */
static void nr_extract_primary_language_tags(
    const char *accept_language_header,
    char ***out_tags,
    uint8_t *out_count
) {
    *out_tags = NULL;
    *out_count = 0;

    if (accept_language_header == NULL || strlen(accept_language_header) == 0) {
        return;
    }

    language_tags_collection_t collection = {
        .tags = (char**)malloc(sizeof(char*) * 4), // Initial capacity
        .count = 0,
        .capacity = 4
    };
    if (collection.tags == NULL) {
        return; // Handle allocation error
    }

    nr_string_split(accept_language_header, strlen(accept_language_header), ",", language_tag_collection_callback, &collection);

    *out_tags = collection.tags;
    *out_count = collection.count;
}

/**
 * @brief Checks if a comma-separated string contains a specific target value (case-insensitive).
 *
 * @param list_str The comma-separated string (e.g., "value1, value2,value3").
 * @param target_value The specific value to search for.
 * @param is_language_match If true, performs a language-specific "starts with" match.
 * @return true if the target_value is found, false otherwise.
 */
static bool nr_list_contains_value(const char *list_str, const char *target_value, bool is_language_match) {
    if (list_str == NULL || target_value == NULL || strlen(list_str) == 0 || strlen(target_value) == 0) {
        return false;
    }

    value_search_data_t search_data = {
        .target_value = target_value,
        .found = false,
        .is_language_match = is_language_match
    };

    nr_string_split(list_str, strlen(list_str), ",", value_search_callback, &search_data);

    return search_data.found;
}

bool nanorouter_match_conditions(
    const nr_condition_item_t *conditions,
    uint8_t num_conditions,
    const nanorouter_request_context_t *request_context
) {
    // If there are no conditions in the rule, it's always a match.
    if (num_conditions == 0) {
        return true;
    }

    // If conditions exist but no request context is provided, it's a mismatch.
    if (request_context == NULL) {
        return false;
    }

    for (uint8_t i = 0; i < num_conditions; i++) {
        const nr_condition_item_t *condition = &conditions[i];
        bool condition_met = false;

        if (strcasecmp(condition->key, "Country") == 0) {
            if (strlen(request_context->country) > 0) {
                condition_met = nr_list_contains_value(condition->value, request_context->country, false);
            }
        } else if (strcasecmp(condition->key, "Language") == 0) {
            if (strlen(request_context->language) > 0) {
                char **extracted_tags = NULL;
                uint8_t num_extracted_tags = 0;
                nr_extract_primary_language_tags(request_context->language, &extracted_tags, &num_extracted_tags);

                for (uint8_t j = 0; j < num_extracted_tags; j++) {
                    if (nr_list_contains_value(condition->value, extracted_tags[j], true)) { // Use language-specific matching
                        condition_met = true;
                        break;
                    }
                }

                // Free allocated memory for extracted tags
                for (uint8_t j = 0; j < num_extracted_tags; j++) {
                    free(extracted_tags[j]);
                }
                free(extracted_tags);
            }
        } else if (strcasecmp(condition->key, "Domain") == 0) {
            if (strlen(request_context->domain) > 0) {
                condition_met = (strcasecmp(condition->value, request_context->domain) == 0);
            }
        } else {
            // Unknown condition key, treat as not met for strict matching
            return false;
        }

        if (!condition_met) {
            return false; // If any single condition is not met, the entire rule fails.
        }
    }

    return true; // All conditions were met.
}
