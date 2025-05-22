#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CoreFile.h"

#define TEST_FILENAME "temp_test_file.txt"
#define TEST_CONTENT  "Hello, CoreFile!"

static void destroy_corefile(CoreFile *cf) {
    if (!cf) return;
    CoreFile_close(cf);
}

START_TEST(test_corefile_create_and_size)
{
    CoreFile *cf = CoreFile_create(TEST_FILENAME, "wb+");
    ck_assert_ptr_nonnull(cf);

    const char *data = TEST_CONTENT;
    size_t len = strlen(data);
    int written = CoreFile_write(cf, data, len);
    ck_assert_int_eq(written, (int)len);

    CoreFile_flush(cf);

    uint64_t size = CoreFile_get_size(cf);
    ck_assert_uint_eq(size, len);

    destroy_corefile(cf);
    remove(TEST_FILENAME);
}
END_TEST

START_TEST(test_corefile_read_write_chunk)
{
    CoreFile *cf = CoreFile_create(TEST_FILENAME, "wb+");
    ck_assert_ptr_nonnull(cf);

    const char *chunk = "1234567890";
    size_t len = strlen(chunk);
    CoreFile_write_chunk(cf, chunk, len, 5);
    CoreFile_flush(cf);

    char buffer[16] = {0};
    CoreFile_read_chunk(cf, buffer, len, 5);
    ck_assert_str_eq(buffer, chunk);

    destroy_corefile(cf);
    remove(TEST_FILENAME);
}
END_TEST

START_TEST(test_corefile_exists_and_delete)
{
    CoreFile *cf = CoreFile_create(TEST_FILENAME, "wb");
    ck_assert_ptr_nonnull(cf);
    CoreFile_flush(cf);
    CoreFile_close(cf);

    ck_assert(CoreFile_exists(TEST_FILENAME));

    cf = CoreFile_open(TEST_FILENAME, "rb");
    CoreFile_delete(cf);

    ck_assert(CoreFile_exists(TEST_FILENAME) == false);
}
END_TEST

START_TEST(test_corefile_preallocate)
{
    CoreFile *cf = CoreFile_create(TEST_FILENAME, "wb+");
    ck_assert_ptr_nonnull(cf);

    CoreFile_preallocate(cf, 4096);
    CoreFile_flush(cf);
    ck_assert_uint_ge(CoreFile_get_size(cf), 4096);

    destroy_corefile(cf);
    remove(TEST_FILENAME);
}
END_TEST

Suite *corefile_suite(void) {
    Suite *s = suite_create("CoreFile");
    TCase *tc = tcase_create("CoreFileTests");

    tcase_add_test(tc, test_corefile_create_and_size);
    tcase_add_test(tc, test_corefile_read_write_chunk);
    tcase_add_test(tc, test_corefile_exists_and_delete);
    tcase_add_test(tc, test_corefile_preallocate);

    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int failed;
    Suite *s = corefile_suite();
    SRunner *runner = srunner_create(s);
    srunner_run_all(runner, CK_NORMAL);
    failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
