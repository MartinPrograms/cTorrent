// Chat gpt generated tests because im lazy

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Bencode.h"
#include "CoreFile.h"

#define TEST_FILENAME      "temp_bencode_test.txt"
#define BUFFER_SIZE        1024

// Helper: read entire file into a buffer and return the number of bytes read
static size_t read_file_into_buffer(const char *filename, char *buffer, size_t buf_size) {
    CoreFile *cf = CoreFile_open(filename, "rb");
    ck_assert_ptr_nonnull(cf);

    uint64_t file_size = CoreFile_get_size(cf);
    ck_assert_uint_le(file_size, buf_size - 1);

    memset(buffer, 0, buf_size);
    int read_count = CoreFile_read(cf, buffer, (size_t)file_size);
    ck_assert_int_eq(read_count, (int)file_size);

    CoreFile_close(cf);
    return (size_t)file_size;
}

// Clean up temporary file if it still exists
static void remove_test_file(void) {
    if (CoreFile_exists(TEST_FILENAME)) {
        CoreFile *cf = CoreFile_open(TEST_FILENAME, "rb");
        CoreFile_delete(cf);
    }
}

// TEST 1: Parse a simple integer “i42e”
START_TEST(test_parse_integer)
{
    const char *bstr = "i42e";
    BencodeItem *item = BencodeItem_parse(bstr, strlen(bstr));
    ck_assert_ptr_nonnull(item);
    ck_assert_int_eq(item->type, BENCODE_TYPE_INTEGER);
    ck_assert_int_eq(item->value.integer, 42);
    BencodeItem_destroy(item);
}
END_TEST

// TEST 2: Parse a simple string “4:test”
START_TEST(test_parse_string)
{
    const char *bstr = "4:test";
    BencodeItem *item = BencodeItem_parse(bstr, strlen(bstr));
    ck_assert_ptr_nonnull(item);
    ck_assert_int_eq(item->type, BENCODE_TYPE_STRING);

    CoreString *cs = item->value.string;
    ck_assert_ptr_nonnull(cs);
    // Compare length and content
    size_t len = cs->length;
    ck_assert_uint_eq(len, 4);
    ck_assert_str_eq(cs->str, "test");

    BencodeItem_destroy(item);
}
END_TEST

// TEST 3: Parse a list of two strings: “l4:spam4:eggse”
START_TEST(test_parse_list)
{
    const char *bstr = "l4:spam4:eggse";
    BencodeItem *item = BencodeItem_parse(bstr, strlen(bstr));
    ck_assert_ptr_nonnull(item);
    ck_assert_int_eq(item->type, BENCODE_TYPE_LIST);

    BencodeList *lst = item->value.list;
    ck_assert_ptr_nonnull(lst);
    ck_assert_uint_eq(lst->count, 2);

    // First element: “spam”
    BencodeItem *first = &lst->items[0];
    ck_assert_int_eq(first->type, BENCODE_TYPE_STRING);
    ck_assert_str_eq(first->value.string->str, "spam");

    // Second element: “eggs”
    BencodeItem *second = &lst->items[1];
    ck_assert_int_eq(second->type, BENCODE_TYPE_STRING);
    ck_assert_str_eq(second->value.string->str, "eggs");
}
END_TEST

// TEST 4: Parse a dictionary { "cow": "moo", "spam": "eggs" }
// Bencoded as: “d3:cow3:moo4:spam4:eggse”
START_TEST(test_parse_dictionary)
{
    const char *bstr = "d3:cow3:moo4:spam4:eggse";
    BencodeItem *item = BencodeItem_parse(bstr, strlen(bstr));
    ck_assert_ptr_nonnull(item);
    ck_assert_int_eq(item->type, BENCODE_TYPE_DICTIONARY);

    BencodeDictionary *dict = item->value.dictionary;
    ck_assert_ptr_nonnull(dict);
    ck_assert_uint_eq(dict->count, 2);

    // We don’t know insertion order, so search for each key
    bool found_cow = false, found_spam = false;
    for (size_t i = 0; i < dict->count; i++) {
        CoreString *key_cs = dict->keys[i];
        const char *key = key_cs->str;

        if (strcmp(key, "cow") == 0) {
            BencodeItem *val_item = dict->values[i];
            ck_assert_int_eq(val_item->type, BENCODE_TYPE_STRING);
            ck_assert_str_eq(val_item->value.string->str, "moo");
            found_cow = true;
        }
        else if (strcmp(key, "spam") == 0) {
            BencodeItem *val_item = dict->values[i];
            ck_assert_int_eq(val_item->type, BENCODE_TYPE_STRING);
            ck_assert_str_eq(val_item->value.string->str, "eggs");
            found_spam = true;
        }
    }
    ck_assert_msg(found_cow,  "Key “cow” not found in parsed dictionary");
    ck_assert_msg(found_spam, "Key “spam” not found in parsed dictionary");

    BencodeItem_destroy(item);
}
END_TEST

// TEST 5: Save‐and‐re‐parse round-trip for nested structure:
//   { "numbers": [ i1e, i2e ], "msg": "hi" }
// Bencode‐encoded (keys in lex order):
//   d4:msg2:hi7:numbersli1ei2eee
START_TEST(test_save_and_parse_roundtrip)
{
    // Step 1: construct the BencodeItem manually

    // Create the outer dictionary
    BencodeItem *root = BencodeItem_create(BENCODE_TYPE_DICTIONARY);
    ck_assert_ptr_nonnull(root);

    // Prepare a CoreDictionary builder
    CoreDictionary *dict_builder = CoreDictionary_create(2);
    ck_assert_ptr_nonnull(dict_builder);

    // 1a: “msg” → “hi”
    CoreString *key_msg = CoreString_create("msg");
    ck_assert_ptr_nonnull(key_msg);

    BencodeItem *val_msg = BencodeItem_create(BENCODE_TYPE_STRING);
    ck_assert_ptr_nonnull(val_msg);
    val_msg->value.string = CoreString_create("hi");
    ck_assert_ptr_nonnull(val_msg->value.string);

    // 1b: “numbers” → a list [ 1, 2 ]
    CoreString *key_num = CoreString_create("numbers");
    ck_assert_ptr_nonnull(key_num);

    BencodeItem *list_item = BencodeItem_create(BENCODE_TYPE_LIST);
    ck_assert_ptr_nonnull(list_item);

    // Build a BencodeList struct
    BencodeList *bl = (BencodeList *)malloc(sizeof(BencodeList));
    ck_assert_ptr_nonnull(bl);
    bl->count = 2;
    bl->items = (BencodeItem *)malloc(2 * sizeof(BencodeItem));
    ck_assert_ptr_nonnull(bl->items);

    // First element: i1e
    BencodeItem *i1 = BencodeItem_create(BENCODE_TYPE_INTEGER);
    ck_assert_ptr_nonnull(i1);
    i1->value.integer = 1;
    memcpy(&bl->items[0], i1, sizeof(BencodeItem));
    free(i1);

    // Second element: i2e
    BencodeItem *i2 = BencodeItem_create(BENCODE_TYPE_INTEGER);
    ck_assert_ptr_nonnull(i2);
    i2->value.integer = 2;
    memcpy(&bl->items[1], i2, sizeof(BencodeItem));
    free(i2);

    list_item->value.list = bl;

    // Insert into CoreDictionary builder
    void *old1 = CoreDictionary_put(dict_builder, key_msg->str, val_msg);
    ck_assert_ptr_null(old1);
    CoreString_destroy(key_msg);

    void *old2 = CoreDictionary_put(dict_builder, key_num->str, list_item);
    ck_assert_ptr_null(old2);
    CoreString_destroy(key_num);

    // Finalize BencodeDictionary in root
    BencodeDictionary *bd = (BencodeDictionary *)malloc(sizeof(BencodeDictionary));
    ck_assert_ptr_nonnull(bd);

    size_t n = CoreDictionary_size(dict_builder);
    bd->count  = n;
    bd->keys   = (CoreString **)malloc(n * sizeof(CoreString *));
    bd->values = (BencodeItem **)malloc(n * sizeof(BencodeItem *));
    ck_assert_ptr_nonnull(bd->keys);
    ck_assert_ptr_nonnull(bd->values);

    // Copy from CoreDictionary internals
    // (Assuming CoreDictionary stores keys in dict_builder->keys[] and values in dict_builder->values[])
    // We rely on the fact that CoreDictionary stores keys as strdup’d C strings.
    for (size_t i = 0; i < n; i++) {
        char **all_keys   = (char **)dict_builder->keys;
        void  **all_vals  = (void  **)dict_builder->values;

        bd->keys[i]   = CoreString_create_with_length(all_keys[i], strlen(all_keys[i]));
        bd->values[i] = (BencodeItem *)all_vals[i];
    }

    root->value.dictionary = bd;
    CoreDictionary_destroy(dict_builder);

    // Step 2: save the root to a file
    CoreFile *cf = CoreFile_create(TEST_FILENAME, "wb+");
    ck_assert_ptr_nonnull(cf);

    bool saved = BencodeItem_save(root, cf);
    ck_assert(saved == true);
    CoreFile_flush(cf);
    CoreFile_close(cf);

    // Step 3: read back the saved bytes
    char buffer[BUFFER_SIZE] = {0};
    size_t len = read_file_into_buffer(TEST_FILENAME, buffer, BUFFER_SIZE);

    // The expected Bencode string is (keys sorted lex):
    //   d4:msg2:hi7:numbersli1ei2eee
    const char *expected = "d3:msg2:hi7:numbersli1ei2eee";
    ck_assert_uint_eq(len, strlen(expected));
    ck_assert_str_eq(buffer, expected);

    // Step 4: parse what we just saved, and verify the same structure
    BencodeItem *parsed = BencodeItem_parse(buffer, len);
    ck_assert_ptr_nonnull(parsed);
    ck_assert_int_eq(parsed->type, BENCODE_TYPE_DICTIONARY);

    BencodeDictionary *pdict = parsed->value.dictionary;
    ck_assert_uint_eq(pdict->count, 2);

    // Check “msg” → “hi”
    bool found_msg = false, found_nums = false;
    for (size_t i = 0; i < pdict->count; i++) {
        const char *k = pdict->keys[i]->str;
        if (strcmp(k, "msg") == 0) {
            BencodeItem *vi = pdict->values[i];
            ck_assert_int_eq(vi->type, BENCODE_TYPE_STRING);
            ck_assert_str_eq((vi->value.string->str), "hi");
            found_msg = true;
        }
        else if (strcmp(k, "numbers") == 0) {
            BencodeItem *vi = pdict->values[i];
            ck_assert_int_eq(vi->type, BENCODE_TYPE_LIST);
            BencodeList *pl = vi->value.list;
            ck_assert_uint_eq(pl->count, 2);

            // Check each integer
            BencodeItem *elem0 = &pl->items[0];
            BencodeItem *elem1 = &pl->items[1];
            ck_assert_int_eq(elem0->type, BENCODE_TYPE_INTEGER);
            ck_assert_int_eq(elem1->type, BENCODE_TYPE_INTEGER);
            ck_assert_int_eq(elem0->value.integer, 1);
            ck_assert_int_eq(elem1->value.integer, 2);
            found_nums = true;
        }
    }
    ck_assert_msg(found_msg,  "Key “msg” not found after re‐parsing");
    ck_assert_msg(found_nums, "Key “numbers” not found or incorrect after re‐parsing");

    remove_test_file();
}
END_TEST

// Build the test suite
Suite *bencode_suite(void) {
    Suite *s = suite_create("Bencode");
    TCase *tc = tcase_create("BencodeTests");

    tcase_add_test(tc, test_parse_integer);
    tcase_add_test(tc, test_parse_string);
    tcase_add_test(tc, test_parse_list);
    tcase_add_test(tc, test_parse_dictionary);
    tcase_add_test(tc, test_save_and_parse_roundtrip);

    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int failed;
    Suite *s = bencode_suite();
    SRunner *runner = srunner_create(s);
    srunner_run_all(runner, CK_NORMAL);
    failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    // Clean up any leftover test file
    remove_test_file();
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
