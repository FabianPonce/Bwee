#include "StdAfx.h"
SocketMgr *pSocketMgr = NULL;

SimpleSocket::SimpleSocket()
{
	m_fd = 0;
	m_sendCount = 0;
	m_sendPerPeriod = 3;
	m_lastSendTime = time(NULL);
}

SimpleSocket::~SimpleSocket()
{

}

void SimpleSocket::UpdateQueue()
{
	time_t nt = time(NULL);
	uint32 c = 0;
	if( m_lastSendTime == nt )
		return;

	m_lastSendTime = nt;
	buffer_lock.Acquire();
	while( m_sendQueue.size() )
	{
		c++;
		m_outBuf.append(m_sendQueue.front().c_str(), m_sendQueue.front().length());
		m_sendQueue.pop_front();
		if( c >= m_sendPerPeriod )
			break;
	}
	buffer_lock.Release();
	m_sendCount = c;
}

void SimpleSocket::SendLine(string line)
{
	if( m_sendCount >= m_sendPerPeriod )
	{
		m_sendQueue.push_back(line);
		return;
	}

	buffer_lock.Acquire();
	m_outBuf.append(line.c_str(), line.length());
	buffer_lock.Release();
	m_sendCount++;
}

void SimpleSocket::SendForcedLine(string line)
{
	buffer_lock.Acquire();
	m_outBuf.append(line.c_str(), line.length());
	buffer_lock.Release();
}

bool SimpleSocket::HasLine()
{
	bool ret = false;
	buffer_lock.Acquire();
	if( m_inBuf.find("\n") != string::npos )
		ret = true;		
	buffer_lock.Release();
	return ret;
}

string SimpleSocket::GetLine()
{
	// can probably be optimized /lazy

	string ret;
	char c;

	buffer_lock.Acquire();
	for( ;; )
	{
		c = m_inBuf[0];
		m_inBuf.erase(0, 1);

		if( c == '\n' )
			break;
		
		ret.append(&c, 1);
	}
	buffer_lock.Release();
	return ret;
}

/*void SimpleSocket::WipeBuffers()
{
	buffer = "";
	m_full = false;
	SendBuffer.clear();
}*/

bool SimpleSocket::Connect(string host, uint32 port)
{
	if(m_fd) // We already have an existing socket
		return false;

	// Populate the socket handle
	m_fd = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);

	struct hostent * ci = gethostbyname(host.c_str());
	if(ci == 0)
		return false;

	m_client.sin_family = ci->h_addrtype;
	m_client.sin_port = ntohs((u_short)port);
	memcpy(&m_client.sin_addr.s_addr, ci->h_addr_list[0], ci->h_length);

	u_long arg = 0;
	ioctlsocket(m_fd, FIONBIO, &arg);

	if(connect(m_fd, (const sockaddr*)&m_client, sizeof(m_client)) == -1)
	{
		m_fd = NULL;
		return false;
	}

	arg = 1;
	ioctlsocket(m_fd, FIONBIO, &arg);

	if( pSocketMgr == NULL )
		pSocketMgr = new SocketMgr;

	pSocketMgr->AddSocket(this);

	return true;
}

void SimpleSocket::Disconnect()
{
	shutdown(m_fd, SD_BOTH);
	closesocket(m_fd);
	m_fd = NULL;
}

void SocketMgr::AddSocket(SimpleSocket *pSocket)
{
	socket_lock.Acquire();
	m_sockets.insert(pSocket);
	socket_lock.Release();
}

void SocketMgr::RemoveSocket(SimpleSocket *pSocket)
{
	socket_lock.Acquire();
	m_sockets.erase(pSocket);
	socket_lock.Release();
}

SocketMgr::SocketMgr()
{
	Thread *p = new Thread(this);
	p->start();
}

void SocketMgr::Update()
{
	fd_set read_set;
	fd_set write_set;
	fd_set exception_set;
	set<SimpleSocket*>::iterator itr;
	SimpleSocket *s;
	int res;
	uint32 max_fd = 0;
	char buffer[65000];

	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 50000;			// 50ms
	

	for( ;; )
	{
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);
		FD_ZERO(&exception_set);

		socket_lock.Acquire();
		for( itr = m_sockets.begin(); itr != m_sockets.end(); ++itr )
		{
			if( (*itr)->m_fd >= max_fd )
				max_fd = uint32((*itr)->m_fd) + 1;

			if( (*itr)->m_outBuf.size() > 0 )
				FD_SET((*itr)->m_fd, &write_set);
			else
				FD_SET((*itr)->m_fd, &read_set);
		}
		socket_lock.Release();
		
		res = select(max_fd, &read_set, &write_set, &exception_set, &tv);

		socket_lock.Acquire();
		for( itr = m_sockets.begin(); itr != m_sockets.end(); )
		{
			s = *itr;
			++itr;

			if( FD_ISSET(s->m_fd, &exception_set) )
			{
				s->m_fd = NULL;
				continue;
			}

			if( FD_ISSET(s->m_fd, &read_set) )
			{
				res = recv(s->m_fd, buffer, 65000, 0);
				if( res <= 0 )
				{
					s->m_fd = NULL;
					continue;
				}

				s->buffer_lock.Acquire();
				s->m_inBuf.append(buffer, res);
				s->buffer_lock.Release();
			}

			if( FD_ISSET(s->m_fd, &write_set) && s->m_outBuf.size() )
			{
				s->buffer_lock.Acquire();
				res = send(s->m_fd, s->m_outBuf.c_str(), (int)s->m_outBuf.size(), 0);
				if( res <= 0 )
				{
					s->buffer_lock.Release();
					s->m_fd = NULL;
					continue;
				}

				s->m_outBuf.erase(0, res);
				s->buffer_lock.Release();
			}
		}

		socket_lock.Release();
	}
}

