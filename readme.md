# NanoRouter Middleware

![nanorouter.png](nanorouter.png)

[![Status](https://github.com/drorgl/NanoRouter/actions/workflows/platformio-test.yml/badge.svg)](https://github.com/drorgl/NanoRouter/actions/workflows/platformio-test.yml)

This project implements a generic web routing middleware inspired by Netlify's `_headers` and `_redirects` [configuration files](https://docs.netlify.com/manage/routing/overview/). It provides a flexible way to manage HTTP headers and URL redirects/rewrites for web applications.

This project was vibe-coded using cline / gemini as an experiment.

## Overview

The NanoRouter middleware allows you to define custom HTTP headers and powerful routing rules (redirects, rewrites, and proxies) using simple configuration files. These rules are processed by the middleware to control traffic flow and enhance content delivery within a web server environment.

This implementation is based on the concepts and syntax introduced by Netlify's documentation for their static routing features. Credit to Netlify for the clear and comprehensive approach to managing web routing.

## Documentation Structure

This documentation is divided into the following sections:

*   `readme.md`: This overview document.
*   [docs/headers.md](docs/headers.md): Detailed documentation on configuring custom HTTP headers.
*   [docs/redirects.md](docs/redirects.md): Detailed documentation on configuring URL redirects, rewrites, and proxies.
*   [lib/nanorouter/readme.md](lib/nanorouter/readme.md): Library usage guide

## Getting Started

To utilize the routing features, you will typically place `_headers` and `_redirects` files in the root of your web application's publish directory. The NanoRouter middleware will then parse these files and apply the defined rules to incoming requests.

For detailed syntax and examples, please refer to [docs/headers.md](docs/headers.md) and [docs/redirects.md](docs/redirects.md).
