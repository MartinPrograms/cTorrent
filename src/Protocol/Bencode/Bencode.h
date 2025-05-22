#ifndef BENCODE_H
#define BENCODE_H

#include <CoreFile.h> // For file analysis
#include <CoreString.h> // For string manipulation

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
        long long integer;
        CoreString *string;
        BencodeList *list;
        BencodeDictionary *dictionary;
    } value;
};

BencodeItem *BencodeItem_create(BencodeType type); // After creating, you can make it whatever you want.
void BencodeItem_destroy(BencodeItem *item);

BencodeItem *BencodeItem_parse(const char* data, size_t size);
bool BencodeItem_save(BencodeItem *item, CoreFile *file); // First you have to create a file (CoreFile_create) and then you can save it to the file.

#endif //BENCODE_H
