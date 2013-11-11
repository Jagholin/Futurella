/*
	networking.cpp
	Created on: Feb 16, 2009
	Copyright (C) 2009 - 2014 Pavel Belskiy, github.com/Jagholin
 */

#include "networking.h"
#ifdef _WIN32
#include <winsock2.h>
#ifdef _MSC_VER
// Visual Studio doesn't recognize *_t types
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#define BOOST_DATE_TIME_NO_LIB
#define BOOST_REGEX_NO_LIB
#endif
#else
#include <netinet/in.h>
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

class CService
{
protected:
	static asio::io_service myService;
	//static asio::io_service::work* myWork;
	static volatile bool isRunning;
public:
	static asio::io_service& getService()
	{
		return myService;
	}
	static bool getRunning()
	{
		return isRunning;
	}
	static void run()
	{
		if (!isRunning)
		{
			//myWork = new asio::io_service::work(myService);
			isRunning = true;
			myService.run();
			myService.reset();
			isRunning = false;
		}
	}
	static void stop()
	{
		myService.stop();
		isRunning = false;
		//delete myWork;
	}
};
//asio::io_service::work* CService::myWork = 0;
asio::io_service CService::myService;
volatile bool CService::isRunning = false;

class NetConnImpl : public std::enable_shared_from_this<NetConnImpl>, public boost::noncopyable
{
public:
	typedef shared_ptr<NetConnImpl> pointer;
	typedef auto_ptr<asio::ip::tcp::socket> socket;
	typedef std::deque<vector<char> > sendList;

	//static pointer Create(QString address);
	static pointer Create(NetConnection* b)
	{
		return pointer(new NetConnImpl(CService::getService(), b));
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

	static pointer Create(NetServer* buddy)
	{
		return pointer(new NetServerImpl(CService::getService(), buddy));
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
	//asio::error_code ec = asio::error::host_not_found;
	asio::ip::tcp::resolver myResolver(mserv);
	asio::ip::tcp::resolver::query resQuery(addr, boost::lexical_cast<std::string>(port));
	mBegin = myResolver.resolve(resQuery);

	if (mBegin != mEnd)
	{
		//asio::ip::tcp::endpoint* ep = new asio::ip::tcp::endpoint(asio::ip::address::from_string(addr.toStdString()), port);
		mserv.post(std::bind(&NetConnImpl::doConnect, shared_from_this()));
		//mySock.async_connect(ep, boost::bind(NetConnImpl::onConnect&, shared_from_this(), asio::placeholders::error));
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
		if(buddy) buddy->onConnected(/*buddy->myId*/);
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
		if (buddy) buddy->onMessageSent(/*buddy->myId,*/ frontMsg++);
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
		if (buddy) buddy->onMessageReceived(/*buddy->myId,*/ msg);
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

NetConnection::NetConnection():
	privData(NetConnImpl::Create(this))
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
	onDisconnect(/*myId,*/ err.str);
}

void NetConnection::scheduleDeletion()
{
	// TODO: This code will schedule the deletion to the service if it is still running.
	// However, it doesn't guarantee that the delete procedures will be fulfilled, in case when the service is stopped.
	// (use shared pointers to this? Not to make use of stop() function? Something else?)
	if (CService::getRunning())
		CService::getService().post(std::bind(&NetConnection::deleteNow, this));
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
		if (buddy) buddy->onStop();
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
		NetConnection* newCon = new NetConnection();
		newCon->privData->setSocket(mSocket);
		mSocket.reset(new socket::element_type(mserv));
		if (buddy) buddy->onConnection(newCon);
		doListen();
	}
	else
	{
		started = false;
		//qDebug() << "Server stopped: " << QString::fromStdString(code.message());
		if (buddy) buddy->onStop();
	}
}

void NetServerImpl::stop()
{
	mAccept.close();
}

NetServer::NetServer():
	privData(NetServerImpl::Create(this))
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
		onStop();
		return false;
	}
	return true;
}

void NetServer::stop()
{
	privData->stop();
}