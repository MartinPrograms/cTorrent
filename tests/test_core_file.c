#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CoreFile.h"

#define TEST_FILENAME "temp_test_file.txt"
#define TEST_CONTENT  "Hello, CoreFile!"

// Helper to clean up CoreFile*
static void destroy_corefile(CoreFile *cf) {
    if (!cf) return;
    if (cf->file) fclose(cf->file);
    free(cf->name);
    free(cf->path);
    free(cf);
}

// Test opening a real file
START_TEST(test_open_valid_file)
{
    // Create a temp file with known content
    FILE *f = fopen(TEST_FILENAME, "w");
    ck_assert_ptr_nonnull(f);
    fprintf(f, TEST_CONTENT);
    fclose(f);

    // Open via CoreFile_open
    CoreFile *cf = CoreFile_open(TEST_FILENAME, "r");
    ck_assert_ptr_nonnull(cf);

    // name should be the filename only
    ck_assert_str_eq(cf->name, TEST_FILENAME);

    // path should match exactly
    ck_assert_str_eq(cf->path, TEST_FILENAME);

    // size should equal length of TEST_CONTENT
    size_t expected = strlen(TEST_CONTENT);
    ck_assert_uint_eq(cf->size, expected);

    destroy_corefile(cf);
    // remove temp file
    remove(TEST_FILENAME);
}
END_TEST

// Test NULL path or mode
START_TEST(test_open_null_args)
{
    CoreFile *cf1 = CoreFile_open(NULL, "r");
    ck_assert_ptr_null(cf1);

    CoreFile *cf2 = CoreFile_open(TEST_FILENAME, NULL);
    ck_assert_ptr_null(cf2);
}
END_TEST

// Test opening a non‑existent file
START_TEST(test_open_nonexistent_file)
{
    const char *badpath = "this_file_does_not_exist.txt";
    CoreFile *cf = CoreFile_open(badpath, "r");
    ck_assert_ptr_null(cf);
}
END_TEST

Suite *corefile_suite(void) {
    Suite *s = suite_create("CoreFile");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_open_valid_file);
    tcase_add_test(tc_core, test_open_null_args);
    tcase_add_test(tc_core, test_open_nonexistent_file);

    suite_add_tcase(s, tc_core);
    return s;
}

// shitty chatgpt generated test. Is going to be replaced with a real one when i have time

int main(void) {
    int failed;
    Suite *s = corefile_suite();
    SRunner *runner = srunner_create(s);
    srunner_run_all(runner, CK_NORMAL);
    failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
