# Custom Headers for NanoRouter Middleware

This document details how to configure custom HTTP headers for the NanoRouter middleware. This functionality is inspired by Netlify's `_headers` file configuration.

## Overview

Custom headers allow you to add or modify the default HTTP headers that the NanoRouter middleware processes for your web application's responses when a client makes a request. This can be used for various purposes, such as security policies, caching control, or content negotiation.

## Configuration

You can configure custom headers by creating a plain text file named `_headers` in the publish directory of your web application. The middleware will automatically detect and apply the rules defined in this file.

## Syntax for the `_headers` file

In the `_headers` file, you specify one or more URL paths, with their additional headers indented below them.

*   Any line beginning with `#` will be ignored as a comment.
*   Header field names are case-insensitive.
*   Paths can contain wildcards and placeholders.

### Basic Example

To set the `X-Frame-Options` header for all pages on your site:

```
/*
  X-Frame-Options: DENY
```

### Path-Specific Headers

To apply different headers to specific paths:

```
# Headers for /templates/index.html
/templates/index.html
  X-Frame-Options: DENY

# Headers for /templates/index2.html
/templates/index2.html
  X-Frame-Options: SAMEORIGIN
```

### Wildcards and Placeholders in Paths

You can use wildcards (`*`) and placeholders (`:placeholder`) in URL path segments:

*   **Wildcards (`*`)**: Can be used at any place inside of a path segment to match any character.
*   **Placeholders (`:placeholder`)**: Can only be used at the start of a path segment to match any character except `/`.
*   **Limitation**: Wildcards and placeholders cannot be within the same path segment (e.g., `/templates/:placeholder*` is not supported).

### Multi-value Headers

Some header fields can accept multiple values. You can configure multi-value headers by listing multiple headers with the same field name. The NanoRouter Web Server will concatenate the values into a single header, following HTTP 1.1 specifications.

Example:

```
/*
  cache-control: max-age=0
  cache-control: no-cache
  cache-control: no-store
  cache-control: must-revalidate
```

This will result in a single `cache-control` header:

`cache-control: max-age=0,no-cache,no-store,must-revalidate`

## Limitations

*   Custom headers apply only to files served directly by the web server utilizing NanoRouter. If content is proxied from another service, that service should return any required headers.
*   Certain HTTP header names are typically ignored as they are set by the web server for proper operation. These headers are usually managed by the underlying web server or network stack to ensure proper functionality. The NanoRouter middleware will not override these headers if they are present in the `_headers` file.

    The following header names are typically ignored:
    *   `Accept-Ranges`
    *   `Age`
    *   `Allow`
    *   `Alt-Svc`
    *   `Connection`
    *   `Content-Encoding`
    *   `Content-Length`
    *   `Content-Range`
    *   `Date`
    *   `Server`
    *   `Set-Cookie` (may be overridden by the web server's cookie handling)
    *   `Trailer`
    *   `Transfer-Encoding`
    *   `Upgrade`

    For embedded developers, this list of ignored headers would typically be configured within the library's source code, for example, as a preprocessor `#define` or a C array of strings, allowing for customization during compilation.

*   `Location` headers should be managed using redirects, not custom headers.

---
*Inspired by Netlify's Custom Headers documentation.*
