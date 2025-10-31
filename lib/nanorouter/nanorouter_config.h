#ifndef NANOROUTER_CONFIG_H
#define NANOROUTER_CONFIG_H

/**
 * @brief Maximum length for the domain string in the request context.
 *        Affects the size of the buffer allocated for storing the domain.
 */
#define NR_MAX_DOMAIN_LEN           128

/**
 * @brief Maximum length for the country code string(s) in the request context.
 *        Used for GeoIP-based condition matching. Can handle comma-separated
 *        country codes (e.g., "us,ca").
 */
#define NR_MAX_COUNTRY_LEN          16 

/**
 * @brief Maximum length for the language code string(s) in the request context.
 *        Used for Accept-Language header-based condition matching. Can handle
 *        complex language strings (e.g., "en-US,en;q=0.9").
 */
#define NR_MAX_LANGUAGE_LEN         32

/**
 * @brief Maximum length for HTTP header keys.
 *        Affects the size of the buffer allocated for storing header keys.
 */
#define NR_MAX_HEADER_KEY_LEN       64

/**
 * @brief Maximum length for HTTP header values.
 *        Affects the size of the buffer allocated for storing header values.
 */
#define NR_MAX_HEADER_VALUE_LEN     256

/**
 * @brief Maximum number of headers that can be specified per rule.
 *        Defines the maximum capacity of the headers array within a header rule.
 */
#define NR_MAX_HEADERS_PER_RULE     10

/**
 * @brief Maximum length for route paths used in rules.
 *        This define is re-used for both header and redirect rules.
 */
#define NR_MAX_ROUTE_LEN            128

/**
 * @brief Maximum number of headers that can be included in the response context.
 *        This defines the maximum capacity of the headers array within the
 *        nanorouter_header_response_t structure.
 */
#define NR_HEADERS_MAX_ENTRIES_PER_RESPONSE 10

/**
 * @brief Maximum length for query parameter keys.
 */
#define NR_MAX_QUERY_KEY_LEN        32

/**
 * @brief Maximum length for query parameter values.
 */
#define NR_MAX_QUERY_VALUE_LEN      64

/**
 * @brief Maximum length for condition keys.
 */
#define NR_MAX_CONDITION_KEY_LEN    32

/**
 * @brief Maximum length for condition values.
 */
#define NR_MAX_CONDITION_VALUE_LEN  128

/**
 * @brief Maximum number of query parameters per rule.
 */
#define NR_MAX_QUERY_ITEMS          10

/**
 * @brief Maximum number of conditions per rule.
 */
#define NR_MAX_CONDITION_ITEMS      10

/**
 * @brief Maximum length for the URL in redirect responses.
 */
#define NR_REDIRECT_MAX_URL_LEN     128

#endif // NANOROUTER_CONFIG_H
