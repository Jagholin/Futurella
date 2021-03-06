#pragma once

#include "../gamecommon/GameObject.h"
#include "../gameserver/SpaceShipServer.h"

#include <osg/Group>
#include <boost/asio/io_service.hpp>

class GameInstanceClient;
class ChaseCam;

class SpaceShipClient : public GameObject
{
public:
    typedef std::shared_ptr<SpaceShipClient> pointer;
    SpaceShipClient(std::string playerName, osg::Vec3f pos, osg::Vec4f orient, uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx);
    virtual ~SpaceShipClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    void setTransform(osg::Vec3f pos, osg::Vec4f orient);
    void tick(float deltaTime);

    // Methods used by ChaseCam
    void sendInput(SpaceShipServer::inputType inType, bool isOn);
    osg::Quat getOrientation();
    osg::Vec3f getPivotLocation();

    std::string getPlayerName();
    int getPlayerScore();

protected:
    osg::ref_ptr<osg::Group> m_rootGroup;
    osg::ref_ptr<osg::Node> m_shipNode;

    osg::ref_ptr<osg::MatrixTransform> m_transformGroup;
    osg::Vec3f m_projVelocity;
    osg::Vec3f m_lastPosition;
    osg::Vec4f m_lastOrientation;
    bool m_inputCache[6];
    static int m_dummy;

    std::string m_playerName;
    int m_score;

    boost::asio::io_service m_shipUpdateService;
};
