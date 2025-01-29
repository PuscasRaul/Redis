#include "utilities.h"
#include <cstdint>
#include <sys/types.h>

uint64_t str_hash(const uint8_t *data, size_t len) {
 uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

bool entry_eq(HNode *lhs, HNode *rhs) {
	// check the keys
	// then check the values attached
  Entry *le = container_of(lhs, Entry, node);  
	Entry *re = container_of(rhs, Entry, node);

	return le->key == re->key;
}


