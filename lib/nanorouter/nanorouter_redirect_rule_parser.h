#ifndef NANOROUTER_RULE_PARSER_H
#define NANOROUTER_RULE_PARSER_H

#include <stdbool.h> // For bool type
#include <stdint.h>  // For uint16_t, etc.
#include <stddef.h>  // For size_t

#include "nanorouter_config.h" // For configuration defines

// --- Struct Definitions ---

typedef struct {
    char key[NR_MAX_QUERY_KEY_LEN + 1];
    char value[NR_MAX_QUERY_VALUE_LEN + 1];
    bool is_present;
} nr_key_value_item_t;

typedef struct {
    char key[NR_MAX_CONDITION_KEY_LEN + 1];
    char value[NR_MAX_CONDITION_VALUE_LEN + 1];
    bool is_present;
} nr_condition_item_t;

typedef struct {
    char from_route[NR_MAX_ROUTE_LEN + 1];
    char to_route[NR_MAX_ROUTE_LEN + 1];
    uint16_t status_code;
    bool force;

    nr_key_value_item_t query_params[NR_MAX_QUERY_ITEMS];
    uint8_t num_query_params;

    nr_condition_item_t conditions[NR_MAX_CONDITION_ITEMS];
    uint8_t num_conditions;
} redirect_rule_t;

// --- Enums and Callbacks for Rule Processing ---
typedef enum {
    NR_REDIRECT_PART_FROM_ROUTE,
    NR_REDIRECT_PART_TO_ROUTE,
    NR_REDIRECT_PART_STATUS,
    NR_REDIRECT_PART_FORCE,
    NR_REDIRECT_PART_QUERY,     // For query parameters like 'id=:id'
    NR_REDIRECT_PART_CONDITION, // For conditions like 'Country=au,nz' or 'Language=he'
    NR_REDIRECT_PART_UNKNOWN
} nr_redirect_part_type_t;

typedef void (*nr_redirect_rule_part_callback_t)(
    const char *token,
    size_t token_len,
    nr_redirect_part_type_t part_type,
    void *user_data
);

// --- Core Rule Processing Function ---
void nr_process_redirect_rule(
    const char *rule_line,
    size_t rule_line_len,
    nr_redirect_rule_part_callback_t callback,
    void *user_data
);

// --- Function Signatures for Parsing into Struct ---

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
);

/**
 * @brief Parses a single redirect rule line into a redirect_rule_t structure.
 *
 * This function takes a raw rule line string, processes it using nr_process_redirect_rule,
 * and populates the provided redirect_rule_t structure.
 *
 * @param rule_line The null-terminated string containing the redirect rule.
 * @param rule_line_len The length of the rule_line string.
 * @param rule A pointer to the redirect_rule_t structure to fill.
 * @return true if the rule was successfully parsed, false otherwise.
 */
bool nr_parse_redirect_rule(
    const char *rule_line,
    size_t rule_line_len,
    redirect_rule_t *rule
);

#endif // NANOROUTER_RULE_PARSER_H
