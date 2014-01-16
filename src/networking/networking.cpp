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
	typedef auto_ptr<asio::ip::tcp::socket> socket_tcp;
    typedef auto_ptr<asio::ip::udp::socket> socket_udp;
	typedef std::deque<vector<char> > sendList;

	//static pointer Create(QString address);
	static pointer Create(NetConnection* b, boost::asio::io_service& s)
	{
		return pointer(new NetConnImpl(s, b));
	}
	bool connect(std::string addr, uint16_t portTCP, uint16_t portUDP);
	unsigned int writeTCP(RawMessage::const_pointer toSend);
    unsigned int writeUDP(RawMessage::const_pointer toSend);
	void close();
	bool isConnected()const
	{
		return connectedTCP;
	}
	std::string getAddrStr()const
	{
		std::stringstream formatStr;
		formatStr << mySockTCP->remote_endpoint().address().to_string() << ":" << mySockTCP->remote_endpoint().port();
        formatStr << ", " << mySockUDP->remote_endpoint().port();
		return formatStr.str();
	}
	void setSocket(socket_tcp&);
    void setSocket(socket_udp&);
	void disconnect()
	{
		buddy = 0;
		close();
	}
	~NetConnImpl()
	{
		//qDebug() << "Net connection private class destructor...";
    }

    void datagramReceived(RawMessage::pointer newMessage); // called by NetServerImpl;
    void doConnectTCP(const asio::ip::tcp::resolver::iterator& tcpBegin);
    void doConnectUDP(const asio::ip::udp::resolver::iterator& udpBegin);
protected:
	asio::io_service& mserv;
	socket_tcp mySockTCP;
    socket_udp mySockUDP;
	vector<char> onReceiving;
	sendList onSending;
	bool sending;
	bool connectedTCP, connectedUDP;
	unsigned int frontMsg, endMsg;
	NetConnection* buddy;

	//asio::ip::tcp::resolver::iterator mBegin, mEnd;
	NetConnImpl(asio::io_service& serv, NetConnection*);
	void doWrite(RawMessage::const_pointer);
    void onConnectTCP(const boost::system::error_code&, const asio::ip::tcp::resolver::iterator& tcpBegin);
    void onConnectUDP(const boost::system::error_code&, const asio::ip::udp::resolver::iterator& udpBegin);
	void onHeadReceived(const boost::system::error_code&, size_t, uint16_t*);
	void onBodyReceived(const boost::system::error_code&, size_t);
    void onMsgSent(const boost::system::error_code&, size_t);
    void onMsgSentUDP(const boost::system::error_code&, size_t);
};

class ConnectionFinder
{
    static std::map<asio::ip::address, NetConnImpl*> m_conMap;
public:
    static void addConnection(const asio::ip::address& key, NetConnImpl* conn)
    {
        m_conMap[key] = conn;
    }
    static NetConnImpl* getConnection(const asio::ip::address& key)
    {
        if (m_conMap.count(key) >= 1)
            return m_conMap[key];
        return nullptr;
    }
    static void eraseConnection(const asio::ip::address& key)
    {
        m_conMap.erase(key);
    }
};
std::map<asio::ip::address, NetConnImpl*> ConnectionFinder::m_conMap;

class NetServerImpl: public std::enable_shared_from_this<NetServerImpl>, public boost::noncopyable
{
public:
	typedef shared_ptr<NetServerImpl> pointer;
	typedef auto_ptr<asio::ip::tcp::socket> socket_tcp;
    typedef auto_ptr<asio::ip::udp::socket> socket_udp;
	typedef asio::ip::tcp::acceptor acceptor;

	static pointer Create(NetServer* buddy, boost::asio::io_service& s)
	{
		return pointer(new NetServerImpl(s, buddy));
	}
    boost::system::error_code beginListen(uint16_t portTCP, uint16_t portUDP);
	void stop();
	void disconnect()
	{
		buddy = 0;
		stop();
	}
	~NetServerImpl()
	{
        //nop
	}
protected:
	asio::io_service& mserv;
	socket_tcp mSocketTCP;
    socket_udp mSocketUDP;
	acceptor mAccept;
	NetServer* buddy;
	bool started;

    char mDatagramBuffer[512];
    asio::ip::udp::endpoint mDatagramSender;

	NetServerImpl(asio::io_service& serv, NetServer* b);
	void doListen();
	void onAccept(const boost::system::error_code&);
    void onDatagramReceive(const boost::system::error_code&, std::size_t);
};

NetConnImpl::NetConnImpl(asio::io_service& serv, NetConnection* b):
	mserv(serv),
	mySockTCP(new socket_tcp::element_type(serv)),
    mySockUDP(new socket_udp::element_type(serv)),
    sending(false), connectedTCP(false), connectedUDP(false),
	frontMsg(0), endMsg(0), buddy(b)
{
	//qDebug() << "Net connection private class constructor...";
}

bool NetConnImpl::connect(std::string addr, uint16_t portTCP, uint16_t portUDP)
{
	if (connectedTCP || connectedUDP)
		return false;
	boost::system::error_code ecTCP = asio::error::host_not_found, ecUDP = asio::error::host_not_found;
	asio::ip::tcp::resolver myResolver(mserv);
	asio::ip::tcp::resolver::query resQueryTCP(addr, boost::lexical_cast<std::string>(portTCP));
	asio::ip::tcp::resolver::iterator tcpBegin = myResolver.resolve(resQueryTCP, ecTCP);
    asio::ip::udp::resolver udpResolver(mserv);
    asio::ip::udp::resolver::query resQueryUDP(addr, boost::lexical_cast<std::string>(portUDP));
    asio::ip::udp::resolver::iterator udpBegin = udpResolver.resolve(resQueryUDP, ecUDP);
    asio::ip::tcp::resolver::iterator tcpEnd;
    asio::ip::udp::resolver::iterator udpEnd;

	if (tcpBegin != tcpEnd && udpBegin != udpEnd && !ecTCP && !ecUDP)
	{
		mserv.post(std::bind(&NetConnImpl::doConnectTCP, shared_from_this(), tcpBegin));
        mserv.post(std::bind(&NetConnImpl::doConnectUDP, shared_from_this(), udpBegin));
	}
	else
	{
		NetworkError err;
		err.str = boost::str(boost::format("Host not found: %s") % addr);
		if (buddy) buddy->onNetworkError(err);
	}
	return true;
}

void NetConnImpl::doConnectTCP(const asio::ip::tcp::resolver::iterator& tcpBegin)
{
    auto nextTcpEndpoint = tcpBegin;
    ++nextTcpEndpoint;
	mySockTCP->async_connect(*tcpBegin, 
		std::bind(&NetConnImpl::onConnectTCP, 
			shared_from_this(), 
			std::placeholders::_1, //asio::placeholders::error
            nextTcpEndpoint
			));
}

void NetConnImpl::onConnectTCP(const boost::system::error_code& code, const asio::ip::tcp::resolver::iterator& tcpBegin)
{
	if (!code)
	{
		mySockTCP->set_option(asio::socket_base::keep_alive(true));
		connectedTCP = true;
		uint16_t* header = new uint16_t;
		asio::async_read(*mySockTCP, asio::buffer(header, sizeof(uint16_t)),
				std::bind(&NetConnImpl::onHeadReceived,
						shared_from_this(),
						std::placeholders::_1, //asio::placeholders::error,
						std::placeholders::_2, //asio::placeholders::bytes_transferred,
						header));
        if (connectedUDP && buddy)
    		buddy->m_connected(/*buddy->myId*/);
	}
	else
	{
		mySockTCP->close();
		if (tcpBegin != asio::ip::tcp::resolver::iterator())
		{
            auto nextTcpEndpoint = tcpBegin;
            ++nextTcpEndpoint;
			mySockTCP->async_connect(*tcpBegin, 
				std::bind(&NetConnImpl::onConnectTCP, 
					shared_from_this(), 
					std::placeholders::_1, //asio::placeholders::error
                    nextTcpEndpoint
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

void NetConnImpl::doConnectUDP(const asio::ip::udp::resolver::iterator& udpBegin)
{
    auto nextUdpEndpoint = udpBegin;
    ++nextUdpEndpoint;
    mySockUDP->async_connect(*udpBegin,
        std::bind(&NetConnImpl::onConnectUDP,
        shared_from_this(),
        std::placeholders::_1, //asio::placeholders::error
        nextUdpEndpoint
        ));
}

void NetConnImpl::onConnectUDP(const boost::system::error_code& code, const asio::ip::udp::resolver::iterator& udpBegin)
{
    if (!code)
    {
        mySockUDP->set_option(asio::socket_base::keep_alive(true));
        connectedUDP = true;
        ConnectionFinder::addConnection(mySockUDP->remote_endpoint().address(), this);
        if (connectedTCP && buddy)
            buddy->m_connected(/*buddy->myId*/);
    }
    else
    {
        mySockUDP->close();
        if (udpBegin != asio::ip::udp::resolver::iterator())
        {
            auto nextUdpEndpoint = udpBegin;
            ++nextUdpEndpoint;
            mySockUDP->async_connect(*udpBegin,
                std::bind(&NetConnImpl::onConnectUDP,
                shared_from_this(),
                std::placeholders::_1, //asio::placeholders::error
                nextUdpEndpoint
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

void NetConnImpl::setSocket(socket_tcp& sock)
{
	//if (mySock)
	{
		mySockTCP->close();
		//delete mySock;
	}
	mySockTCP = sock;
	mySockTCP->set_option(asio::socket_base::keep_alive(true));
	connectedTCP = true;
	uint16_t* header = new uint16_t;
	asio::async_read(*mySockTCP, asio::buffer(header, sizeof(uint16_t)),
			std::bind(&NetConnImpl::onHeadReceived,
					shared_from_this(),
					std::placeholders::_1, //asio::placeholders::error,
					std::placeholders::_2, //asio::placeholders::bytes_transferred,
					header));
}

void NetConnImpl::setSocket(socket_udp& sock)
{
    //if (mySock)
    {
        if (connectedUDP)
            ConnectionFinder::eraseConnection(mySockUDP->remote_endpoint().address());
        mySockUDP->close();
        //delete mySock;
    }
    mySockUDP = sock;
    mySockUDP->set_option(asio::socket_base::keep_alive(true));
    connectedUDP = true;
    ConnectionFinder::addConnection(mySockUDP->remote_endpoint().address(), this);
}

unsigned int NetConnImpl::writeTCP(RawMessage::const_pointer toSend)
{
	if (!connectedTCP)
		return 0;
	mserv.post(std::bind(&NetConnImpl::doWrite, shared_from_this(), toSend));
	return endMsg++;
}

unsigned int NetConnImpl::writeUDP(RawMessage::const_pointer msg)
{
    if (!connectedUDP)
        return 0;
    // Create asio read buffer from message
    vector<char> toSend/*msg->msgBytes.size() + sizeof(uint32_t) + sizeof(uint16_t)*/;
    toSend.reserve(msg->msgBytes.size() + sizeof(uint32_t)+sizeof(uint16_t));
    // Composite send vector
    uint16_t nboSize = htons(msg->msgBytes.size() + sizeof(uint32_t));
    toSend.insert(toSend.end(), (char*)&nboSize, ((char*)&nboSize) + sizeof(uint16_t));
    uint32_t nboType = htonl(msg->msgType);
    toSend.insert(toSend.end(), (char*)&nboType, ((char*)&nboType) + sizeof(uint32_t));
    toSend.insert(toSend.end(), msg->msgBytes.begin(), msg->msgBytes.end());

    if (toSend.size() > 512)
    {
        if (buddy)
            buddy->m_warn("Packet size > 512 bytes, can't send via UDP, ignoring");
        return 0;
    }
    mySockUDP->async_send(asio::buffer(toSend), std::bind(&NetConnImpl::onMsgSentUDP,
        shared_from_this(),
        std::placeholders::_1, std::placeholders::_2));
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
		asio::async_write(*mySockTCP, asio::buffer(onSending.front()),
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
			asio::async_write(*mySockTCP, asio::buffer(onSending.front()),
					std::bind(&NetConnImpl::onMsgSent,
							shared_from_this(),
							std::placeholders::_1, //asio::placeholders::error,
							std::placeholders::_2 //asio::placeholders::bytes_transferred
							));
		if (buddy) buddy->m_messageSent(/*buddy->myId,*/ frontMsg++);
	}
	else
	{
		sending = false; connectedTCP = false;
		NetworkError err;
		err.str = code.message().c_str();
		if (buddy) buddy->onNetworkError(err);
	}
}

void NetConnImpl::onMsgSentUDP(const boost::system::error_code& code, size_t /*bytes_transferred*/)
{
    if (!code)
    {
        if (buddy) buddy->m_messageSent(/*buddy->myId,*/ frontMsg++);
    }
    else
    {
        sending = false; connectedUDP = false;
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
		asio::async_read(*mySockTCP, asio::buffer(onReceiving),
				std::bind(&NetConnImpl::onBodyReceived,
						shared_from_this(),
						std::placeholders::_1, //asio::placeholders::error,
						std::placeholders::_2 //::placeholders::bytes_transferred
						));
	}
	else
	{
		sending = false; connectedTCP = false;
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
		asio::async_read(*mySockTCP, asio::buffer(header, sizeof(uint16_t)),
				std::bind(&NetConnImpl::onHeadReceived,
						shared_from_this(),
						std::placeholders::_1, //asio::placeholders::error,
						std::placeholders::_2, //asio::placeholders::bytes_transferred,
						header));
		if (buddy) buddy->m_messageReceived(/*buddy->myId,*/ msg);
	}
	else
	{
		sending = false; connectedTCP = false;
		NetworkError err;
		err.str = code.message().c_str();
		if (buddy) buddy->onNetworkError(err);
	}
}

void NetConnImpl::datagramReceived(RawMessage::pointer newMessage)
{
    if (buddy) buddy->m_messageReceived(newMessage);
}

void NetConnImpl::close()
{
	if (mySockTCP->is_open())
	{
		mySockTCP->shutdown(asio::ip::tcp::socket::shutdown_both);
		mySockTCP->close();
	}
    if (mySockUDP->is_open())
    {
        ConnectionFinder::eraseConnection(mySockUDP->remote_endpoint().address());
        mySockUDP->close();
    }
	sending = false; connectedTCP = false;
}

NetConnection::NetConnection(boost::asio::io_service &s):
	privData(NetConnImpl::Create(this, s)),
    //m_messageSent("NetConnection::messageSent"),
    m_messageReceived("NetConnection::messageReceived"),
    m_connected("NetConnection::connected"),
    m_disconnected("NetConnection::disconnected"),
    m_warn("NetConnection::warn"),
	m_service(s)
{
    //nop
}

NetConnection::~NetConnection()
{
	privData->disconnect();
}

void NetConnection::connectTo(std::string addr, unsigned int portTCP, unsigned int portUDP)
{
	privData->connect(addr, portTCP, portUDP);
}

unsigned int NetConnection::sendMessage(RawMessage::const_pointer msg)
{
	return privData->writeTCP(msg);
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
	mSocketTCP(new socket_tcp::element_type(serv)),
    mSocketUDP(new socket_udp::element_type(serv)),
	mAccept(serv),
	buddy(b),
	started(false)
{
    //nop
}

boost::system::error_code NetServerImpl::beginListen(uint16_t portTCP, uint16_t portUDP)
{
	if (started)
		return boost::system::error_code();
	boost::system::error_code ec;
	mAccept.open(asio::ip::tcp::v4(), ec);
	if (ec) return ec;
	mAccept.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), portTCP), ec);
	if (ec) return ec;
    mSocketUDP->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), portUDP), ec);
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
		if (buddy) buddy->m_stopped();
		return;
	}
	mAccept.async_accept(*mSocketTCP,
			std::bind(&NetServerImpl::onAccept,
					shared_from_this(),
					std::placeholders::_1 //::placeholders::error
					));

    mSocketUDP->async_receive_from(asio::buffer(mDatagramBuffer, 512), mDatagramSender,
        std::bind(&NetServerImpl::onDatagramReceive, shared_from_this(),
        std::placeholders::_1, std::placeholders::_2));
}

void NetServerImpl::onAccept(const boost::system::error_code& code)
{
	if (!code)
	{
		NetConnection* newCon = new NetConnection(mserv);
		newCon->privData->setSocket(mSocketTCP);
		mSocketTCP.reset(new socket_tcp::element_type(mserv));

        asio::ip::udp::resolver udpResolver(mserv);
        auto udpEndpointIterator = udpResolver.resolve(asio::ip::udp::endpoint(mSocketTCP->remote_endpoint().address(), mSocketUDP->local_endpoint().port()));
        newCon->privData->doConnectUDP(udpEndpointIterator);

		if (buddy) buddy->m_newConnection(newCon);
		doListen();
	}
	else
	{
		started = false;
		if (buddy) buddy->m_stopped();
	}
}

void NetServerImpl::onDatagramReceive(const boost::system::error_code& ec, std::size_t datagramSize)
{
    if (!ec)
    {
        char* buffer = &(mDatagramBuffer[0]);
        if (datagramSize >= sizeof(uint16_t)+sizeof(uint32_t))
        {
            uint16_t packetSize = ntohs(*reinterpret_cast<uint16_t*>(buffer));
            assert(packetSize + sizeof(uint16_t) == datagramSize);
            uint32_t packetType = ntohl(*reinterpret_cast<uint32_t*>(buffer + sizeof(uint16_t)));
            // Create new RawMessage
            RawMessage::pointer newMessage(new RawMessage);
            newMessage->msgType = packetType;
            newMessage->msgBytes = std::string(buffer + sizeof(uint16_t) + sizeof(uint32_t), buffer + datagramSize);

            NetConnImpl* associatedConnection = ConnectionFinder::getConnection(mDatagramSender.address());
            if (associatedConnection)
            {
                associatedConnection->datagramReceived(newMessage);
            }
            else
            {
                if (buddy)
                    buddy->m_newDatagram(newMessage, mDatagramSender);
            }
        }

        mSocketUDP->async_receive_from(asio::buffer(mDatagramBuffer, 512), mDatagramSender,
            std::bind(&NetServerImpl::onDatagramReceive, shared_from_this(),
            std::placeholders::_1, std::placeholders::_2));
    }
    else
    {
        // TODO: error on datagram receive
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
    // nop
}

NetServer::~NetServer()
{
	privData->disconnect();
    // nop
}

bool NetServer::listen(unsigned int portTCP, unsigned int portUDP, std::string& errMsg)
{
	boost::system::error_code ec = privData->beginListen(portTCP, portUDP);
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