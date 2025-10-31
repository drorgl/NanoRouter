#ifndef NANOROUTER_REDIRECT_MIDDLEWARE_H
#define NANOROUTER_REDIRECT_MIDDLEWARE_H

#include <stdbool.h>
#include <stddef.h>
#include "nanorouter_redirect_rule_parser.h" // For redirect_rule_t
#include "nanorouter_condition_matching.h" // For nanorouter_request_context_t

#include "nanorouter_config.h" // For configuration defines

// --- Struct Definitions ---

/**
 * @brief Structure to hold the response context after processing a redirect request.
 */
typedef struct {
    char new_url[NR_REDIRECT_MAX_URL_LEN + 1]; /**< The new URL if a redirect/rewrite/proxy occurs. Null-terminated. */
    int status_code;                           /**< The HTTP status code to be used. 0 if no redirect. */
} nanorouter_redirect_response_t;

/**
 * @brief Node structure for the linked list of redirect rules.
 */
typedef struct nanorouter_redirect_rule_t {
    redirect_rule_t rule;                          /**< The actual redirect rule data. */
    struct nanorouter_redirect_rule_t *next;       /**< Pointer to the next rule in the list. */
} nanorouter_redirect_rule_t;

/**
 * @brief Structure to manage a linked list of redirect rules.
 */
typedef struct {
    nanorouter_redirect_rule_t *head;              /**< Pointer to the first rule in the list. */
    size_t count;                                  /**< Number of rules in the list. */
} nanorouter_redirect_rule_list_t;

// --- Function Prototypes for Rule List Management ---

/**
 * @brief Creates and initializes an empty nanorouter_redirect_rule_list_t.
 *
 * @return A pointer to the newly created list, or NULL if memory allocation fails.
 */
nanorouter_redirect_rule_list_t* nanorouter_redirect_rule_list_create();

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
bool nanorouter_redirect_rule_list_add_rule(nanorouter_redirect_rule_list_t *list, const redirect_rule_t *rule_data);

/**
 * @brief Frees all memory associated with the redirect rule list and its contained nodes.
 *
 * @param list A pointer to the nanorouter_redirect_rule_list_t to be freed.
 */
void nanorouter_redirect_rule_list_free(nanorouter_redirect_rule_list_t *list);

// --- Function Prototype for Middleware ---

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
);

#endif // NANOROUTER_REDIRECT_MIDDLEWARE_H
