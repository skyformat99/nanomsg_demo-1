#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "base_type.h"

#include <functional>
#include <string>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include <event2/event.h>
#include <event2/event-config.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/util.h>

typedef void(*LW_HTTP_CB)(struct evhttp_request *, void *);

class HttpServer;

struct HTTP_BUSINESS_SIGNATURE
{
	char * _signature;
	char * _cmd;
	LW_HTTP_CB _cb;

public:
	HTTP_BUSINESS_SIGNATURE(char * signature, char* cmd, LW_HTTP_CB cb)
	{
		this->_signature = signature;
		this->_cmd = cmd;
		this->_cb = cb;
	}
};

class HttpServer
{
public:
	HttpServer();
	~HttpServer();

public:
	void start_listener(const char* addr, lw_uint32 port, std::function<lw_int32(HttpServer* server)> func);
	void set_cb(const char * path, LW_HTTP_CB cb, void * cb_arg);
	void set_gen_cb(LW_HTTP_CB cb, void * cb_arg);

private:
	void __run();

private:
	struct event_base* _http_base;
	struct evhttp *_httpServ;
	std::string _addr;
	lw_uint32 port;
	std::function<lw_int32(HttpServer* server)> _func;
};

#endif // !__HTTP_SERVER_H__