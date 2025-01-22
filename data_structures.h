#include <cstdint>
#include <vector>
#include <map>
#include <string>

const uint32_t k_max_args = 32;
const uint32_t k_max_msg = 12800;
static std::map<std::string, std::string> g_data = std::map<std::string, std::string>();

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
