#include <cstddef>
#include <cstdint>
#include <vector>
#include <map>
#include <string>

#include "hashtable.h"

const uint32_t k_max_args = 32;
const uint32_t k_max_msg = 12800;

enum {
	RESPONSE_OK = 0,
	RESPONSE_ERR = 1, // error
	RESPONSE_NX = 2 // key not found
};

struct response{
	uint32_t status = 0;
	std::vector<uint8_t> data;
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

uint64_t str_hash(const uint8_t *data, size_t len);
bool entry_eq(HNode *lhs, HNode *rhs); 

