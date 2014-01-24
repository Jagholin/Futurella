#pragma once

#include "../gamecommon/GameObject.h"
#include "../gameserver/SpaceShipServer.h"

#include <osg/Group>

class GameInstanceClient;

class SpaceShipClient : public GameObject
{
public:
    typedef std::shared_ptr<SpaceShipClient> pointer;
    SpaceShipClient(osg::Vec3f pos, osg::Vec4f orient, uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx);

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    void setTransform(osg::Vec3f pos, osg::Vec4f orient);
    void tick(float deltaTime);
protected:
    osg::ref_ptr<osg::Group> m_rootGroup;
    osg::ref_ptr<osg::Node> m_shipNode;

    osg::ref_ptr<osg::MatrixTransform> m_transformGroup;
    static int m_dummy;
};
