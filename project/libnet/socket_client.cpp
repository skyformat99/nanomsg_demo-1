﻿#include "socket_client.h"

#include <stdio.h>
#include <thread>
#include <assert.h>

#include "event2/event.h"

#include "socket_core.h"
#include "socket_session.h"
#include "socket_timer.h"

using namespace LW;

////////////////////////////////////////////////////////////////////////////////////////////////////

SocketClient::SocketClient() : isession(NULL)
{
	_session = new SocketSession(SESSION_TYPE::Client);
	_timer = new SocketTimer();
}

SocketClient::~SocketClient()
{
	delete _session;
	_session = NULL;

	delete _timer;
	_timer = NULL;
}

bool SocketClient::create(ISocketSession* isession)
{
	if (this->_base == NULL)
	{
		this->_base = event_base_new();

		_timer->create(this->_base);
	}

	this->isession = isession;

	return true;
}

void SocketClient::destory()
{
	if (_timer != NULL)
	{
		_timer->destory();
	}

	if (_session != NULL)
	{
		_session->destory();
	}

	if (this->_base != NULL)
	{
		event_base_free(this->_base);
		this->_base = NULL;
	}
}

int SocketClient::run(const char* addr, int port)
{
	if (addr == NULL) return -1;

	this->_session->setHost(addr);
	this->_session->setPort(port);


	std::thread t(std::bind(&SocketClient::__run, this));
	t.detach();

	return 0;
}

SocketSession* SocketClient::getSession()
{
	return this->_session;
}

int SocketClient::startTimer(int id, int t, std::function<bool(int id)> func)
{
	assert(_timer != NULL);
	if (_timer != NULL)
	{
		_timer->startTimer(id, t, func);
	}	
	return 0;
}

void SocketClient::killTimer(int id)
{
	assert(_timer != NULL);
	if (_timer != NULL)
	{
		_timer->killTimer(id);
	}
}

void SocketClient::__run()
{
	int r = this->_session->create(_base, -1, EV_READ | EV_PERSIST, this->isession);

	event_base_dispatch(this->_base);

	this->_session->destory();
	this->destory();

	return;
}