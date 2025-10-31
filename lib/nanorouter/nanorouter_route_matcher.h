#ifndef NANOROUTER_MATCHER_H
#define NANOROUTER_MATCHER_H

#include <stdbool.h> // For bool type
#include <stdint.h>  // For uint8_t
#include <stddef.h>  // For size_t

#include "nanorouter_redirect_rule_parser.h" // For redirect_rule_t and nr_key_value_item_t

// --- Item Count Defines for Matcher ---
#define NR_MAX_MATCHED_PARAMS       10
#define NR_MAX_MATCHED_KEY_LEN      32
#define NR_MAX_MATCHED_VALUE_LEN    128

// --- Struct Definitions for Matcher ---

/**
 * @brief Represents a single captured parameter (placeholder or query parameter).
 */
typedef struct {
    char key[NR_MAX_MATCHED_KEY_LEN + 1];
    char value[NR_MAX_MATCHED_VALUE_LEN + 1];
} nr_matched_param_t;

/**
 * @brief Holds all captured parameters during a rule match.
 */
typedef struct {
    nr_matched_param_t params[NR_MAX_MATCHED_PARAMS];
    uint8_t num_params;
} nr_matched_params_t;

// --- Function Signatures for Matcher ---

/**
 * @brief Matches a URL path against a rule's 'from_route' pattern, capturing placeholders.
 *
 * This function compares the provided URL path against the `from_route` pattern
 * from a redirect rule. It supports wildcards (`*`) and placeholders (`:placeholder`).
 * If a match is found, it captures any placeholder values into the `matched_params` structure.
 *
 * @param url_path The URL path to match (e.g., "/news/02/12/my-story").
 * @param from_route_pattern The pattern from the redirect rule (e.g., "/news/:month/:date/:slug").
 * @param matched_params A pointer to `nr_matched_params_t` to store captured placeholder values.
 * @return true if the URL path matches the pattern, false otherwise.
 */
bool nr_match_path_pattern(
    const char *url_path,
    const char *from_route_pattern,
    nr_matched_params_t *matched_params
);

/**
 * @brief Matches a URL's query string against a rule's query parameters, capturing values.
 *
 * This function compares the provided URL query string against the `query_params`
 * defined in a redirect rule. It checks for presence and specific values, and
 * captures values for parameters like 'id=:id'.
 *
 * @param url_query The URL query string to match (e.g., "id=123&tag=test").
 * @param rule_query_params An array of `nr_key_value_item_t` from the rule.
 * @param num_rule_query_params The number of query parameters in the rule.
 * @param matched_params A pointer to `nr_matched_params_t` to store captured query values.
 * @return true if the URL query string matches the rule's query parameters, false otherwise.
 */
bool nr_match_query_params(
    const char *url_query,
    const nr_key_value_item_t *rule_query_params,
    uint8_t num_rule_query_params,
    nr_matched_params_t *matched_params
);

/**
 * @brief Matches an incoming URL against a redirect rule.
 *
 * This is the main matcher function. It takes a redirect rule and a full URL,
 * and determines if the rule applies to the URL. It handles URL normalization,
 * path matching, and query parameter matching.
 *
 * @param rule A pointer to the `redirect_rule_t` to match against.
 * @param url The full URL string (e.g., "https://example.com/news/02/12/my-story?id=123").
 * @param matched_params A pointer to `nr_matched_params_t` to store all captured values (placeholders and query params).
 * @return true if the rule matches the URL, false otherwise.
 */
bool nanorouter_match_rule(
    const redirect_rule_t *rule,
    const char *url,
    nr_matched_params_t *matched_params
);

#endif // NANOROUTER_MATCHER_H
