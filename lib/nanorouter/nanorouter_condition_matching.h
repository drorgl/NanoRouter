#ifndef NANOROUTER_CONDITION_MATCHING_H
#define NANOROUTER_CONDITION_MATCHING_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "nanorouter_redirect_rule_parser.h" // For nr_condition_item_t

#include "nanorouter_config.h" // For configuration defines

// --- Struct Definitions ---

/**
 * @brief Structure to hold request-specific context for condition matching.
 *        All string fields should be null-terminated.
 */
typedef struct {
    char domain[NR_MAX_DOMAIN_LEN + 1];     /**< The domain of the incoming request. */
    char country[NR_MAX_COUNTRY_LEN + 1];   /**< The country code(s) from GeoIP data. */
    char language[NR_MAX_LANGUAGE_LEN + 1]; /**< The language code(s) from Accept-Language header. */
} nanorouter_request_context_t;

// --- Function Prototypes ---

/**
 * @brief Matches a set of conditions against the provided request context.
 *
 * This function evaluates whether all specified conditions in a redirect rule
 * are met by the current request's context.
 *
 * @param conditions An array of nr_condition_item_t from a redirect rule.
 * @param num_conditions The number of conditions in the array.
 * @param request_context A pointer to the nanorouter_request_context_t containing
 *                        the current request's domain, country, and language.
 *                        If NULL, conditions cannot be met.
 * @return true if all conditions are met or if there are no conditions, false otherwise.
 */
bool nanorouter_match_conditions(
    const nr_condition_item_t *conditions,
    uint8_t num_conditions,
    const nanorouter_request_context_t *request_context
);

#endif // NANOROUTER_CONDITION_MATCHING_H
