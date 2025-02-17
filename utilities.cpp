#include <cstdint>
#include <sys/types.h>
#include <string.h>

#include "utilities.h"

void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

void buf_append_u8(std::vector<uint8_t> &buf,const uint8_t data) {
    buf.push_back(data);
}
void buf_append_u32(std::vector<uint8_t> &buf, const uint32_t data) {
    buf_append(buf, (const uint8_t *)&data, 4);
}
void buf_append_i64(std::vector<uint8_t> &buf, const int64_t data) {
    buf_append(buf, (const uint8_t *)&data, 8);
}
void buf_append_dbl(std::vector<uint8_t> &buf, const double data) {
    buf_append(buf, (const uint8_t *)&data, 8);
}
void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, const size_t size) {
	buf.insert(buf.end(), data, data + size);
}

void buf_consume(std::vector<uint8_t>& buf, const size_t size) {
	buf.erase(buf.begin(), buf.begin() + size);
}

bool read_uint32(const uint8_t *&cur, const uint8_t *end, uint32_t& rez) {
	if (cur + 4 > end)
		return false;

	memcpy(&rez, cur, sizeof(uint32_t));
	cur += 4;
	return true;
}

bool read_string(const uint8_t *&cur, const uint8_t *end, std::string &out, size_t size) {
	if (cur + size > end)
		return false;
	
	out.assign(cur, cur + size);	
	cur += size;
	return true;
}

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


void out_nil(std::vector<uint8_t> &buffer) {
	buffer.push_back(TAG_NIl);
}

void out_str(std::vector<uint8_t> &buffer, const char *s, size_t size) {
	buffer.push_back(TAG_STR);
	buf_append_u32(buffer, (uint32_t) size);
	buf_append(buffer, (const uint8_t*) s, size);
}

void out_int(std::vector<uint8_t> &buffer, const int64_t val) {
	buffer.push_back(TAG_INT);
	buf_append_i64(buffer, val);
}

void out_arr(std::vector<uint8_t> &buffer, const uint32_t arr_len) {
	buffer.push_back(TAG_ARR);
	buf_append_u32(buffer, arr_len);
}

