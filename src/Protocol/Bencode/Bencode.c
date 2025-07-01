#include "Bencode.h"

#include <CommonCrypto/CommonDigest.h>

/// Create a new BencodeItem of the specified type. NULL is returned if memory allocation fails.
/// You MUST allocate your own list or dictionary after creating the item.
BencodeItem * BencodeItem_create(BencodeType type) {
    BencodeItem *item = (BencodeItem *)malloc(sizeof(BencodeItem));
    if (item == NULL) {
        return NULL; // Memory allocation failed
    }

    item->type = type;
    return item;
}

void free_list(BencodeItem* item) {
    BencodeList* list = item->value.list;
    if (list == NULL) {
        return; // Nothing to free
    }
    for (size_t i = 0; i < list->count; i++) {
        BencodeItem_destroy(&list->items[i]);
    }
    free(list->items); // Free the array of items
    free(list); // Free the list structure
    item->value.list = NULL;
    // Done!
}

void free_dict(BencodeItem* item) {
    BencodeDictionary* dict = item->value.dictionary;
    if (dict == NULL) {
        return; // Nothing to free
    }

    for (size_t i = 0; i < dict->count; i++) {
        CoreString_destroy(dict->keys[i]); // Free the key
        BencodeItem_destroy(dict->values[i]); // Free the value
    }
    free(dict->keys); // Free the array of keys
    free(dict->values); // Free the array of values
    free(dict); // Free the dictionary structure
    item->value.dictionary = NULL;
}

/// Destroy (and free) a BencodeItem.
void BencodeItem_destroy(BencodeItem *item) {
    if (item == NULL) {
        return; // Nothing to destroy
    }

    if (item->type == BENCODE_TYPE_LIST) {
        // Go through the list and free them all
        free_list(item);
    }

    if (item->type == BENCODE_TYPE_DICTIONARY) {
        // Go through the dictionary and free them all
        free_dict(item);
    }

    if (item->type == BENCODE_TYPE_STRING) {
        if (item->value.string != NULL) {
            CoreString_destroy(item->value.string);
        }
    }

    free(item); // Free the item itself
    item = NULL; // Set to NULL to avoid dangling pointer
    // Done!
}

static bool parse_integer(const char *data, size_t size, size_t *pos, BencodeItem *item);
static bool parse_string(const char *data, size_t size, size_t *pos, BencodeItem *item);
static BencodeItem *parse_list(const char *data, size_t size, size_t *pos);
static BencodeItem *parse_dictionary(const char *data, size_t size, size_t *pos);

static BencodeItem *parse_next(const char *data, size_t size, size_t *pos) {
    if (*pos >= size) return NULL;

    char c = data[*pos];
    if (c == 'i') {
        BencodeItem *item = BencodeItem_create(BENCODE_TYPE_INTEGER);
        if (!item) return NULL;
        if (!parse_integer(data, size, pos, item)) {
            BencodeItem_destroy(item);
            return NULL;
        }
        return item;
    } else if (c >= '0' && c <= '9') {
        BencodeItem *item = BencodeItem_create(BENCODE_TYPE_STRING);
        if (!item) return NULL;
        if (!parse_string(data, size, pos, item)) {
            BencodeItem_destroy(item);
            return NULL;
        }
        return item;
    } else if (c == 'l') {
        return parse_list(data, size, pos);
    } else if (c == 'd') {
        return parse_dictionary(data, size, pos);
    } else {
        return NULL; // Invalid type
    }
}

static bool parse_integer(const char *data, size_t size, size_t *pos, BencodeItem *item) {
    if (*pos >= size || data[*pos] != 'i') return false;
    (*pos)++; // Skip 'i'

    size_t start = *pos;
    while (*pos < size && data[*pos] != 'e') {
        (*pos)++;
    }
    if (*pos == size || data[*pos] != 'e') return false;

    char buffer[64];
    size_t len = *pos - start;
    if (len >= sizeof(buffer)) return false;
    memcpy(buffer, data + start, len);
    buffer[len] = '\0';

    char *end;
    item->value.integer = strtoll(buffer, &end, 10);
    if (end != buffer + len) return false;

    (*pos)++; // Skip 'e'
    return true;
}

static bool parse_string(const char *data, size_t size, size_t *pos, BencodeItem *item) {
    size_t start = *pos;
    while (*pos < size && data[*pos] != ':') {
        (*pos)++;
    }
    if (*pos == size || data[*pos] != ':') return false;

    char buffer[64];
    size_t len = *pos - start;
    if (len >= sizeof(buffer)) return false;
    memcpy(buffer, data + start, len);
    buffer[len] = '\0';

    char *end;
    long str_len = strtol(buffer, &end, 10);
    if (end != buffer + len || str_len < 0 || *pos + 1 + str_len > size) {
        return false;
    }

    (*pos)++; // Skip ':'
    CoreString *str = CoreString_create_with_length(data + *pos, str_len);
    if (!str) return false;

    item->value.string = str;
    *pos += str_len;
    return true;
}

static BencodeItem *parse_list(const char *data, size_t size, size_t *pos) {
    if (*pos >= size || data[*pos] != 'l') return NULL;
    (*pos)++; // Skip 'l'

    BencodeItem *list_item = BencodeItem_create(BENCODE_TYPE_LIST);
    if (!list_item) return NULL;

    CoreList *temp_list = CoreList_create(10);
    if (!temp_list) {
        BencodeItem_destroy(list_item);
        return NULL;
    }

    while (*pos < size && data[*pos] != 'e') {
        BencodeItem *item = parse_next(data, size, pos);
        if (!item) {
            // Cleanup on error
            for (size_t i = 0; i < CoreList_size(temp_list); i++) {
                BencodeItem_destroy((BencodeItem*)CoreList_get(temp_list, i));
            }
            CoreList_destroy(temp_list);
            BencodeItem_destroy(list_item);
            return NULL;
        }
        CoreList_append(temp_list, item);
    }

    if (*pos >= size || data[*pos] != 'e') {
        // Cleanup if no closing 'e'
        for (size_t i = 0; i < CoreList_size(temp_list); i++) {
            BencodeItem_destroy((BencodeItem*)CoreList_get(temp_list, i));
        }
        CoreList_destroy(temp_list);
        BencodeItem_destroy(list_item);
        return NULL;
    }
    (*pos)++; // Skip 'e'

    // Create final list structure
    size_t count = CoreList_size(temp_list);
    BencodeList *blist = (BencodeList*)malloc(sizeof(BencodeList));
    if (!blist) {
        CoreList_destroy(temp_list);
        BencodeItem_destroy(list_item);
        return NULL;
    }

    blist->count = count;
    blist->items = (BencodeItem*)malloc(count * sizeof(BencodeItem));
    if (!blist->items) {
        free(blist);
        CoreList_destroy(temp_list);
        BencodeItem_destroy(list_item);
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        blist->items[i] = *(BencodeItem*)CoreList_get(temp_list, i);
    }

    list_item->value.list = blist;
    CoreList_destroy(temp_list); // Only destroys container, not items
    return list_item;
}

static BencodeItem *parse_dictionary(const char *data, size_t size, size_t *pos) {
    if (*pos >= size || data[*pos] != 'd') return NULL;
    (*pos)++; // Skip 'd'

    BencodeItem *dict_item = BencodeItem_create(BENCODE_TYPE_DICTIONARY);
    if (!dict_item) return NULL;

    CoreList *keys = CoreList_create(10);
    CoreList *values = CoreList_create(10);
    if (!keys || !values) {
        if (keys) CoreList_destroy(keys);
        if (values) CoreList_destroy(values);
        BencodeItem_destroy(dict_item);
        return NULL;
    }

    while (*pos < size && data[*pos] != 'e') {
        // Parse key (must be string)
        BencodeItem *key_item = parse_next(data, size, pos);
        if (!key_item || key_item->type != BENCODE_TYPE_STRING) {
            BencodeItem_destroy(key_item);
            goto dict_cleanup;
        }

        // Parse value
        BencodeItem *value_item = parse_next(data, size, pos);
        if (!value_item) {
            BencodeItem_destroy(key_item);
            goto dict_cleanup;
        }

        // Transfer ownership
        CoreList_append(keys, key_item->value.string);
        CoreList_append(values, value_item);
        key_item->value.string = NULL; // Prevent double-free
        BencodeItem_destroy(key_item);
    }

    if (*pos >= size || data[*pos] != 'e') {
        goto dict_cleanup;
    }
    (*pos)++; // Skip 'e'

    // Create final dictionary
    size_t count = CoreList_size(keys);
    BencodeDictionary *bdict = (BencodeDictionary*)malloc(sizeof(BencodeDictionary));
    if (!bdict) {
        goto dict_cleanup;
    }

    bdict->count = count;
    bdict->keys = (CoreString**)malloc(count * sizeof(CoreString*));
    bdict->values = (BencodeItem**)malloc(count * sizeof(BencodeItem*));

    if (!bdict->keys || !bdict->values) {
        free(bdict->keys);
        free(bdict->values);
        free(bdict);
        goto dict_cleanup;
    }

    for (size_t i = 0; i < count; i++) {
        bdict->keys[i] = (CoreString*)CoreList_get(keys, i);
        bdict->values[i] = (BencodeItem*)CoreList_get(values, i);
    }

    dict_item->value.dictionary = bdict;
    CoreList_destroy(keys);
    CoreList_destroy(values);
    return dict_item;

dict_cleanup:
    // Cleanup on error
    for (size_t i = 0; i < CoreList_size(keys); i++) {
        CoreString_destroy((CoreString*)CoreList_get(keys, i));
    }
    for (size_t i = 0; i < CoreList_size(values); i++) {
        BencodeItem_destroy((BencodeItem*)CoreList_get(values, i));
    }
    CoreList_destroy(keys);
    CoreList_destroy(values);
    BencodeItem_destroy(dict_item);
    return NULL;
}

void debug_print(BencodeItem* item, int tab) {
    if (item == NULL) {
        return; // Nothing to print
    }

    if (item->type == BENCODE_TYPE_INTEGER) {
        printf("%*sInteger: %lld\n", tab, "", (long long)item->value.integer);
    } else if (item->type == BENCODE_TYPE_STRING) {
        printf("%*sString: '%s'\n", tab, "", item->value.string->str);
    } else if (item->type == BENCODE_TYPE_LIST) {
        printf("%*sList:\n", tab, "");
        for (size_t i = 0; i < item->value.list->count; i++) {
            debug_print(&item->value.list->items[i], tab + 2);
        }
    } else if (item->type == BENCODE_TYPE_DICTIONARY) {
        printf("%*sDictionary:\n", tab, "");
        for (size_t i = 0; i < item->value.dictionary->count; i++) {
            printf("%*sKey: '%s'\n", tab + 2, "", item->value.dictionary->keys[i]->str);
            debug_print(item->value.dictionary->values[i], tab + 4);
        }
    } else {
        printf("%*sUnknown type\n", tab, "");
    }
}

/// Parse a BencodeItem from a data buffer.
BencodeItem * BencodeItem_parse(const char *data, size_t size) {
    if (data == NULL || size == 0) {
        return NULL;
    }

    size_t pos = 0;
    BencodeItem *item = parse_next(data, size, &pos);
    if (item == NULL || pos != size) {
        BencodeItem_destroy(item);
        return NULL; // Parsing failed or not all data was consumed
    }

    // Debug print
    debug_print(item, 0);
    return item;
}


// Helper function to compare two dictionary keys
static int compare_keys(CoreString *key1, CoreString *key2) {
    const char *s1 = key1->str;
    size_t len1 = key1->length;
    const char *s2 = key2->str;
    size_t len2 = key2->length;

    size_t min_len = len1 < len2 ? len1 : len2;
    int cmp = memcmp(s1, s2, min_len);
    if (cmp != 0) return cmp;
    return (len1 > len2) - (len1 < len2);  // Compare lengths if prefixes match
}

// Serialization buffer structure
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} BencodeBuffer;

// Create a new buffer
static BencodeBuffer* bencode_buffer_create(void) {
    BencodeBuffer* buf = malloc(sizeof(BencodeBuffer));
    if (buf) {
        buf->data = NULL;
        buf->size = 0;
        buf->capacity = 0;
    }
    return buf;
}

// Append data to buffer
static bool bencode_buffer_append(BencodeBuffer* buf, const void* data, size_t len) {
    if (buf->size + len > buf->capacity) {
        size_t new_capacity = buf->capacity ? buf->capacity * 2 : 256;
        if (new_capacity < buf->size + len)
            new_capacity = buf->size + len;

        uint8_t* new_data = realloc(buf->data, new_capacity);
        if (!new_data) return false;

        buf->data = new_data;
        buf->capacity = new_capacity;
    }

    memcpy(buf->data + buf->size, data, len);
    buf->size += len;
    return true;
}

// Append a single character to buffer
static bool bencode_buffer_append_char(BencodeBuffer* buf, char c) {
    return bencode_buffer_append(buf, &c, 1);
}

// Serialization functions for different Bencode types
static bool save_integer_to_buffer(BencodeItem *item, BencodeBuffer *buf);
static bool save_string_to_buffer(BencodeItem *item, BencodeBuffer *buf);
static bool save_list_to_buffer(BencodeItem *item, BencodeBuffer *buf);
static bool save_dictionary_to_buffer(BencodeItem *item, BencodeBuffer *buf);

// Main serialization function
static bool save_item_to_buffer(BencodeItem *item, BencodeBuffer *buf) {
    switch (item->type) {
        case BENCODE_TYPE_INTEGER:
            return save_integer_to_buffer(item, buf);
        case BENCODE_TYPE_STRING:
            return save_string_to_buffer(item, buf);
        case BENCODE_TYPE_LIST:
            return save_list_to_buffer(item, buf);
        case BENCODE_TYPE_DICTIONARY:
            return save_dictionary_to_buffer(item, buf);
        default:
            return false;
    }
}

// Integer serialization
static bool save_integer_to_buffer(BencodeItem *item, BencodeBuffer *buf) {
    char temp[64];
    int n = snprintf(temp, sizeof(temp), "i%llde", (long long)item->value.integer);
    if (n < 0 || (size_t)n >= sizeof(temp)) return false;
    return bencode_buffer_append(buf, temp, (size_t)n);
}

// String serialization
static bool save_string_to_buffer(BencodeItem *item, BencodeBuffer *buf) {
    if (!item->value.string) return false;

    const char *str = item->value.string->str;
    size_t len = item->value.string->length;

    char prefix[32];
    int prefix_len = snprintf(prefix, sizeof(prefix), "%zu:", len);
    if (prefix_len < 0 || (size_t)prefix_len >= sizeof(prefix)) return false;

    return bencode_buffer_append(buf, prefix, (size_t)prefix_len) &&
           (len == 0 || bencode_buffer_append(buf, str, len));
}

// List serialization
static bool save_list_to_buffer(BencodeItem *item, BencodeBuffer *buf) {
    if (!item->value.list) return false;
    if (!bencode_buffer_append_char(buf, 'l')) return false;

    BencodeList *list = item->value.list;
    for (size_t i = 0; i < list->count; i++) {
        if (!save_item_to_buffer(&list->items[i], buf)) return false;
    }

    return bencode_buffer_append_char(buf, 'e');
}

// Dictionary serialization with insertion sort
static bool save_dictionary_to_buffer(BencodeItem *item, BencodeBuffer *buf) {
    if (!item->value.dictionary) return false;
    BencodeDictionary *dict = item->value.dictionary;
    size_t count = dict->count;

    if (!bencode_buffer_append_char(buf, 'd')) return false;
    if (count == 0) return bencode_buffer_append_char(buf, 'e');

    // Create and sort index array using insertion sort
    size_t *indices = malloc(count * sizeof(size_t));
    if (!indices) return false;

    // Initialize indices
    for (size_t i = 0; i < count; i++) indices[i] = i;

    // Insertion sort for lexicographical order
    for (size_t i = 1; i < count; i++) {
        size_t j = i;
        while (j > 0) {
            size_t prev_idx = indices[j-1];
            size_t curr_idx = indices[j];

            CoreString *prev_key = dict->keys[prev_idx];
            CoreString *curr_key = dict->keys[curr_idx];

            if (compare_keys(prev_key, curr_key) <= 0) break;

            // Swap indices
            indices[j] = prev_idx;
            indices[j-1] = curr_idx;
            j--;
        }
    }

    // Serialize in sorted order
    for (size_t i = 0; i < count; i++) {
        size_t idx = indices[i];
        CoreString *key = dict->keys[idx];
        BencodeItem *value = dict->values[idx];

        // Save key
        char prefix[32];
        int prefix_len = snprintf(prefix, sizeof(prefix), "%zu:", key->length);
        if (prefix_len < 0 || (size_t)prefix_len >= sizeof(prefix)) {
            free(indices);
            return false;
        }

        if (!bencode_buffer_append(buf, prefix, (size_t)prefix_len) ||
            !bencode_buffer_append(buf, key->str, key->length) ||
            !save_item_to_buffer(value, buf)) {
            free(indices);
            return false;
        }
    }

    free(indices);
    return bencode_buffer_append_char(buf, 'e');
}

// Convert BencodeItem to byte array
uint8_t* BencodeItem_to_bytes(const BencodeItem* item, size_t* out_size) {
    if (!item || !out_size) return NULL;

    BencodeBuffer buf = {0};
    if (!save_item_to_buffer((BencodeItem*)item, &buf)) {
        free(buf.data);
        return NULL;
    }

    *out_size = buf.size;
    return buf.data;
}

// Compute SHA1 hash of BencodeItem
uint8_t* BencodeItem_compute_sha1(const BencodeItem* item) {
    if (!item) return NULL;

    size_t buf_size = 0;
    uint8_t* buffer = BencodeItem_to_bytes(item, &buf_size);
    if (!buffer || !buf_size) return NULL;

    uint8_t hash[CC_SHA1_DIGEST_LENGTH];
    CC_SHA1_CTX ctx;
    CC_SHA1_Init(&ctx);
    CC_SHA1_Update(&ctx, buffer, (CC_LONG)buf_size);
    CC_SHA1_Final(hash, &ctx);

    free(buffer);

    uint8_t* hash_copy = malloc(CC_SHA1_DIGEST_LENGTH);
    if (hash_copy) memcpy(hash_copy, hash, CC_SHA1_DIGEST_LENGTH);
    return hash_copy;
}

static bool write_char(CoreFile *file, char c) {
    return CoreFile_write(file, &c, 1) == 1;
}

static bool write_buffer(CoreFile *file, const void *buf, size_t len) {
    return CoreFile_write(file, buf, len) == (uint64_t)len;
}

static bool save_integer(BencodeItem *item, CoreFile *file) {
    char temp[64];
    int n = snprintf(temp, sizeof(temp), "i%llde", (long long)item->value.integer);
    if (n < 0 || (size_t)n >= sizeof(temp)) {
        return false;
    }
    return write_buffer(file, temp, (size_t)n);
}

static bool save_string(BencodeItem *item, CoreFile *file) {
    if (item->value.string == NULL) {
        return false;
    }

    const char *data = item->value.string->str;
    size_t len = item->value.string->length;

    char prefix[32];
    int prefix_len = snprintf(prefix, sizeof(prefix), "%zu:", len);
    if (prefix_len < 0 || (size_t)prefix_len >= sizeof(prefix)) {
        return false;
    }

    if (!write_buffer(file, prefix, (size_t)prefix_len)) {
        return false;
    }
    if (len > 0) {
        return write_buffer(file, data, len);
    }
    return true;
}

static bool save_list(BencodeItem *item, CoreFile *file) {
    if (item->value.list == NULL) {
        return false;
    }

    if (!write_char(file, 'l')) {
        return false;
    }

    BencodeList *lst = item->value.list;
    for (size_t i = 0; i < lst->count; i++) {
        BencodeItem *child = &(lst->items[i]);
        if (!BencodeItem_save(child, file)) {
            return false;
        }
    }

    return write_char(file, 'e');
}

typedef struct {
    BencodeDictionary *dict;
    size_t *indices;
} DictSortCtx;

static DictSortCtx *g_sort_ctx = NULL;
static int dict_cmp_with_ctx(const void *a, const void *b) {
    size_t i1 = *(const size_t *)a;
    size_t i2 = *(const size_t *)b;
    CoreString *key1 = g_sort_ctx->dict->keys[i1];
    CoreString *key2 = g_sort_ctx->dict->keys[i2];

    const char *s1 = key1->str;
    size_t len1 = key1->length;
    const char *s2 = key2->str;
    size_t len2 = key2->length;

    /* Perform lexicographical comparison over raw bytes */
    size_t minlen = (len1 < len2) ? len1 : len2;
    int cmp = memcmp(s1, s2, minlen);
    if (cmp != 0) {
        return cmp;
    }
    if (len1 < len2) {
        return -1;
    } else if (len1 > len2) {
        return 1;
    }
    return 0;
}

static bool save_dictionary(BencodeItem *item, CoreFile *file) {
    if (item->value.dictionary == NULL) {
        return false;
    }

    BencodeDictionary *dict = item->value.dictionary;
    size_t count = dict->count;
    if (count == 0) {
        /* Even an empty dictionary is "de" */
        if (!write_char(file, 'd')) {
            return false;
        }
        return write_char(file, 'e');
    }

    size_t *indices = malloc(sizeof(size_t) * count);
    if (!indices) {
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        indices[i] = i;
    }

    DictSortCtx sort_ctx = { .dict = dict, .indices = indices };
    g_sort_ctx = &sort_ctx;
    qsort(indices, count, sizeof(size_t), dict_cmp_with_ctx);
    g_sort_ctx = NULL;

    if (!write_char(file, 'd')) {
        free(indices);
        return false;
    }

    for (size_t ix = 0; ix < count; ix++) {
        size_t idx = indices[ix];
        CoreString *key = dict->keys[idx];
        BencodeItem *val = dict->values[idx];

        {
            const char *kdata = key->str;
            size_t klen = key->length;
            char prefix[32];
            int plen = snprintf(prefix, sizeof(prefix), "%zu:", klen);
            if (plen < 0 || (size_t)plen >= sizeof(prefix)) {
                free(indices);
                return false;
            }
            if (!write_buffer(file, prefix, (size_t)plen)) {
                free(indices);
                return false;
            }
            if (klen > 0 && !write_buffer(file, kdata, klen)) {
                free(indices);
                return false;
            }
        }

        if (!BencodeItem_save(val, file)) {
            free(indices);
            return false;
        }
    }

    if (!write_char(file, 'e')) {
        free(indices);
        return false;
    }

    free(indices);
    return true;
}
