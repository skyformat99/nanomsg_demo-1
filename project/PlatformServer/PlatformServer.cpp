#include "PlatformServer.h"
	
#include "command.h"
#include "platform.pb.h"

#include "Users.h"
#include <memory>

using namespace LW;

ServerHandler::ServerHandler()
{
	iuser = new Users();
}

ServerHandler::~ServerHandler()
{

}

void ServerHandler::onListener(SocketSession* session)
{
	static int i = 0;
	USER_INFO user;
	user.uid = i++;
	iuser->add(user, session);

	UserSession* us = iuser->find(user.uid);

	platform::msg_connected msg;
	lw_llong64 t = time(NULL);
	msg.set_time(t);
	int len = msg.ByteSize();
	{
		char *s = new char[len + 1];

		bool ret = msg.SerializeToArray(s, len);
		if (ret)
		{
			session->sendData(cmd_connected, s, len);
		}
		delete s;
	}

	printf("join ([%d] host: %s, port:%d)\n", session->getSocket(), session->getHost().c_str(), session->getPort());
}

int ServerHandler::onSocketConnected(SocketSession* session)
{
	return 0;
}

int ServerHandler::onSocketDisConnect(SocketSession* session)
{
	iuser->remove(session);
	return 0;
}

int ServerHandler::onSocketTimeout(SocketSession* session)
{
	iuser->remove(session);
	return 0;
}

int ServerHandler::onSocketError(SocketSession* session)
{
	iuser->remove(session);
	return 0;
}

void ServerHandler::onSocketParse(SocketSession* session, lw_int32 cmd, lw_char8* buf, lw_int32 bufsize)
{
	switch (cmd)
	{
	case cmd_connected:
	{
		printf("cmd_connected: %d\n", session->getSocket());
	} break;
	case cmd_heart_beat:
	{
		platform::msg_heartbeat msg;
		lw_llong64 t = time(NULL);
		msg.set_time(t);
		int len = msg.ByteSize();
		{
			std::unique_ptr<char[]> s(new char[len + 1]);
			bool ret = msg.SerializeToArray(s.get(), len);
			if (ret)
			{
				session->sendData(cmd_heart_beat, s.get(), len);
			}
		}
	} break;
	case cmd_platform_cs_userinfo:
	{
		platform::msg_userinfo_request client_userinfo;
		client_userinfo.ParseFromArray(buf, bufsize);

		platform::msg_userinfo_reponse userinfo;
		userinfo.set_uid(client_userinfo.uid());
		userinfo.set_age(30);
		userinfo.set_sex(1);
		userinfo.set_name("liwei");
		userinfo.set_address("guangdong");
		int c = userinfo.ByteSize();
		std::unique_ptr<char[]> s(new char[c + 1]);
		bool ret = userinfo.SerializeToArray(s.get(), c);
		if (ret)
		{
			session->sendData(cmd_platform_sc_userinfo, s.get(), c);
		}
	} break;
	default:
		break;
	}
}
