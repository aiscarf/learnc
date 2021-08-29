#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>

#include "uv.h"
#include "session.h"
#include "session_uv.h"

#include "netbus.h"
#include "ws_protocol.h"



using namespace std;


extern "C" {

	static void on_recv_client_cmd(uv_session* s, unsigned char* body, int len) {
		printf("client command !!!!\n");

		// todo test
		s->send_data(body, len);
		// end
	}

	static void on_recv_ws_data(uv_session* s) {
		unsigned char* pkg_data = (unsigned char*)((s->long_pkg != NULL) ? s->long_pkg : s->recv_buf);
		while (s->recved > 0)
		{
			if (pkg_data[0] == 0x88) {
				s->close();
				break;
			}

			int pkg_size = 0;
			int head_size = 0;

			if (!ws_protocol::read_ws_header(pkg_data, s->recved, &pkg_size, &head_size)) {
				break;
			}

			if (s->recved < pkg_size)
			{
				break;
			}

			unsigned char* raw_data = pkg_data + head_size;
			unsigned char* mask = raw_data - 4;
			ws_protocol::parser_ws_recv_data(raw_data, mask, pkg_size - head_size);

			on_recv_client_cmd(s, raw_data, pkg_size - head_size);

			if (s->recved > pkg_size)
			{
				memmove(pkg_data, pkg_data + pkg_size, s->recved - pkg_size);
			}
			s->recved -= pkg_size;

			if (s->recved == 0 && s->long_pkg != NULL)
			{
				free(s->long_pkg);
				s->long_pkg = NULL;
				s->long_pkg_size = 0;
			}
		}
	}

	static void after_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
		
		uv_session* s = (uv_session*)stream->data;
		if (nread < 0)
		{
			s->close();
			return;
		}

		s->recved += nread;

		if (s->socket_type == WS_SOCKET) // WebSocket
		{
			if (s->is_ws_shake == 0)
			{
				if (ws_protocol::ws_shake_hand((session*)s, s->recv_buf, s->recved))
				{
					s->is_ws_shake = 1;
					s->recved = 0;
				}
			}
			else {
				
			}
		}
		else { // TCP Socket

		}
	}

	static void uv_alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		uv_session* s = (uv_session*)handle->data;

		if (s->recved < RECV_LEN)
		{
			*buf = uv_buf_init(s->recv_buf + s->recved, RECV_LEN - s->recved);
		}
		else {
			if (s->long_pkg == NULL)
			{
				if (s->socket_type == WS_SOCKET && s->is_ws_shake)
				{
					int pkg_size;
					int head_size;
					ws_protocol::read_ws_header((unsigned char*)s->recv_buf, s->recved, &pkg_size, &head_size);
					s->long_pkg_size = pkg_size;
					s->long_pkg = (char*)malloc(pkg_size);
					memcpy(s->long_pkg, s->recv_buf, s->recved);
				}
				else { // tcp > RECV_LEN's package

				}

				*buf = uv_buf_init(s->long_pkg + s->recved, s->long_pkg_size - s->recved);
			}
		}
	}

	static void uv_connection(uv_stream_t* server, int status) {

		uv_session* s = uv_session::create();
		uv_tcp_t* client = &s->tcp_handle;
		memset(client, 0, sizeof(uv_tcp_t));
		uv_tcp_init(uv_default_loop(), client);
		client->data = (void*)s;
		uv_accept(server, (uv_stream_t*)client);

		struct sockaddr_in addr;
		int len = sizeof(addr);
		uv_tcp_getpeername(client, (sockaddr*)&addr, &len);
		uv_ip4_name(&addr, (char*)s->c_address, 64);
		s->c_port = ntohs(addr.sin_port);
		s->socket_type = (int)server->data;
		printf("new client coming %s:%d\n", s->c_address, s->c_port);

		uv_read_start((uv_stream_t*)client, uv_alloc_buf, after_read);
	}
}

static netbus g_netbus;
netbus* netbus::instance() {
	return &g_netbus;
}

void netbus::init()
{
	init_session_allocer();
}

void netbus::start_tcp_server(int port)
{
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(uv_tcp_t));

	uv_tcp_init(uv_default_loop(), listen);

	struct sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);

	int ret = uv_tcp_bind(listen, (const struct sockaddr*) & addr, 0);
	if (ret != 0)
	{
		printf("bind error\n");
		free(listen);
		return;
	}

	uv_listen((uv_stream_t*)listen, SOMAXCONN, uv_connection);
	listen->data = (void*)TCP_SOCKET;
}

void netbus::start_ws_server(int port)
{
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(uv_tcp_t));

	uv_tcp_init(uv_default_loop(), listen);

	struct sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);

	int ret = uv_tcp_bind(listen, (const struct sockaddr*) & addr, 0);
	if (ret != 0)
	{
		printf("bind error\n");
		free(listen);
		return;
	}

	uv_listen((uv_stream_t*)listen, SOMAXCONN, uv_connection);
	listen->data = (void*)WS_SOCKET;
}

void netbus::run()
{
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}


