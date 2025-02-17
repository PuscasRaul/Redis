#include <cstddef>
#include <cstdint>
#include <vector>
#include <map>
#include <string>

#include "hashtable.h"

const uint32_t k_max_args = 32;
const uint32_t k_max_msg = 12800;

enum {
	TAG_NIl = 0,
	TAG_ERR = 1,
	TAG_STR = 2,
	TAG_INT = 3, 
	TAG_DBL = 4,
	TAG_ARR = 5
};

struct Con {
	int fd = -1;
	bool want_read = false;
	bool want_write = false;
	bool want_close = false;
	
	std::vector<uint8_t> incoming; // data to be parsed
	std::vector<uint8_t> outgoing; // generated response
};

struct g_data {
	HMap db; 
};

struct Entry {
	struct HNode node;
	std::string key;
	std::string val;
};

// utility functions for error reporting
void msg(const char *msg);
void die(const char *msg);



// utility functions used for appending, reading strings
void buf_append(std::vector<uint8_t>& buf, const uint8_t *data, size_t size);
void buf_consume(std::vector<uint8_t>& buf, size_t size);
bool read_uint32(const uint8_t *&cur, const uint8_t *end, uint32_t& rez); 
bool read_string(const uint8_t *&cur, const uint8_t *end, std::string &out, size_t size); 
void buf_append_dbl(std::vector<uint8_t> &buf, double data); 
void buf_append_i64(std::vector<uint8_t> &buf, int64_t data); 
void buf_append_u32(std::vector<uint8_t> &buf, uint32_t data);
void buf_append_u8(std::vector<uint8_t> &buf, uint8_t data); 

// utility functions used for hashing
// might need to move them to the hashmap file
// nvm i have to do something else before that
uint32_t str_hash(const uint8_t *data, size_t len);
bool entry_eq(HNode *lhs, HNode *rhs); 

// utility functions used for data serialization
void out_nil(std::vector<uint8_t> &buffer);
void out_str(std::vector<uint8_t> &buffer, const char *s, size_t size); 
void out_arr(std::vector<uint8_t> &buffer, uint32_t arr_len);
void out_int(std::vector<uint8_t> &buffer, int64_t val); 

