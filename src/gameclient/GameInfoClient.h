#pragma once

#include "../gamecommon/GameObject.h"
#include "../gameserver/GameInfoServer.h"

#include <osg/Group>

class GameInstanceClient;

class GameInfoClient : public GameObject
{
public:
    typedef std::shared_ptr<GameInfoClient> pointer;

    GameInfoClient(uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx);
    virtual ~GameInfoClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);
    osg::Vec3f finishArea() const;

protected:

    osg::ref_ptr<osg::Group> m_rootGroup;
    osg::ref_ptr<osg::Node> m_finishAreaNode;

    osg::ref_ptr<osg::MatrixTransform> m_transformGroup;

    osg::Vec3f m_startingPoint, m_finishArea;
    float m_finishAreaSize;

    static int m_dummy;
};
