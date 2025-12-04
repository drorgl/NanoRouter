# Redirects and Rewrites for NanoRouter Middleware

This document explains how to configure redirects, rewrites, and proxies for the NanoRouter middleware. This functionality is inspired by Netlify's `_redirects` file configuration.

## Overview

The NanoRouter middleware allows you to define powerful routing rules to manage traffic to and from your web application. These rules can redirect visitors to different paths, rewrite paths internally, or proxy requests to other services, all while maintaining clean URLs and improving user experience within a web server environment.

## Configuration

You can configure redirect and rewrite rules by creating a plain text file named `_redirects` (without a file extension) in the publish directory of your site. Each rule must be listed on a separate line.

## Syntax for the `_redirects` file

In the `_redirects` file, each redirect rule is defined on a separate line, with the original path followed by the new path or URL.

*   Any line beginning with `#` will be ignored as a comment.
*   Paths are case-sensitive, and special characters in paths must be URL-encoded.

### Basic Redirect Example

```
# Redirects from what the browser requests to what we serve
/home                /blog/my-post
/news                /blog/cuties
```

### HTTP Status Codes

You can specify an HTTP status code for any redirect rule. If left unspecified, the default is `301` (permanent redirect).

*   **`301` (default)**: Permanent redirect. The browser address bar will display the new address.
*   **`302`**: Temporary redirect. The browser address bar will display the new address.
*   **`404`**: Not Found. Used to present custom 404 pages. The URL in the browser address bar will not change.
*   **`200`**: OK. Used for rewrites and proxying. The URL in the browser address bar will not change.

**Invalid Status Codes:** If a redirect rule contains an invalid or non-numeric HTTP status code (e.g., `9999999999`, `invalid_status`), the rule will be considered malformed. In such cases, the `nr_parse_redirect_rule` function will return `false`, and the `from_route` and `to_route` fields of the parsed rule will be cleared. This ensures that invalid rules are not partially applied and are effectively ignored by the NanoRouter middleware.

Example with status codes:

```
/old-path         /new-path              301
/temporary-page   /current-page          302
/non-existent     /custom-404.html       404
/app              /index.html            200
/bad-status       /error-page            999  # This rule will be ignored due to invalid status
```

### Custom 404 Page Handling

If you add a `404.html` page to your site, it will be automatically displayed for any failed paths that don't have a specific 404 redirect rule. You can also define path-specific 404 pages:

```
/en/* /en/404.html 404
/de/* /de/404.html 404
```

### Force Redirects

Sometimes a redirect might not occur because a static file matches the URL path. To force a redirect, append an exclamation mark `!` to the status code.

Example:

```
/best-pets/dogs  /best-pets/cats.html 200!
```
This rule will always display the content of `/best-pets/cats.html` for requests to `/best-pets/dogs`, even if `/best-pets/dogs/index.html` exists.

### Splats (`*`)

An asterisk (`*`) indicates a **splat** that will match anything that follows it in a path segment.

Example:

```
/news/*  /blog/:splat
```
This would redirect `/news/2004/01/10/my-story` to `/blog/2004/01/10/my-story`.

**Important Considerations for Splats:**
*   More specific rules should be listed before more general ones, as the server processes the first matching rule.
*   Wildcards (`*`) can only be used at the end of a path segment (e.g., `/jobs/*`, not `/jobs/*.html`).

### Placeholders (`:placeholder`)

You can use placeholders in both the original and target paths to capture and reuse segments of the URL.

Example:

```
/news/:month/:date/:year/:slug  /blog/:year/:month/:date/:slug
```
This would redirect `/news/02/12/2004/my-story` to `/blog/2004/02/12/my-story`. A placeholder matches a path segment from one `/` to the next `/`, or a final path segment including a file extension but excluding a query string.

### Query Parameters

You can use query parameters to control URL matches more finely. The server automatically passes on all query string parameters to destination paths for `200`, `301`, and `302` redirects. You can also define redirects based on specific parameters:

```
/store id=:id  /blog/:id  301
```
This rule matches URLs like `/store?id=my-blog-post` and redirects to `/blog/my-blog-post`. It only matches if the `id` parameter is present and no other parameters are in the URL.

To match multiple query parameters:

```
/articles id=:id tag=:tag /posts/:tag/:id 301
```

### Trailing Slash Behavior

The NanoRouter middleware normalizes URLs by removing trailing slashes before applying redirect rules. This means rules will match paths regardless of whether they contain a trailing slash. You cannot use a redirect rule to add or remove a trailing slash.

### Domain-Level Redirects

You can define rules for specific domain aliases or protocols (HTTP vs. HTTPS).

Example:

```
http://blog.yoursite.com/* https://www.yoursite.com/blog/:splat 301!
https://blog.yoursite.com/* https://www.yoursite.com/blog/:splat 301!
```

### Redirect by Country or Language

For multi-regional or multi-lingual sites, you can redirect visitors based on their GeoIP data (country) or browser's language configuration.

*   `Country`: Accepts [ISO 3166-1 alpha-2 country codes](https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2#Officially_assigned_code_elements).
*   `Language`: Accepts [standard browser language identification codes](https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes).

Example:

```
/  /anz     302  Country=au,nz
/israel/*  /israel/he/:splat  302  Language=he
```

### Rewrites and Proxies (Status Code `200`)

When a redirect rule is assigned an HTTP status code of `200`, it becomes a **rewrite**. The URL in the visitor’s address bar remains the same, while the middleware directs the web server to fetch content from the new location behind the scenes. This is useful for single-page applications, proxying to other services, or internal rewrites.

#### History `pushState` for Single-Page Apps

To enable clean URLs for single-page applications using `pushState`:

```
/*  /index.html  200
```
This serves `index.html` for any URL, preventing 404 errors.

#### Shadowing

By default, a rewrite will not shadow a URL that actually exists. To force a rewrite even if a static file exists at the URL, append an exclamation mark `!` to the status code.

Example:

```
/app/*  /app/index.html  200!
```

#### Proxy to Another Service

You can proxy parts of your site to external services.

Example:

```
/api/*  https://api.example.com/:splat  200
```
Requests to `/api/...` will be proxied to `https://api.example.com` from the web server via the middleware.

---
*Inspired by Netlify's Redirects and Rewrites documentation.*
