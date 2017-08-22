#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <signal.h>
#include <thread>

#include <libwebsockets.h>

#include "example.h"

static int destroy_flag = 0;
static int connection_flag = 0;
static int writeable_flag = 0;

static void __signal_cb(int signo) {
	destroy_flag = 1;
}

struct session_data {
	int fd;
};

struct pthread_routine_tool {
	struct lws_context *context;
	struct lws *wsi;
};

static int websocket_write(struct lws *wsi_in, char *str, int str_size_in) {
	if (str == NULL || wsi_in == NULL)
		return -1;

	int n;
	int len;
	char *out = NULL;

	len = (str_size_in < 1) ? strlen(str) : str_size_in;

	out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
	//* setup the buffer*/
	memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, str, len);
	//* write out*/
	n = lws_write(wsi_in, (unsigned char*)out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

	printf("[websocket_write_back] %s\n", str);
	//* free the buffer*/
	free(out);

	return n;
}

static int ws_service_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user,
	void *in, size_t len) {
	switch (reason)
	{
	case LWS_CALLBACK_CLIENT_ESTABLISHED: {
		printf("Connect with server success.\n");
		connection_flag = 1;
	} break;
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
		printf("Connect with server error.\n");
		destroy_flag = 1;
		connection_flag = 0;
	} break;
	case LWS_CALLBACK_CLOSED: {
		printf("LWS_CALLBACK_CLOSED\n");
		destroy_flag = 1;
		connection_flag = 0;
	} break;
	case LWS_CALLBACK_CLIENT_RECEIVE: {
		printf("Client recvived:%s\n", (char *)in);
		if (writeable_flag)
			destroy_flag = 1;
	} break;
	case LWS_CALLBACK_CLIENT_WRITEABLE: {
		printf("On writeable is called. send byebye message\n");
		websocket_write(wsi, "Byebye! See you later", -1);
		writeable_flag = 1;
	} break;

	default:
		break;
	}

	return 0;
}

static void *pthread_routine(void *tool_in)
{
	struct pthread_routine_tool *tool = (struct pthread_routine_tool *)tool_in;

	printf("[pthread_routine] Good day. This is pthread_routine.\n");
	//* waiting for connection with server done.*/
	while (!connection_flag)
	{
		Sleep(1000 * 2);
	}

	static int i = 0;
	while (!destroy_flag)
	{
		char buf[128];
		sprintf(buf, "{\"data\":\"Good day [%d]\"}", i++);
		websocket_write(tool->wsi, buf, strlen(buf));
		Sleep(200);
	}

	//*Send greeting to server*/
	printf("[pthread_routine] Server is ready. send a greeting message to server.\n");
	websocket_write(tool->wsi, "Good day", -1);

	printf("[pthread_routine] sleep 2 seconds then call onWritable\n");
	Sleep(1);
	printf("------------------------------------------------------\n");
		
	Sleep(1);
	//*involked wriable*/
	printf("[pthread_routine] call on writable.\n");
	lws_callback_on_writable(tool->wsi);
	
	return nullptr;
}

static struct lws_protocols protocols[] =
{
	{
		"my-protocol",
		ws_service_callback,
		sizeof(struct session_data),
		0,
		0,
		NULL,
		0
	},
	{
		NULL, NULL, 0, 0, 0, NULL, 0   /* End of list */
	}
};

int test_client(int argc, char **argv)
{
	struct lws_context *context = NULL;
	struct lws_context_creation_info info;
	struct lws *wsi = NULL;

	memset(&info, 0, sizeof info);
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.iface = NULL;
	info.protocols = protocols;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.extensions = NULL;
	info.gid = -1;
	info.uid = -1;
	info.options = 0;

	context = lws_create_context(&info);
	printf("[Main] context created.\n");

	if (context == NULL)
	{
		printf("[Main] context is NULL.\n");
		return -1;
	}

	wsi = lws_client_connect(context, "localhost", 5000, 0, "/", "localhost:5000", NULL, protocols[0].name, -1);
	if (wsi == NULL)
	{
		printf("[Main] wsi create error.\n");
		return -1;
	}

	printf("[Main] wsi create success.\n");

	struct pthread_routine_tool tool;
	tool.wsi = wsi;
	tool.context = context;

	std::thread a(pthread_routine, &tool);
	a.detach();

	while (!destroy_flag)
	{
		lws_service(context, 50);
	}

	lws_context_destroy(context);

	return 0;
}