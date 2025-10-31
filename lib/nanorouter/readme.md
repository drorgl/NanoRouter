# NanoRouter Middleware

A lightweight and efficient web server middleware for handling custom HTTP headers, redirects, rewrites, and proxies in embedded systems. NanoRouter provides Netlify-style declarative configuration for web servers, optimized for resource-constrained environments like ESP32.

## Features

- **Custom HTTP Headers**: Add or modify headers via `_headers` file configuration
- **Redirects & Rewrites**: Handle permanent (301), temporary (302), and internal rewrites (200) 
- **API Proxies**: Forward requests to external services
- **Custom 404 Pages**: Route to specific pages for different paths
- **Path Matching**: Support for wildcards (`*`) and placeholders (`:placeholder`)
- **Query Parameters**: Conditional routing based on URL parameters
- **GeoIP & Language**: Country and language-based redirects
- **Force Rules**: Override existing static files with `!` flag
- **Embedded Optimized**: Designed for ESP32 and similar constrained environments

## Installation

Copy the `lib/nanorouter/` directory to your project and include the header:

```c
#include "nanorouter.h"
```

Or include individual middleware components:

```c
#include "nanorouter_headers_middleware.h"
#include "nanorouter_redirect_middleware.h"
```

## Architecture

NanoRouter consists of two main middleware components:

1. **Headers Middleware** - Manages HTTP response headers
2. **Redirect Middleware** - Handles URL routing and redirects

### System Flow

```
HTTP Request → NanoRouter → Rule Matching → Action (Headers/Redirect/Proxy) → Response
```

## Configuration Files

### `_headers` File

Define custom HTTP headers for specific URL paths:

```c
/*
  X-Frame-Options: DENY
  Content-Security-Policy: default-src 'self'

/api/*
  Access-Control-Allow-Origin: *
  Access-Control-Allow-Methods: GET, POST, PUT, DELETE

/templates/index.html
  X-Frame-Options: SAMEORIGIN
```

### `_redirects` File

Define redirect, rewrite, and proxy rules:

```c
# Basic redirects
/home    /blog/my-post          301
/news    /blog/cuties           302

# Wildcards and placeholders
/news/*  /blog/:splat           301
/news/:month/:date/:year/:slug  /blog/:year/:month/:date/:slug  301

# Rewrites (status 200)
/app/*   /index.html            200
/api/*   https://api.example.com/:splat  200

# Custom 404 pages
/*       /404.html              404

# Query parameter matching
/store   id=:id    /blog/:id    301

# Country/language conditions
/        /anz     302  Country=au,nz
/israel/* /israel/he/:splat  302  Language=he
```

## API Reference

### Headers Middleware

#### Core Functions

```c
/**
 * @brief Process header request and populate response headers
 * @param request_url The incoming URL path
 * @param rules List of loaded header rules
 * @param response_context Output: headers to apply
 * @param request_context Request context for conditions
 * @return true if headers were applied, false otherwise
 */
bool nanorouter_process_header_request(
    const char *request_url,
    nanorouter_header_rule_list_t *rules,
    nanorouter_header_response_t *response_context,
    const nanorouter_request_context_t *request_context
);
```

#### Rule Management

```c
/**
 * @brief Create empty header rule list
 * @return Created list or NULL on failure
 */
nanorouter_header_rule_list_t* nanorouter_header_rule_list_create();

/**
 * @brief Add rule to header rule list
 * @param list Rule list
 * @param rule_data Rule data to add
 * @return true on success, false otherwise
 */
bool nanorouter_header_rule_list_add_rule(
    nanorouter_header_rule_list_t *list, 
    const header_rule_t *rule_data
);

/**
 * @brief Free header rule list and all contained rules
 * @param list Rule list to free
 */
void nanorouter_header_rule_list_free(nanorouter_header_rule_list_t *list);
```

#### Parsing

```c
/**
 * @brief Parse _headers file content into rule list
 * @param file_content Content of _headers file
 * @param rule_list Output: parsed rules
 * @return true on successful parsing
 */
bool nanorouter_parse_headers_file(
    const char *file_content, 
    nanorouter_header_rule_list_t *rule_list
);
```

### Redirect Middleware

#### Core Functions

```c
/**
 * @brief Process redirect request and populate response
 * @param request_url The incoming URL path
 * @param rules List of loaded redirect rules
 * @param response_context Output: redirect information
 * @param request_context Request context for conditions
 * @return true if redirect applied, false otherwise
 */
bool nanorouter_process_redirect_request(
    const char *request_url,
    nanorouter_redirect_rule_list_t *rules,
    nanorouter_redirect_response_t *response_context,
    const nanorouter_request_context_t *request_context
);
```

#### Rule Management

```c
/**
 * @brief Create empty redirect rule list
 * @return Created list or NULL on failure
 */
nanorouter_redirect_rule_list_t* nanorouter_redirect_rule_list_create();

/**
 * @brief Add rule to redirect rule list
 * @param list Rule list
 * @param rule_data Rule data to add
 * @return true on success, false otherwise
 */
bool nanorouter_redirect_rule_list_add_rule(
    nanorouter_redirect_rule_list_t *list, 
    const redirect_rule_t *rule_data
);

/**
 * @brief Free redirect rule list and all contained rules
 * @param list Rule list to free
 */
void nanorouter_redirect_rule_list_free(nanorouter_redirect_rule_list_t *list);
```

#### Rule Parsing

```c
/**
 * @brief Parse redirect rule line into redirect_rule_t structure
 * @param rule_line Raw rule line string
 * @param rule_line_len Length of rule line
 * @param rule Output: parsed rule data
 * @return true on successful parsing
 */
bool nr_parse_redirect_rule(
    const char *rule_line,
    size_t rule_line_len,
    redirect_rule_t *rule
);
```

### Request Context

```c
typedef struct {
    char domain[NR_MAX_DOMAIN_LEN + 1];     /**< Request domain */
    char country[NR_MAX_COUNTRY_LEN + 1];   /**< Country code from GeoIP */
    char language[NR_MAX_LANGUAGE_LEN + 1]; /**< Language from Accept-Language */
} nanorouter_request_context_t;
```

## Usage Examples

### Basic Integration

```c
#include "nanorouter.h"

// Initialize rule lists
nanorouter_header_rule_list_t *header_rules = nanorouter_header_rule_list_create();
nanorouter_redirect_rule_list_t *redirect_rules = nanorouter_redirect_rule_list_create();

// Load configuration files
char headers_content[] = "/*\n  X-Frame-Options: DENY";
nanorouter_parse_headers_file(headers_content, header_rules);

char redirects_content[] = "/old /new 301";
redirect_rule_t redirect_rule;
nr_parse_redirect_rule(redirects_content, strlen(redirects_content), &redirect_rule);
nanorouter_redirect_rule_list_add_rule(redirect_rules, &redirect_rule);

// Process incoming request
nanorouter_header_response_t header_response = {0};
nanorouter_redirect_response_t redirect_response = {0};
nanorouter_request_context_t request_context = {0};

// Apply headers
bool headers_applied = nanorouter_process_header_request(
    "/some/path", header_rules, &header_response, &request_context
);

// Apply redirects
bool redirect_applied = nanorouter_process_redirect_request(
    "/some/path", redirect_rules, &redirect_response, &request_context
);

// Clean up
nanorouter_header_rule_list_free(header_rules);
nanorouter_redirect_rule_list_free(redirect_rules);
```

### Creating Rules Programmatically

```c
// Create header rule
header_rule_t header_rule = {
    .from_route = "/api/*",
    .headers = {
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE"}
    },
    .num_headers = 2
};
nanorouter_header_rule_list_add_rule(header_rules, &header_rule);

// Create redirect rule
redirect_rule_t redirect_rule = {
    .from_route = "/news/:date/:slug",
    .to_route = "/blog/:date/:slug",
    .status_code = 301,
    .force = false
};
nanorouter_redirect_rule_list_add_rule(redirect_rules, &redirect_rule);
```

### Advanced Query Parameter Matching

```c
redirect_rule_t rule = {
    .from_route = "/store",
    .to_route = "/blog/:id",
    .status_code = 301,
    .force = false
};

// Add query parameter condition
rule.query_params[0].key[0] = 'i';
strcpy(rule.query_params[0].value, ":id");
rule.query_params[0].is_present = true;
rule.num_query_params = 1;

nanorouter_redirect_rule_list_add_rule(redirect_rules, &rule);
```

## Configuration

### Compile-time Settings

Modify `nanorouter_config.h` for your specific requirements:

```c
// Memory limits for embedded systems
#define NR_MAX_DOMAIN_LEN           128    /**< Domain string length */
#define NR_MAX_COUNTRY_LEN          16     /**< Country code length */
#define NR_MAX_LANGUAGE_LEN         32     /**< Language code length */
#define NR_MAX_ROUTE_LEN            128    /**< Route path length */
#define NR_MAX_HEADER_KEY_LEN       64     /**< Header key length */
#define NR_MAX_HEADER_VALUE_LEN     256    /**< Header value length */
#define NR_MAX_HEADERS_PER_RULE     10     /**< Headers per rule */
#define NR_HEADERS_MAX_ENTRIES_PER_RESPONSE 10  /**< Response headers */
#define NR_REDIRECT_MAX_URL_LEN     128    /**< Redirect URL length */
#define NR_MAX_QUERY_ITEMS          10     /**< Query parameters per rule */
#define NR_MAX_CONDITION_ITEMS      10     /**< Conditions per rule */
```

### Memory Optimization for ESP32

For constrained environments, consider:

1. **Reduce buffer sizes** in `nanorouter_config.h`
2. **Use fewer rules** per list
3. **Parse config files once** at startup, not per request
4. **Free unused rule lists** after loading

## Path Matching Rules

### Wildcards (`*`)
- Match any characters within a path segment
- Can only be used at the end of a path segment
- Example: `/api/*` matches `/api/users`, `/api/data/file.json`

### Placeholders (`:placeholder`)
- Match single path segments
- Cannot contain `/` characters
- Example: `/news/:date/:slug` matches `/news/2024/01/15/my-story`

### Splats (`:splat`)
- Available in redirect rules only
- Captures remaining path segments
- Example: `/news/*` → `/blog/:splat` matches `/news/2024/01/15` → `/blog/2024/01/15`

### Ignored Headers

The following headers are ignored by design to prevent conflicts with the web server:

- `Accept-Ranges`, `Age`, `Allow`, `Alt-Svc`
- `Connection`, `Content-Encoding`, `Content-Length`
- `Content-Range`, `Date`, `Server`
- `Set-Cookie`, `Trailer`, `Transfer-Encoding`
- `Upgrade`

### Status Codes

- **301**: Permanent redirect (browser shows new URL)
- **302**: Temporary redirect (browser shows new URL)
- **404**: Not found (browser shows original URL)
- **200**: Internal rewrite/proxy (browser shows original URL)

### Force Mode

Add `!` to status code to force rule execution even if static files exist:

```c
/app/*  /app/index.html  200!  # Force rewrite over static files
```

## Testing

The library includes comprehensive tests in `test/test_nanorouter/`. Run tests with:

```bash
# If using PlatformIO
pio test -f test_nanorouter
```

## Examples

See `docs/samples/` for real-world configuration examples:

- **floatplaneapi**: API proxy configuration
- **Techlore**: Multi-rule header setup
- **hocus-focus**: Complex redirect patterns
- **json-ld.org**: Security headers configuration

## Limitations

- **Embedded Focus**: Designed for constrained environments
- **HTTP Only**: No HTTP/2 or HTTP/3 support
- **Simple Conditions**: Country/language only for routing
- **Memory Constraints**: Limited by compile-time buffer sizes
- **Static Configuration**: Rules loaded at startup, not runtime

## Contributing

1. Follow the existing test patterns (When-Act-Assert)
2. Ensure all functions have proper documentation
3. Test on ESP32 or similar embedded platforms
4. Maintain backward compatibility

## MIT License

Copyright (c) 2025 Dror Gluska

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.