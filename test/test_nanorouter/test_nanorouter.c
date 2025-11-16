#include "unity.h"
#include "nanorouter.h"
#include "test_nanorouter_redirect_rule_parser.h" // Include the new test header
#include "test_string_utils.h" // Include the new string utils test header
#include "test_matcher.h" // Include the new matcher test header
#include "test_nanorouter_redirect_middleware.h" // Include the new redirect middleware test header
#include "test_nanorouter_header_rule_parser.h" // Include the new header rule parser test header
#include "test_nanorouter_headers_middleware.h" // Include the new headers middleware test header
#include "test_nanorouter_condition_matching.h"
#include "test_headers_edge_cases.h"
#include "test_string_utils_edge_cases.h"
#include "test_condition_matching_edge_cases.h"
#include <string.h> // For strncpy
#include <stdlib.h> // For free
#include <stdbool.h> // For bool type

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    // Run all test suites
    return 
        test_string_utils() | // Run the string utils tests
        test_string_utils_edge_cases() | // Run string utils edge case tests
    
        test_rule_parser() | // Run the rule parser tests
        test_rule_parser_redirect_rules() | // Run the redirect rule parser tests
        test_matcher() |  // Run the matcher tests
        test_nanorouter_redirect_middleware() | // Run the redirect middleware tests
        test_header_rule_parser() | // Run the header rule parser tests
        test_headers_edge_cases() | // Run header parsing edge case tests
        test_nanorouter_headers_middleware() | // Run the headers middleware tests
        test_nanorouter_condition_matching() | // Run condition matching tests
        test_condition_matching_edge_cases();// Run condition matching edge case tests
}

void app_main() {
    main();
}
