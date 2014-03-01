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
#include <typeindex>

class NetMessage : public std::enable_shared_from_this<NetMessage>
{
public:
    virtual ~NetMessage(){}
    virtual RawMessage::pointer toRaw()const = 0;
    virtual unsigned int gettype()const = 0;
    virtual bool prefersUdp()const = 0;
    virtual bool isGameMessage()const = 0;

    enum Flags{
        MESSAGE_NO_FLAGS = 0,
        MESSAGE_PREFERS_UDP = 0x100,
        MESSAGE_OVERRIDES_PREVIOUS = 0x200,
        MESSAGE_HIGH_PRIORITY = 2,
    };

    virtual Flags messageFlags() const
    {
        if (prefersUdp())
            return MESSAGE_PREFERS_UDP;
        else
            return MESSAGE_NO_FLAGS;
    }

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

    // Special case for std::tuple<...>

    template<typename IndexType, typename... Types>
    struct outTupleImpl
    {
        static void exec(binaryStream& s, const std::tuple<Types...> &val){
            s << std::get<IndexType::value>(val);
            outTupleImpl<std::integral_constant<size_t, IndexType::value + 1>, Types...>::exec(s, val);
        }
    };

    template<typename... Types>
    struct outTupleImpl<std::integral_constant<size_t, std::tuple_size<std::tuple<Types...>>::value>, Types...>
    {
        static void exec(binaryStream& s, const std::tuple<Types...> &val){
            // nop
        }
    };

    template<typename... Types>
    binaryStream& operator<<(const std::tuple<Types...> &val){
        outTupleImpl<std::integral_constant<size_t, 0>, Types...>::exec(*this, val);
        return (*this);
    }

    template<typename T>
    binaryStream& operator<<(const std::vector<T> &val){
        uint32_t size = val.size();
        (*this) << size;
        for (uint32_t i = 0; i < size; ++i){
            (*this) << val[i];
        }
        return (*this);
    }

    /// Input

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

    // Special case for std::tuple<...>

    template<typename IndexType, typename... Types>
    struct inTupleImpl
    {
        static void exec(binaryStream& s, std::tuple<Types...> &val){
            s >> std::get<IndexType::value>(val);
            inTupleImpl<std::integral_constant<size_t, IndexType::value + 1>, Types...>::exec(s, val);
        }
    };

    template<typename... Types>
    struct inTupleImpl < std::integral_constant<size_t, std::tuple_size<std::tuple<Types...>>::value>, Types...>
    {
        static void exec(binaryStream& s, std::tuple<Types...> &val){
            // nop
        }
    };

    template<typename... Types>
    binaryStream& operator>>(std::tuple<Types...> &val){
        inTupleImpl<std::integral_constant<size_t, 0>, Types...>::exec(*this, val);
        return (*this);
    }

    template<typename T>
    binaryStream& operator>>(std::vector<T> &val){
        uint32_t size;
        (*this) >> size;
        val.resize(size);
        for (uint32_t i = 0; i < size; ++i){
            (*this) >> val[i];
        }
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

#define REGISTER_MESSAGE_BASE(name, prefix) unsigned int prefix##name##Message::mtype = MsgFactory::regFactory<prefix##name##Message>(prefix##name##Message::type);

#define REGISTER_NETMESSAGE(name) REGISTER_MESSAGE_BASE(name, Net)

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
    virtual bool send(const NetMessage::const_pointer& msg, MessagePeer* sender = nullptr);
    virtual std::string name() const = 0;

protected:
    // inform all connected(except yourself) peers about the message
    void broadcastLocally(const NetMessage::const_pointer&);
//private:
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

class MessageMetaData
{
public:
    MessageMetaData():
        m_flags(NetMessage::MESSAGE_NO_FLAGS)
    {

    }

    template <typename T>
    T& get(const NetMessage::pointer& obj, const std::string &name) const
    {
        assert(m_valueNames.count(name) > 0);
        unsigned int variableId = m_valueNames.at(name);
        // Control data type
        assert(m_valueTypes[variableId] == typeid(T));
        return *(reinterpret_cast<T*>(m_setFuncs[variableId](obj)));
    }

    template <typename T>
    const T& get(const NetMessage::const_pointer& obj, const std::string &name) const
    {
        assert(m_valueNames.count(name) > 0);
        unsigned int variableId = m_valueNames.at(name);
        // Control data type
        assert(m_valueTypes[variableId] == typeid(T));
        return *(reinterpret_cast<const T*>(m_getFuncs.at(variableId)(obj)));
    }

    template<typename MSGT, typename T, unsigned int ID>
    void declVariable(const std::string& varName)
    {
        m_valueNames[varName] = ID;
        m_valueTypes.emplace(m_valueTypes.begin() + ID, typeid(T));
        m_setFuncs.emplace(m_setFuncs.begin() + ID, [](const NetMessage::pointer& obj) -> void*
        {
            return &(std::get<ID>(obj->as<MSGT>()->m_values));
        });
        m_getFuncs.emplace(m_getFuncs.begin() + ID, [](const NetMessage::const_pointer& obj) -> const void*
        {
            return &(std::get<ID>(obj->as<MSGT>()->m_values));
        });
    }

    // IDtype here and everywhere else is a std::integral_constant<unsigned int, ID>.
    template <typename IDtype, typename MSGT>
    struct registerVariable
    {
        static void exec(MessageMetaData& md, const std::string& varNames)
        {
            typedef typename std::tuple_element<IDtype::value, typename MSGT::values_type>::type varType;
            std::string varName;
            // find (n-1)th newline symbol
            std::string::size_type begName = 0, endName;
            for (unsigned int i = 0; i < IDtype::value; ++i)
            {
                begName = varNames.find('\n', begName);
                begName += 1;
            }
            endName = varNames.find('\n', begName);
            varName = varNames.substr(begName, endName - begName);

            md.declVariable<MSGT, varType, IDtype::value>(varName);
            if (endName != std::string::npos)
                registerVariable<std::integral_constant<unsigned int, IDtype::value + 1>, MSGT>::exec(md, varNames);
        }
    };

    template <typename MSGT>
    struct registerVariable < std::integral_constant<unsigned int, std::tuple_size<typename MSGT::values_type>::value>, MSGT>
    {
        static void exec(MessageMetaData& md, const std::string& varNames)
        {
        }
    };

    template<typename MSGT>
    static MessageMetaData createMetaData(const std::string &varNames, int flags = NetMessage::MESSAGE_NO_FLAGS)
    {
        MessageMetaData result;
        registerVariable<std::integral_constant<unsigned int, 0>, MSGT>::exec(result, varNames);
        result.m_flags = static_cast<NetMessage::Flags>(flags);
        MSGT::postMetaInit(result);
        return result;
    }

    NetMessage::Flags flags() const
    {
        return m_flags;
    }

protected:
    std::map<std::string, unsigned int> m_valueNames;
    std::vector<std::type_index> m_valueTypes;
    std::vector<std::function<void*(const NetMessage::pointer&)>> m_setFuncs;
    std::vector<std::function<const void*(NetMessage::const_pointer)>> m_getFuncs;
    NetMessage::Flags m_flags;
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

template <typename BaseClass, int MessageTypeID, typename... Types>
class GenericNetMessage_Base : public BaseClass
{
public:
    typedef std::tuple<Types...> values_type;
    values_type m_values;

    typedef std::shared_ptr<GenericNetMessage_Base<BaseClass, MessageTypeID, Types...>> pointer;
    typedef std::shared_ptr<const GenericNetMessage_Base<BaseClass, MessageTypeID, Types...>> const_pointer;
    enum {type = MessageTypeID};

    template<typename T>
    T& get(const std::string& name)
    {
        return m_metaData.get<T>(shared_from_this(), name);
    }

    template<typename T>
    const T& get(const std::string& name) const
    {
        return m_metaData.get<T>(shared_from_this(), name);
    }

    unsigned int gettype() const override final
    {
        return MessageTypeID;
    }

    bool prefersUdp() const override final
    {
        return (messageFlags() & NetMessage::MESSAGE_PREFERS_UDP) != 0;
    }

    bool isGameMessage() const override
    {
        return false;
    }

    template<int Test = sizeof...(Types)>
    RawMessage::pointer toRawImpl(typename std::enable_if<(Test>0)>::type *test = 0) const
    {
        RawMessage::pointer result{ new RawMessage };
        result->msgType = MessageTypeID;
        binaryStream out;
        out << m_values;
        result->msgBytes = out.str();
        return result;
    }

    template<int Test = sizeof...(Types)>
    RawMessage::pointer toRawImpl(typename std::enable_if<(Test==0)>::type *test = 0) const
    {
        RawMessage::pointer result{ new RawMessage };
        result->msgType = MessageTypeID;
        return result;
    }

    RawMessage::pointer toRaw() const override final
    {
        return toRawImpl();
    }

    template<int Test = sizeof...(Types)>
    void fromRawImpl(const std::string& str, typename std::enable_if<(Test>0)>::type *test = 0)
    {
        binaryStream in(str);
        in >> m_values;
    }
    template<int Test = sizeof...(Types)>
    void fromRawImpl(const std::string& str, typename std::enable_if<(Test==0)> ::type *test = 0)
    {
    }

    void fromRaw(const std::string& str)
    {
        fromRawImpl(str);
    }

    static void postMetaInit(MessageMetaData& md)
    {

    }

    virtual NetMessage::Flags messageFlags() const override
    {
        return m_metaData.flags();
    }

    static MessageMetaData& meta()
    {
        return m_metaData;
    }

protected:
    static unsigned int mtype;

    static MessageMetaData m_metaData;
};

template<int MessageTypeId, typename... Types>
using GenericNetMessage = GenericNetMessage_Base<NetMessage, MessageTypeId, Types...>;
