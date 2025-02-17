// c/system
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/poll.h>
#include <sys/types.h>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>

// c++
#include <stdio.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

// personal headers
#include "utilities.h"

using Buffer =std::vector<uint8_t>;

const uint16_t port = 34900;
const int backlog = 64;
g_data g_data = {}; // initialize the hmap

bool set_nonblocking(int socket_fd) {
	int flags = fcntl(socket_fd, F_GETFD, 0);
	if (flags < 0) {
		std::strerror(errno);
		return false;	
	}	
	flags |= O_NONBLOCK;
	if (fcntl(socket_fd, F_SETFD, flags) < 0) {
		std::strerror(errno);
		return false;	
	}
	return true;
}

std::unique_ptr<Con> handle_accept(int listen_fd) {
	int client_fd;
	sockaddr_in client = {};
	socklen_t client_size = sizeof(sockaddr_in);
	if ((client_fd = accept(listen_fd, (sockaddr*)&client, &client_size)) < 0) {
		msg("accept");
		return nullptr;
	}
	std::unique_ptr<Con> con = std::make_unique<Con>();	
	con->fd = client_fd;
	con->want_read = true;
	return con;
}

ssize_t parse_request(const uint8_t *data, size_t size ,std::vector<std::string> &out) {
	const uint8_t *end = data + size;
	uint32_t net_nstr, nstr = 0;
	
	if (!read_uint32(data, end, net_nstr)) {
		msg("couldn't read nstr");
		return -1;
	}
	
	nstr = ntohl(net_nstr);
	if (nstr > k_max_args) {
		msg("too many arguments");
		return -1;
	}
	
	while (out.size() < nstr) {
		uint32_t net_len, len;
		if (!read_uint32(data, end, net_len))
			return -1;
		len = ntohl(net_len);
		out.push_back(std::string());
		if (!read_string(data, end, out.back(), len))
			return -1;
	}
	
	if (data != end)
		return -1;

	return 0;
}

void handle_write(std::unique_ptr<Con>& con) {
	assert(con->outgoing.size() > 0); 
	ssize_t rv = write(con->fd, con->outgoing.data(), con->outgoing.size());
	fprintf(stderr, "Wrote: %ld bytes\n", rv);
	if (rv < 0 && (errno == EINTR || errno == EAGAIN)) 
		return;
	
	if (rv < 0) {
		msg("write error");
		con->want_close = true;
		return;
	}

	buf_consume(con->outgoing, con->outgoing.size());
	if (con->outgoing.size() == 0) {
		con->want_read = true;
		con->want_write = false;
	}
}

static void do_get(std::vector<std::string> &cmds, Buffer &buffer) {
	Entry key;
	key.key.swap(cmds[1]);

	// set the hcode for this key
	key.node.hcode = str_hash((uint8_t*) key.key.data(), key.key.size());
	HNode *hnode = hmap_lookup(&g_data.db, &key.node, &entry_eq);  

	msg("something after the search");
	if (!hnode) {
		out_nil(buffer);
		return;
	}

	Entry *result = container_of(hnode, Entry, node);
	const std::string val = result->val;

	assert(val.size() < k_max_msg);
	out_str(buffer, val.data(), val.size());
}

static void do_set(std::vector<std::string>& cmds, Buffer &out_buffer) {
	// we have to search for the key
	// if it exists, we update it
	// otherwise we add a new entry into the hashmap
	
	Entry lookup;
	lookup.key.swap(cmds[1]);
	lookup.node.hcode = str_hash((uint8_t*) lookup.key.data(), lookup.key.size());
	HNode *hnode = hmap_lookup(&g_data.db, &lookup.node, &entry_eq);
	
	// no such entry exists
	if (!hnode) {
		msg("didn't find the key");
		Entry *new_entry = new Entry();
		new_entry->key.swap(lookup.key);
		new_entry->val.swap(cmds[2]);
		new_entry->node.hcode = lookup.node.hcode; 
		hmap_insert(&g_data.db, &new_entry->node);
		out_nil(out_buffer);
		return;
	}
	
	Entry *result = container_of(hnode, Entry, node);
	result->val.swap(cmds[2]);
	out_nil(out_buffer);
}

static void do_delete(std::vector<std::string> &cmds, Buffer &buffer) {
	// search the key
	Entry key;
	key.key.swap(cmds[1]);
	key.node.hcode = str_hash((uint8_t*)key.key.data(), key.key.size());

	HNode *hnode = hmap_lookup(&g_data.db, &key.node, entry_eq);
	if (!hnode) {
		out_nil(buffer);
		return;
	}

	Entry *value = container_of(hnode, Entry, node);
	out_int(buffer, 1);

	hmap_deletion(&g_data.db, hnode, entry_eq); 
}

static void do_request(Buffer &buffer, std::vector<std::string> &commands) {
	if (commands.size() == 2 && commands[0] == "get")	{
		msg("case get");
		do_get(commands, buffer);
	} else if (commands.size() == 3 && commands[0] == "set") {
			msg("case set");
			do_set(commands, buffer);
	} else if (commands.size() == 2 && commands[0] == "del") {
			msg("case del");
			do_delete(commands, buffer);
	} else {
		out_nil(buffer);
	}
}

static void response_begin(Buffer &out, size_t *header) {
	*header = out.size();
	buf_append_u32(out, 0);
}

static size_t response_size(const Buffer &out, const size_t header) {
	return out.size() - header - 4;
}

static void response_end(Buffer &out, size_t header) {
	size_t message_size = response_size(out, header);
	if (message_size > k_max_msg) {
		out.resize(header + 4);
		msg("response is too big");
		message_size = response_size(out, header);
	}
	uint32_t len = ntohl((uint32_t) message_size);
	memcpy(&out[header], &len, 4);

}

static bool try_one_request(std::unique_ptr<Con>& con) {
	// total length of the message 
	if (con->incoming.size() < 4)	
		return false; // still have to read
	
	uint32_t net_length, length;
	memcpy(&net_length, con->incoming.data(), sizeof(uint32_t));
	length = ntohl(net_length);

	if (length > k_max_msg) {
		msg("msg too long");
		con->want_close = true;
		return false;
	}
	
	if (4 + length > con->incoming.size()) {
		msg(" not enough data in buffer");
		return false;
	}
	
	std::vector<std::string> commands; 
	const uint8_t *request = &con->incoming[4];

	if (parse_request(request, length, commands)) {
		msg("bad request");
		con->want_close = true;
		return false;
	}

	size_t header_pos = 0;
	response_begin(con->outgoing, &header_pos); 
	do_request(con->outgoing, commands);
	response_end(con->outgoing, header_pos);
	handle_write(con);

	buf_consume(con->incoming, 4 + length);
	return true;
}


void handle_read(std::unique_ptr<Con>& con) {
	uint8_t buf[64 * 1024];
	ssize_t rv = read(con->fd, buf, sizeof(buf));
	
	if (rv < 0 && (errno == EAGAIN || errno == EINTR)) {
		msg("eagain or eintr");
		return;
	}	

	if (rv < 0) {
		msg("read error");	
		con->want_close = true;
		return;
	}
	
	if (rv == 0) {
		if (con->incoming.size())
			msg("unexpected eof");
		else 
			msg("client closed");
		con->want_close = true;
		return;
	}
	
	std::cerr<<"read: "<<rv<<"\n";
	buf[rv] = '\0';
	buf_append(con->incoming, buf, (size_t) rv);
	while (try_one_request(con)) {}
	
	if (con->outgoing.size() > 0) {
		con->want_write = true;
		con->want_read = false;

		return handle_write(con);
	}
}

int main() {
	int listen_fd;
	sockaddr_in server = {};	
	
	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		std::cerr<<std::strerror(errno);
	}
	
	server.sin_port = htons(port);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(listen_fd, (sockaddr*)&server, sizeof(server)) < 0) {
		std::cerr<<std::strerror(errno);
	}
	
	if (!set_nonblocking(listen_fd))
		std::cerr<<std::strerror(errno);
	
	if (listen(listen_fd, backlog) < 0)
		std::cerr<<std::strerror(errno);
	
	std::vector<std::unique_ptr<Con>> fd2con;
	std::vector<pollfd> poll_args;
	
	for(;;) {
		poll_args.clear();
		pollfd listen_pfd = {listen_fd, POLLIN, 0};
		poll_args.push_back(listen_pfd);
	
		for (auto &con : fd2con) {
			if (!con)
				continue;	
			
			pollfd pfd	= {con->fd, POLLERR, 0};
			if (con->want_read)
				pfd.events |= POLLIN;
			
			if (con->want_write)
				pfd.events |= POLLOUT;
			
			poll_args.push_back(pfd);
		}
		
		int rv = poll(poll_args.data(), (nfds_t) poll_args.size(), -1);
		if (rv < 0 && errno == EINTR)
			continue;
		if (rv < 0)
			die("poll");
		
		if (poll_args.at(0).revents) {
			// we can accept a new connection
			std::unique_ptr<Con> new_con = handle_accept(listen_fd);
			if (new_con) {
				if (fd2con.size() < new_con->fd)
					fd2con.resize(new_con->fd + 1);
				fd2con[new_con->fd]	= std::move(new_con);
			}	
		}
		
		for (size_t i = 1; i < poll_args.size(); i++) {
			uint32_t ready = poll_args[i].revents;
			if (ready & POLLIN)
				handle_read(fd2con.at(poll_args[i].fd));
			if (ready & POLLOUT)
				handle_write(fd2con.at(poll_args[i].fd));
			if (ready & POLLERR || fd2con.at(poll_args[i].fd)->want_close) {
				close(poll_args[i].fd);
				fd2con[poll_args[i].fd].reset();
			}
		}
	}
	return 0;
}
