#ifndef NANOROUTER_HEADER_RULE_PARSER_H
#define NANOROUTER_HEADER_RULE_PARSER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "nanorouter_config.h" // For configuration defines

// --- Struct Definitions ---

/**
 * @brief Represents a single HTTP header key-value pair.
 */
typedef struct {
    char key[NR_MAX_HEADER_KEY_LEN + 1];
    char value[NR_MAX_HEADER_VALUE_LEN + 1];
} nanorouter_header_entry_t;

/**
 * @brief Represents a single header rule, including the path to match and associated headers.
 */
typedef struct {
    char from_route[NR_MAX_ROUTE_LEN + 1]; /**< The URL path pattern to match. */
    nanorouter_header_entry_t headers[NR_MAX_HEADERS_PER_RULE]; /**< Array of headers to apply. */
    uint8_t num_headers;                                        /**< Number of headers in the array. */
} header_rule_t;

/**
 * @brief Node structure for the linked list of header rules.
 */
typedef struct nanorouter_header_rule_node_t {
    header_rule_t rule;                                /**< The actual header rule data. */
    struct nanorouter_header_rule_node_t *next;        /**< Pointer to the next rule in the list. */
} nanorouter_header_rule_node_t;

/**
 * @brief Structure to manage a linked list of header rules.
 */
typedef struct {
    nanorouter_header_rule_node_t *head;               /**< Pointer to the first rule in the list. */
    size_t count;                                      /**< Number of rules in the list. */
} nanorouter_header_rule_list_t;

// --- Function Prototypes for Rule List Management ---

/**
 * @brief Creates and initializes an empty nanorouter_header_rule_list_t.
 *
 * @return A pointer to the newly created list, or NULL if memory allocation fails.
 */
nanorouter_header_rule_list_t* nanorouter_header_rule_list_create();

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
bool nanorouter_header_rule_list_add_rule(nanorouter_header_rule_list_t *list, const header_rule_t *rule_data);

/**
 * @brief Frees all memory associated with the header rule list and its contained nodes.
 *
 * @param list A pointer to the nanorouter_header_rule_list_t to be freed.
 */
void nanorouter_header_rule_list_free(nanorouter_header_rule_list_t *list);


// --- Function Prototypes for Header Rule Parsing ---

/**
 * @brief Parses a _headers file content and populates a list of header_rule_t.
 *
 * @param file_content The content of the _headers file as a string.
 * @param rule_list A pointer to the nanorouter_header_rule_list_t to populate.
 * @return true if parsing was successful, false otherwise.
 */
bool nanorouter_parse_headers_file(const char *file_content, nanorouter_header_rule_list_t *rule_list);

#endif // NANOROUTER_HEADER_RULE_PARSER_H
