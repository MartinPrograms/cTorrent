#ifndef BENCODE_H
#define BENCODE_H

#include <CoreFile.h> // For file analysis
#include <CoreString.h> // For string manipulation
#include <CoreStack.h>
#include <CoreList.h>
#include <CoreDictionary.h>

// Bencode ends properties with 'e', so an int property would look like i42e, where i is the int type, and e the end with 42 the value.
#define BENCODE_END_PROPERTY 'e'
#define BENCODE_MAX_STACK_SIZE 1024 // If we have more than 1024 nested items, what?

typedef enum {
    BENCODE_TYPE_INTEGER,
    BENCODE_TYPE_STRING,
    BENCODE_TYPE_LIST,
    BENCODE_TYPE_DICTIONARY
} BencodeType;

// Predefine BencodeItem so we can use it in BencodeList and BencodeDictionary
typedef struct BencodeItem BencodeItem;

typedef struct {
    BencodeItem *items;
    size_t count;
} BencodeList;

typedef struct {
    CoreString **keys;
    BencodeItem **values;
    size_t count;
} BencodeDictionary;

struct BencodeItem {
    BencodeType type;
    union {
        int64_t integer;
        CoreString *string;
        BencodeList *list;
        BencodeDictionary *dictionary;
    } value;
};

BencodeItem *BencodeItem_create(BencodeType type); // After creating, you can make it whatever you want.
void BencodeItem_destroy(BencodeItem *item);

BencodeItem *BencodeItem_parse(const char* data, size_t size);
bool BencodeItem_save(BencodeItem *item, CoreFile *file); // First you have to create a file (CoreFile_create) and then you can save it to the file.

uint8_t *BencodeItem_compute_sha1(const BencodeItem *item); // Compute the SHA1 hash of the item, useful for torrent files.
#endif //BENCODE_H
