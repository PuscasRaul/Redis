#include <cstddef>
#include <cstdint>
#include <stdio.h>

const size_t max_load_factor = 32; 
const size_t rehash_work = 128;

#define container_of(ptr, T, memb) (T *) ((char*) ptr - offsetof(T, memb));

// simple utility functions to be used
// i am not sure that my function will work, since the data stored
// might not be even
// so one equality function might f things up
// will not use it for now, might modify later
typedef uint64_t (*htable_hash)(const void *in);
typedef bool (*htable_keq)(const void *k1, const void *k2);

struct HNode {
	HNode* next;
	uint32_t hcode = 0;
};

struct HTab {
	HNode **buckets;	// array of buckets
	size_t mask  = 0; // a power of 2 to use for the hash function
	size_t size = 0; // the number of keys being stored
};

struct HMap {
	HTab newer; // we will resize lazily, so we keep 2 of them
	HTab older;
	size_t migrate_pos = 0;
};

HNode *hmap_lookup(HMap *hmap, HNode *node, bool (*eq)(HNode*, HNode*));
HNode *hmap_deletion(HMap *hmap, HNode *hnode, bool (*eq)(HNode*, HNode*));
void hmap_insert(HMap *hmap, HNode *hnode);
