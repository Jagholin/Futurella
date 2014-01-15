/*
	networking.cpp
	Created on: Feb 16, 2009
	Copyright (C) 2009 - 2014 Pavel Belskiy, github.com/Jagholin
 */

#include "networking.h"
#ifdef _WIN32
#	include <winsock2.h>
#	ifdef _MSC_VER
#		define BOOST_DATE_TIME_NO_LIB
#		define BOOST_REGEX_NO_LIB
#	endif
#else
#	include <netinet/in.h>
#endif

#include <algorithm> // for min, max
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <deque>
#include <memory>
#include <functional>

using std::shared_ptr;
using std::auto_ptr;
namespace asio=boost::asio;

class NetConnImpl : public std::enable_shared_from_this<NetConnImpl>, public boost::noncopyable
{
public:
	typedef shared_ptr<NetConnImpl> pointer;
	typedef auto_ptr<asio::ip::tcp::socket> socket;
	typedef std::deque<vector<char> > sendList;

	//static pointer Create(QString address);
	static pointer Create(NetConnection* b, boost::asio::io_service& s)
	{
		return pointer(new NetConnImpl(s, b));
	}
	bool connect(std::string addr, uint16_t port);
	unsigned int write(RawMessage::const_pointer toSend);
	void close();
	bool isConnected()const
	{
		return connected;
	}
	std::string getAddrStr()const
	{
		std::stringstream formatStr;
		formatStr << mySock->remote_endpoint().address().to_string() << ":" << mySock->remote_endpoint().port();
		return formatStr.str();
	}
	void setSocket(socket&);
	void disconnect()
	{
		buddy = 0;
		close();
	}
	~NetConnImpl()
	{
		//qDebug() << "Net connection private class destructor...";
	}
protected:
	asio::io_service& mserv;
	socket mySock;
	vector<char> onReceiving;
	sendList onSending;
	bool sending;
	bool connected;
	unsigned int frontMsg, endMsg;
	NetConnection* buddy;

	asio::ip::tcp::resolver::iterator mBegin, mEnd;
	NetConnImpl(asio::io_service& serv, NetConnection*);
	void doConnect();
	void doWrite(RawMessage::const_pointer);
	void onConnect(const boost::system::error_code&);
	void onHeadReceived(const boost::system::error_code&, size_t, uint16_t*);
	void onBodyReceived(const boost::system::error_code&, size_t);
	void onMsgSent(const boost::system::error_code&, size_t);
};

class NetServerImpl: public std::enable_shared_from_this<NetServerImpl>, public boost::noncopyable
{
public:
	typedef shared_ptr<NetServerImpl> pointer;
	typedef auto_ptr<asio::ip::tcp::socket> socket;
	typedef asio::ip::tcp::acceptor acceptor;

	static pointer Create(NetServer* buddy, boost::asio::io_service& s)
	{
		return pointer(new NetServerImpl(s, buddy));
	}
	boost::system::error_code beginListen(uint16_t port);
	void stop();
	void disconnect()
	{
		buddy = 0;
		stop();
	}
	~NetServerImpl()
	{
		//qDebug() << "Net server private class destructor...";
	}
protected:
	asio::io_service& mserv;
	socket mSocket;
	acceptor mAccept;
	NetServer* buddy;
	bool started;

	NetServerImpl(asio::io_service& serv, NetServer* b);
	void doListen();
	void onAccept(const boost::system::error_code&);
};

NetConnImpl::NetConnImpl(asio::io_service& serv, NetConnection* b):
	mserv(serv),
	mySock(new socket::element_type(serv)),
	sending(false), connected(false),
	frontMsg(0), endMsg(0), buddy(b)
{
	//qDebug() << "Net connection private class constructor...";
}

bool NetConnImpl::connect(std::string addr, uint16_t port)
{
	if (connected)
		return false;
	boost::system::error_code ec = asio::error::host_not_found;
	asio::ip::tcp::resolver myResolver(mserv);
	asio::ip::tcp::resolver::query resQuery(addr, boost::lexical_cast<std::string>(port));
	mBegin = myResolver.resolve(resQuery, ec);

	if (mBegin != mEnd && !ec)
	{
		mserv.post(std::bind(&NetConnImpl::doConnect, shared_from_this()));
	}
	else
	{
		NetworkError err;
		err.str = boost::str(boost::format("Host not found: %s") % addr);
		if (buddy) buddy->onNetworkError(err);
	}
	return true;
}

void NetConnImpl::doConnect()
{
	mySock->async_connect(*mBegin++, 
		std::bind(&NetConnImpl::onConnect, 
			shared_from_this(), 
			std::placeholders::_1 //asio::placeholders::error
			));
}

void NetConnImpl::onConnect(const boost::system::error_code& code)
{
	if (!code)
	{
		mySock->set_option(asio::socket_base::keep_alive(true));
		connected = true;
		uint16_t* header = new uint16_t;
		asio::async_read(*mySock, asio::buffer(header, sizeof(uint16_t)),
				std::bind(&NetConnImpl::onHeadReceived,
						shared_from_this(),
						std::placeholders::_1, //asio::placeholders::error,
						std::placeholders::_2, //asio::placeholders::bytes_transferred,
						header));
		if(buddy) buddy->m_connected(/*buddy->myId*/);
	}
	else
	{
		mySock->close();
		if (mBegin != mEnd)
		{
			mySock->async_connect(*mBegin++, 
				std::bind(&NetConnImpl::onConnect, 
					shared_from_this(), 
					std::placeholders::_1 //asio::placeholders::error
					));
		}
		else
		{
			NetworkError err;
			err.str = code.message().data();
			if (buddy) buddy->onNetworkError(err);
		}
	}
}

void NetConnImpl::setSocket(socket& sock)
{
	//if (mySock)
	{
		mySock->close();
		//delete mySock;
	}
	mySock = sock;
	mySock->set_option(asio::socket_base::keep_alive(true));
	connected = true;
	uint16_t* header = new uint16_t;
	asio::async_read(*mySock, asio::buffer(header, sizeof(uint16_t)),
			std::bind(&NetConnImpl::onHeadReceived,
					shared_from_this(),
					std::placeholders::_1, //asio::placeholders::error,
					std::placeholders::_2, //asio::placeholders::bytes_transferred,
					header));
}

unsigned int NetConnImpl::write(RawMessage::const_pointer toSend)
{
	if (!connected)
		return 0;
	mserv.post(std::bind(&NetConnImpl::doWrite, shared_from_this(), toSend));
	return endMsg++;
}

void NetConnImpl::doWrite(RawMessage::const_pointer msg)
{
	vector<char> toSend/*msg->msgBytes.size() + sizeof(uint32_t) + sizeof(uint16_t)*/;
	toSend.reserve(msg->msgBytes.size() + sizeof(uint32_t) + sizeof(uint16_t));
	// Composite send vector
	uint16_t nboSize = htons(msg->msgBytes.size() + sizeof(uint32_t));
	toSend.insert(toSend.end(), (char*)&nboSize, ((char*)&nboSize) + sizeof(uint16_t));
	uint32_t nboType = htonl(msg->msgType);
	toSend.insert(toSend.end(), (char*)&nboType, ((char*)&nboType) + sizeof(uint32_t));
	toSend.insert(toSend.end(), msg->msgBytes.begin(), msg->msgBytes.end());
	onSending.push_back(toSend);
	if (sending == false)
	{
		asio::async_write(*mySock, asio::buffer(onSending.front()),
				std::bind(&NetConnImpl::onMsgSent,
						shared_from_this(),
						std::placeholders::_1, //asio::placeholders::error,
						std::placeholders::_2 //asio::placeholders::bytes_transferred
						));
		sending = true;
	}
}

void NetConnImpl::onMsgSent(const boost::system::error_code& code, size_t /*bytes_transferred*/)
{
	onSending.pop_front();
	if (!code)
	{
		if (onSending.empty())
			sending = false;
		else
			asio::async_write(*mySock, asio::buffer(onSending.front()),
					std::bind(&NetConnImpl::onMsgSent,
							shared_from_this(),
							std::placeholders::_1, //asio::placeholders::error,
							std::placeholders::_2 //asio::placeholders::bytes_transferred
							));
		if (buddy) buddy->m_messageSent(/*buddy->myId,*/ frontMsg++);
	}
	else
	{
		sending = false; connected = false;
		NetworkError err;
		err.str = code.message().c_str();
		if (buddy) buddy->onNetworkError(err);
	}
}

void NetConnImpl::onHeadReceived(const boost::system::error_code& code, size_t /*bytes*/, uint16_t* size)
{
	if (!code)
	{
		uint16_t realSize = ntohs(*size);
		onReceiving.resize(realSize);
		delete size;
		asio::async_read(*mySock, asio::buffer(onReceiving),
				std::bind(&NetConnImpl::onBodyReceived,
						shared_from_this(),
						std::placeholders::_1, //asio::placeholders::error,
						std::placeholders::_2 //::placeholders::bytes_transferred
						));
	}
	else
	{
		sending = false; connected = false;
		NetworkError err;
		delete size;
		err.str = code.message().c_str();
		if (buddy) buddy->onNetworkError(err);
	}
}

void NetConnImpl::onBodyReceived(const boost::system::error_code& code, size_t /*bytes*/)
{
	if (!code)
	{
		RawMessage::pointer msg (new RawMessage);
		uint32_t* rawType = (uint32_t*)&(*onReceiving.begin());
		msg->msgType = ntohl(*rawType);
		msg->msgBytes = std::string(onReceiving.begin() + sizeof(uint32_t), onReceiving.end());

		uint16_t* header = new uint16_t;
		asio::async_read(*mySock, asio::buffer(header, sizeof(uint16_t)),
				std::bind(&NetConnImpl::onHeadReceived,
						shared_from_this(),
						std::placeholders::_1, //asio::placeholders::error,
						std::placeholders::_2, //asio::placeholders::bytes_transferred,
						header));
		if (buddy) buddy->m_messageReceived(/*buddy->myId,*/ msg);
	}
	else
	{
		sending = false; connected = false;
		NetworkError err;
		err.str = code.message().c_str();
		if (buddy) buddy->onNetworkError(err);
	}
}

void NetConnImpl::close()
{
	if (mySock->is_open())
	{
		mySock->shutdown(asio::ip::tcp::socket::shutdown_both);
		mySock->close();
	}
	sending = false; connected = false;
}

NetConnection::NetConnection(boost::asio::io_service &s):
	privData(NetConnImpl::Create(this, s)),
    //m_messageSent("NetConnection::messageSent"),
    m_messageReceived("NetConnection::messageReceived"),
    m_connected("NetConnection::connected"),
    m_disconnected("NetConnection::disconnected"),
	m_service(s)
{
	//qDebug() << "Net connection class constructor...";
}

NetConnection::~NetConnection()
{
	//qDebug() << "Net connection class destructor...";
	privData->disconnect();
}

void NetConnection::connectTo(std::string addr, unsigned int port)
{
	privData->connect(addr, port);
}

unsigned int NetConnection::sendMessage(RawMessage::const_pointer msg)
{
	return privData->write(msg);
}

std::string NetConnection::getAddrString()const
{
	return privData->getAddrStr();
}

bool NetConnection::isActive()const
{
	return privData->isConnected();
}

/*void NetConnection::setId(unsigned int id)
{
	myId = id;
}*/

void NetConnection::close()
{
	privData->close();
}

void NetConnection::onNetworkError(const NetworkError& err)
{
	close();
	m_disconnected(/*myId,*/ err.str);
}

void NetConnection::scheduleDeletion()
{
	// TODO: This code will schedule the deletion to the service if it is still running.
	// However, it doesn't guarantee that the delete procedures will be fulfilled, in case when the service is stopped.
	// (use shared pointers to this? Not to make use of stop() function? Something else?)
	if (!m_service.stopped())
		m_service.post(std::bind(&NetConnection::deleteNow, this));
	else
		// The service isn't running any more, so destroying the object directly.
		deleteNow();
}

void NetConnection::deleteNow()
{
	delete this;
}

NetServerImpl::NetServerImpl(asio::io_service& serv, NetServer* b):
	mserv(serv),
	mSocket(new socket::element_type(serv)),
	mAccept(serv),
	buddy(b),
	started(false)
{
	//qDebug() << "Net server private class constructor...";
}

boost::system::error_code NetServerImpl::beginListen(uint16_t port)
{
	if (started)
		return boost::system::error_code();
	boost::system::error_code ec;
	mAccept.open(asio::ip::tcp::v4(), ec);
	if (ec) return ec;
	mAccept.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port), ec);
	if (ec) return ec;
	mserv.post(std::bind(&NetServerImpl::doListen, shared_from_this()));
	started = true;
	return ec;
}

void NetServerImpl::doListen()
{
	boost::system::error_code ec;
	mAccept.listen(asio::socket_base::max_connections, ec);
	if (ec)
	{
		started = false;
		//qDebug() << "Server stopped: " << QString::fromStdString(ec.message());
		if (buddy) buddy->m_stopped();
		return;
	}
	mAccept.async_accept(*mSocket,
			std::bind(&NetServerImpl::onAccept,
					shared_from_this(),
					std::placeholders::_1 //::placeholders::error
					));
}

void NetServerImpl::onAccept(const boost::system::error_code& code)
{
	if (!code)
	{
		NetConnection* newCon = new NetConnection(mserv);
		newCon->privData->setSocket(mSocket);
		mSocket.reset(new socket::element_type(mserv));
		if (buddy) buddy->m_newConnection(newCon);
		doListen();
	}
	else
	{
		started = false;
		//qDebug() << "Server stopped: " << QString::fromStdString(code.message());
		if (buddy) buddy->m_stopped();
	}
}

void NetServerImpl::stop()
{
	mAccept.close();
}

NetServer::NetServer(boost::asio::io_service &s):
	privData(NetServerImpl::Create(this, s)),
    m_newConnection("NetServer::newConnection"),
    m_stopped("NetServer::stopped")
{
	//qDebug() << "Net server class constructor...";
}

NetServer::~NetServer()
{
	privData->disconnect();
	//qDebug() << "Net server class destructor...";
}

bool NetServer::listen(unsigned int port, std::string& errMsg)
{
	boost::system::error_code ec = privData->beginListen(port);
	if (ec)
	{
		errMsg = ec.message();
		m_stopped();
		return false;
	}
	return true;
}

void NetServer::stop()
{
	privData->stop();
}