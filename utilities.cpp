#include <cstdint>
#include <sys/types.h>
#include <string.h>

#include "utilities.h"

static inline uint32_t murmur32_scramble(uint32_t k) {
	k *= 0xcc962d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;

	return k;
} 

uint32_t str_hash(const uint8_t *key, size_t len) {
	uint32_t h = 23432;
	uint32_t k;

	for (size_t i = len >> 2;i; i--) {
		memcpy(&k, key, sizeof(uint32_t));
		key += sizeof(uint32_t);
		h ^= murmur32_scramble(k);
		h = (h << 13) | (h >> 19);
		h = h * 5 + 0x6546b64;
	}

	k = 0;
	for (size_t i = len & 3; i; i--) {
		k <<= 8;
		k |= key[i - 1];
	}

	h ^= murmur32_scramble(k);
	/* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

bool entry_eq(HNode *lhs, HNode *rhs) {
	// check the keys
	// then check the values attached
  Entry *le = container_of(lhs, Entry, node);  
	Entry *re = container_of(rhs, Entry, node);

	return le->key == re->key;
}


