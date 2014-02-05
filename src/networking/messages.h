/*
    Copyright (C) 2009 - 2014 Pavel Belskiy, github.com/Jagholin
 */
#pragma once

#include <map>
#include <vector>
#include <deque>
#include <memory>
#include <sstream>
#include "networking.h"
#include <osg/Vec3>
#include <osg/Vec4>

class NetMessage : public std::enable_shared_from_this<NetMessage>
{
public:
    virtual ~NetMessage(){}
    virtual RawMessage::pointer toRaw()const = 0;
    virtual unsigned int gettype()const = 0;
    virtual bool prefersUdp()const = 0;
    virtual bool isGameMessage()const = 0;

    template <typename T> typename T::pointer as_safe() { return std::dynamic_pointer_cast<T>(shared_from_this()); } 
    template <typename T> typename T::pointer as() { return std::static_pointer_cast<T>(shared_from_this()); } 
    template <typename T> typename T::const_pointer as_safe() const { return std::dynamic_pointer_cast<const T>(shared_from_this()); } 
    template <typename T> typename T::const_pointer as() const { return std::static_pointer_cast<const T>(shared_from_this()); } 

    typedef std::shared_ptr<NetMessage> pointer;
    typedef std::shared_ptr<const NetMessage> const_pointer;
};

class binaryStream
{
    std::stringstream m_ss;
public:
    binaryStream(){

    };

    binaryStream(const std::string& s) : m_ss(s) {

    }

    template<typename T>
    binaryStream& operator<<(const T& val){
        m_ss.write(reinterpret_cast<const char*>(&val), sizeof(T));
        return *this;
    }

    //special case for std::string
    template <typename T>
    binaryStream& operator<<(const std::basic_string<T>& val){
        int16_t size = static_cast<int16_t>(val.size()*sizeof(T));
        m_ss.write(reinterpret_cast<const char*>(&size), 2);
        m_ss.write(reinterpret_cast<const char*>(val.c_str()), size);
        return *this;
    }

    binaryStream& operator<<(const char* val){
        return operator<<(std::string(val));
    }

    binaryStream& operator<<(const osg::Vec3f &val){
        (*this) << val.x() << val.y() << val.z();
        return *this;
    }

    binaryStream& operator<<(const osg::Vec4f &val){
        (*this) << val.x() << val.y() << val.z() << val.w();
        return (*this);
    }

    template<typename T>
    binaryStream& operator>>(T& val){
        m_ss.read(reinterpret_cast<char*>(&val), sizeof(T));
        return *this;
    }

    template <typename T>
    binaryStream& operator>>(std::basic_string<T>& val){
        int16_t size(0);
        m_ss.read(reinterpret_cast<char*>(&size), 2);
        char* buf = new char[size];
        m_ss.read(buf, size);
        val.assign(buf, size);
        return *this;
    }

    binaryStream& operator>>(osg::Vec3f& val){
        osg::Vec3f::value_type x, y, z;
        (*this) >> x >> y >> z;
        val.set(x, y, z);
        return (*this);
    }

    binaryStream& operator>>(osg::Vec4f& val){
        osg::Vec4f::value_type x, y, z, w;
        (*this) >> x >> y >> z >> w;
        val.set(x, y, z, w);
        return (*this);
    }
    
    std::string str()const{
        return m_ss.str();
    }
};

#define DECLMESSAGE_BASE(name, number, udppref, isgamemsg, prefix, baseclass) class prefix##name##Message : public baseclass { \
    public: typedef std::shared_ptr<prefix##name##Message> pointer; \
    typedef std::shared_ptr<const prefix##name##Message> const_pointer; \
    public: RawMessage::pointer toRaw()const; \
    enum { type = (number) }; \
    protected: static unsigned int mtype; \
    public: unsigned int gettype()const { return mtype; } \
    bool prefersUdp()const { return (udppref); } \
    bool isGameMessage()const { return (isgamemsg); } \
    void fromRaw(const std::string&);

#define BEGIN_DECLNETMESSAGE(name, number, udppref) DECLMESSAGE_BASE(name, number, udppref, false, Net, NetMessage)

#define END_DECLNETMESSAGE() };

#define REGISTER_MESSAGE_BASE(name, prefix) unsigned int prefix##name##Message::mtype = MsgFactory::regFactory<prefix##name##Message>(prefix##name##Message::type);

#define REGISTER_NETMESSAGE(name) REGISTER_MESSAGE_BASE(name, Net)

#define BEGIN_TORAWMESSAGE_QCONVERTBASE(name, prefix) RawMessage::pointer prefix##name##Message::toRaw() const { \
    RawMessage::pointer result(new RawMessage); \
    result->msgType = mtype; \
    binaryStream out; {

#define BEGIN_NETTORAWMESSAGE_QCONVERT(name) BEGIN_TORAWMESSAGE_QCONVERTBASE(name, Net)

#define END_NETTORAWMESSAGE_QCONVERT() } result->msgBytes = out.str(); \
    return result; }

#define BEGIN_TONETMESSAGE_QCONVERTBASE(name, prefix) void prefix##name##Message::fromRaw(const std::string& str) { \
    binaryStream in(str);  

#define BEGIN_RAWTONETMESSAGE_QCONVERT(name) BEGIN_TONETMESSAGE_QCONVERTBASE(name, Net)

#define END_RAWTONETMESSAGE_QCONVERT() }

class MessagePeer
{
protected:
    std::vector<MessagePeer*> m_localPeers;
    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*) = 0;
public:
    MessagePeer();
    virtual ~MessagePeer();

    // Connects to another MessagePeer in a local communication net
    virtual void connectLocallyTo(MessagePeer* buddy, bool recursive = true);
    // and disconnects
    virtual void disconnectLocallyFrom(MessagePeer* buddy, bool recursive = true);

    // send a message to *this* MessagePeer.
    bool send(const NetMessage::const_pointer& msg, MessagePeer* sender = nullptr);
    virtual std::string name() const = 0;

protected:
    // inform all connected(except yourself) peers about the message
    void broadcastLocally(const NetMessage::const_pointer&);
private:
    typedef std::pair<NetMessage::const_pointer, MessagePeer*> workPair;
    std::deque<workPair> m_messageQueue;
    bool m_isWorking;
};

class LocalMessagePeer: public MessagePeer
{
protected:
    // you need to provide this function to make use of this class
    // virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);

    std::string name() const;
};

class MsgFactory
{
public:
    typedef std::function<NetMessage::const_pointer(const std::string&)> factoryFunc;
    typedef std::map<unsigned int, factoryFunc > factoryMap;
private:
    static factoryMap msgFactories;
    MsgFactory();
public:
    template <typename T>
    static unsigned int regFactory(unsigned int id)
    {
        assert(msgFactories.count(id) == 0);
        msgFactories[id] = [](const std::string& str) -> NetMessage::const_pointer {
            typename T::pointer tempObj{ new T };
            tempObj->fromRaw(str);
            return tempObj;
        };
        return id;
    }
    static NetMessage::const_pointer create(const RawMessage& msg)
    {
        if (msgFactories.count(msg.msgType) > 0)
            return msgFactories[msg.msgType](msg.msgBytes);
        throw std::bad_cast();
    }
};

template <typename... Types>
class GenericMessage : public NetMessage
{
protected:
    std::tuple<Types...> m_values;

public:

};
