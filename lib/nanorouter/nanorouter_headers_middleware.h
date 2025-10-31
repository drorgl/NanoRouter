#ifndef NANOROUTER_HEADERS_MIDDLEWARE_H
#define NANOROUTER_HEADERS_MIDDLEWARE_H

#include <stdbool.h>
#include <stddef.h>
#include "nanorouter_header_rule_parser.h" // For header_rule_t and nanorouter_header_entry_t
#include "nanorouter_condition_matching.h" // For nanorouter_request_context_t

#include "nanorouter_config.h" // For configuration defines

// --- Struct Definitions ---

/**
 * @brief Structure to hold the headers to be applied after processing a request.
 */
typedef struct {
    nanorouter_header_entry_t headers[NR_HEADERS_MAX_ENTRIES_PER_RESPONSE]; /**< Array of headers to apply. */
    uint8_t num_headers;                                                    /**< Number of headers in the array. */
} nanorouter_header_response_t;


// --- Function Prototype for Middleware ---

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
);

#endif // NANOROUTER_HEADERS_MIDDLEWARE_H
