#include "nanorouter_redirect_rule_parser.h"
#include "nanorouter_string_utils.h" // For nr_trim_string and nr_string_split

#include <stdbool.h>
#include <string.h>  // For strncpy, strlen, strchr, strncmp
#include <stdlib.h>  // For malloc, free, atoi
#include <ctype.h>   // For isdigit, isspace
#include "nanorouter_config.h" // For configuration defines

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// Helper function to check if a token is purely numeric
static bool is_numeric_token(const char *token, size_t len) {
    if (token == NULL || len == 0) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)token[i])) {
            return false;
        }
    }
    return true;
}

// Internal context for nr_process_redirect_rule's nr_string_split callback
typedef struct {
    nr_redirect_rule_part_callback_t user_callback;
    void *user_data;
    size_t token_count;
    bool to_route_identified;
    bool status_identified;
} InternalProcessContext;

// Internal callback for nr_string_split to process redirect rule parts
static void internal_redirect_split_callback(const char *token, size_t token_len, size_t token_index, void *user_data) {
    InternalProcessContext *context = (InternalProcessContext *)user_data;
    nr_redirect_part_type_t part_type = NR_REDIRECT_PART_UNKNOWN;

    // Make a mutable copy of the token to allow modification (e.g., removing '!')
    char *mutable_token = (char *)malloc(token_len + 1);
    if (mutable_token == NULL) {
        // Handle allocation error, perhaps log and return
        return;
    }
    strncpy(mutable_token, token, token_len);
    mutable_token[token_len] = '\0';

    // Logic based on user's refined plan
    if (context->token_count == 0) {
        part_type = NR_REDIRECT_PART_FROM_ROUTE;
    } else {
        // Check for query parameters (contains '=')
        if (strchr(mutable_token, '=') != NULL) {
            // Check if it's a condition (e.g., Country=, Language=)
            if (strncmp(mutable_token, "Country=", 8) == 0 || strncmp(mutable_token, "Language=", 9) == 0) {
                part_type = NR_REDIRECT_PART_CONDITION;
            } else {
                part_type = NR_REDIRECT_PART_QUERY;
            }
        } else if (!context->to_route_identified) {
            // If TO_ROUTE not yet identified, this is it
            part_type = NR_REDIRECT_PART_TO_ROUTE;
            context->to_route_identified = true;
        } else {
            // Check for status code (numeric, possibly with '!')
            if (!context->status_identified) {
                size_t actual_token_len = token_len;
                bool is_force = false;
                if (token_len > 0 && mutable_token[token_len - 1] == '!') {
                    is_force = true;
                    actual_token_len--; // Exclude '!' for status check
                }

                if (is_numeric_token(mutable_token, actual_token_len)) {
                    part_type = NR_REDIRECT_PART_STATUS;
                    context->status_identified = true;
                    // Callback for status
                    context->user_callback(mutable_token, actual_token_len, part_type, context->user_data);
                    if (is_force) {
                        // Callback for force if '!' was present
                        context->user_callback("!", 1, NR_REDIRECT_PART_FORCE, context->user_data);
                    }
                    free(mutable_token);
                    context->token_count++; // Increment after potential two callbacks
                    return; // Skip default callback at the end
                }
            }
            // If not status, query, or condition, and to_route is identified, it's unknown
            // This case should ideally not be reached if all parts are well-defined
            // but serves as a fallback.
            // if (part_type == NR_REDIRECT_PART_UNKNOWN) {
            //     part_type = NR_REDIRECT_PART_UNKNOWN;
            // }
        }
    }

    context->user_callback(token, token_len, part_type, context->user_data);
    free(mutable_token);
    context->token_count++;
}

void nr_process_redirect_rule(const char *rule_line, size_t rule_line_len, nr_redirect_rule_part_callback_t callback, void *user_data) {
    if (rule_line == NULL || rule_line_len == 0 || callback == NULL) {
        return;
    }

    // Create a mutable copy of the rule_line for trimming
    char *mutable_rule_line = (char *)malloc(rule_line_len + 1);
    if (mutable_rule_line == NULL) {
        return; // Handle allocation error
    }
    strncpy(mutable_rule_line, rule_line, rule_line_len);
    mutable_rule_line[rule_line_len] = '\0';

    nr_trim_string(mutable_rule_line, rule_line_len);
    size_t trimmed_len = strlen(mutable_rule_line);

    if (trimmed_len == 0 || mutable_rule_line[0] == '#') {
        free(mutable_rule_line);
        return; // Ignore empty lines and comments
    }

    InternalProcessContext context = {
        .user_callback = callback,
        .user_data = user_data,
        .token_count = 0,
        .to_route_identified = false,
        .status_identified = false
    };

    nr_string_split(mutable_rule_line, trimmed_len, " ", internal_redirect_split_callback, &context);

    free(mutable_rule_line);
}


/**
 * @brief Callback function used by nr_process_redirect_rule to populate a redirect_rule_t struct.
 *
 * This function is intended to be used as the callback for nr_process_redirect_rule.
 * It parses individual parts of a redirect rule and stores them into the provided
 * redirect_rule_t structure.
 *
 * @param token The string token representing a part of the rule.
 * @param token_len The length of the token.
 * @param part_type The type of the redirect rule part (e.g., from_route, to_route, status).
 * @param user_data A pointer to the redirect_rule_t struct to be populated.
 */
static void nr_redirect_rule_parser_callback(
    const char *token,
    size_t token_len,
    nr_redirect_part_type_t part_type,
    void *user_data
) {
    redirect_rule_t *rule = (redirect_rule_t *)user_data;
    if (rule == NULL) {
        return; // Should not happen if called correctly
    }

    switch (part_type) {
        case NR_REDIRECT_PART_FROM_ROUTE:
            strncpy(rule->from_route, token, MIN(token_len, NR_MAX_ROUTE_LEN));
            rule->from_route[MIN(token_len, NR_MAX_ROUTE_LEN)] = '\0';
            break;
        case NR_REDIRECT_PART_TO_ROUTE:
            strncpy(rule->to_route, token, MIN(token_len, NR_MAX_ROUTE_LEN));
            rule->to_route[MIN(token_len, NR_MAX_ROUTE_LEN)] = '\0';
            break;
        case NR_REDIRECT_PART_STATUS:
            // Convert token to integer status code
            rule->status_code = (uint16_t)atoi(token);
            break;
        case NR_REDIRECT_PART_FORCE:
            rule->force = true;
            break;
        case NR_REDIRECT_PART_QUERY: {
            if (rule->num_query_params < NR_MAX_QUERY_ITEMS) {
                nr_key_value_item_t *qp = &rule->query_params[rule->num_query_params];
                char *equals_pos = strchr((char*)token, '='); // Cast to char* for strchr
                if (equals_pos != NULL) {
                    size_t key_len = equals_pos - token;
                    strncpy(qp->key, token, MIN(key_len, NR_MAX_QUERY_KEY_LEN));
                    qp->key[MIN(key_len, NR_MAX_QUERY_KEY_LEN)] = '\0';

                    size_t value_len = token_len - (key_len + 1);
                    strncpy(qp->value, equals_pos + 1, MIN(value_len, NR_MAX_QUERY_VALUE_LEN));
                    qp->value[MIN(value_len, NR_MAX_QUERY_VALUE_LEN)] = '\0';
                } else {
                    // Handle cases where '=' is missing, e.g., just a key
                    strncpy(qp->key, token, MIN(token_len, NR_MAX_QUERY_KEY_LEN));
                    qp->key[MIN(token_len, NR_MAX_QUERY_KEY_LEN)] = '\0';
                    qp->value[0] = '\0'; // Empty value
                }
                qp->is_present = true;
                rule->num_query_params++;
            }
            break;
        }
        case NR_REDIRECT_PART_CONDITION: {
            if (rule->num_conditions < NR_MAX_CONDITION_ITEMS) {
                nr_condition_item_t *cond = &rule->conditions[rule->num_conditions];
                char *equals_pos = strchr((char*)token, '='); // Cast to char* for strchr
                if (equals_pos != NULL) {
                    size_t key_len = equals_pos - token;
                    strncpy(cond->key, token, MIN(key_len, NR_MAX_CONDITION_KEY_LEN));
                    cond->key[MIN(key_len, NR_MAX_CONDITION_KEY_LEN)] = '\0';

                    size_t value_len = token_len - (key_len + 1);
                    strncpy(cond->value, equals_pos + 1, MIN(value_len, NR_MAX_CONDITION_VALUE_LEN));
                    cond->value[MIN(value_len, NR_MAX_CONDITION_VALUE_LEN)] = '\0';
                } else {
                    // Handle cases where '=' is missing, e.g., just a key
                    strncpy(cond->key, token, MIN(token_len, NR_MAX_CONDITION_KEY_LEN));
                    cond->key[MIN(token_len, NR_MAX_CONDITION_KEY_LEN)] = '\0';
                    cond->value[0] = '\0'; // Empty value
                }
                cond->is_present = true;
                rule->num_conditions++;
            }
            break;
        }
        case NR_REDIRECT_PART_UNKNOWN:
            // Log or ignore unknown parts
            break;
    }
}

// --- Implementation of nr_parse_redirect_rule ---
bool nr_parse_redirect_rule(
    const char *rule_line,
    size_t rule_line_len,
    redirect_rule_t *rule
) {
    if (rule_line == NULL || rule_line_len == 0 || rule == NULL) {
        return false;
    }

    // Initialize the rule struct to a known state (all zeros/empty)
    memset(rule, 0, sizeof(redirect_rule_t));

    // Call the core processing function with our callback
    nr_process_redirect_rule(rule_line, rule_line_len, nr_redirect_rule_parser_callback, rule);

    // A rule is considered successfully parsed if it has at least a from_route and to_route
    // and is not an empty line or comment (which nr_process_redirect_rule already filters)
    return (strlen(rule->from_route) > 0 && strlen(rule->to_route) > 0);
}
