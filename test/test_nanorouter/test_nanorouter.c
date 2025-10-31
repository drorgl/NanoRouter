#include "unity.h"
#include "nanorouter.h"
#include "test_nanorouter_redirect_rule_parser.h" // Include the new test header
#include "test_string_utils.h" // Include the new string utils test header
#include "test_matcher.h" // Include the new matcher test header
#include "test_nanorouter_redirect_middleware.h" // Include the new redirect middleware test header
#include "test_nanorouter_header_rule_parser.h" // Include the new header rule parser test header
#include "test_nanorouter_headers_middleware.h" // Include the new headers middleware test header
#include "test_nanorouter_condition_matching.h"
#include <string.h> // For strncpy
#include <stdlib.h> // For free
#include <stdbool.h> // For bool type

void setUp(void) {}
void tearDown(void) {}






int main(void) {
    UNITY_BEGIN();
    test_string_utils(); // Run the string utils tests

    UNITY_END(); // End the first set of tests

    test_rule_parser(); // Run the rule parser tests
    test_rule_parser_redirect_rules(); // Run the redirect rule parser tests
    test_matcher(); // Run the matcher tests
    test_nanorouter_redirect_middleware(); // Run the redirect middleware tests
    test_header_rule_parser(); // Run the header rule parser tests
    test_nanorouter_headers_middleware(); // Run the headers middleware tests
    test_nanorouter_condition_matching();

    return 0; // Return 0 for successful execution
}

void app_main() {
    main();
}
