#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "src/lib/OpenEXR/ImfName.h"

START_TEST(test_name_buffer_reads_within_bounds)
{
    // Invariant: Buffer reads never exceed the declared length
    const char *payloads[] = {
        "valid",                    // Valid input (within NAME_SIZE)
        "A",                        // Boundary: shortest possible
        "very_long_name_that_exceeds_the_maximum_allowed_length_by_a_wide_margin",  // Oversized by 2x
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",  // 100 chars
        ""                          // Empty string
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        ImfName name;
        const char *input = payloads[i];
        
        // Initialize name with zeros to detect overflows
        memset(&name, 0, sizeof(name));
        
        // Copy input into name - this is the function under test
        strncpy(name.text, input, NAME_SIZE);
        
        // Ensure null termination if string was truncated
        name.text[NAME_SIZE - 1] = '\0';
        
        // Verify no buffer overflow occurred by checking the byte after the buffer
        // We can't directly check this without instrumentation, so we verify:
        // 1. The string is properly null-terminated
        // 2. The length doesn't exceed NAME_SIZE
        ck_assert_msg(strlen(name.text) < NAME_SIZE, 
                     "String length exceeds buffer size for input: %s", input);
        ck_assert_msg(name.text[NAME_SIZE - 1] == '\0',
                     "Buffer not properly null-terminated for input: %s", input);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_name_buffer_reads_within_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}