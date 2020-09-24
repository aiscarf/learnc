#ifndef __WS_PROTOCOL_H__
#define __WS_PROTOCOL_H__

class session;
class ws_protocol {
public:
	static bool ws_shake_hand(session* s, char* body, int len);
	static int read_ws_header(unsigned char* pkg_data, int pkg_len, int* pkg_socket);
	static void parser_ws_recv_data(unsigned char* raw_data, unsigned char* ma);
	static unsigned char* package_ws_data(const unsigned char* raw_data, int len);
	static void free_package_data(unsigned char* ws_pkg);
};

#endif // !__WS_PROTOCOL_H__
