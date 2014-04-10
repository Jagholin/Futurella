#pragma once

#include "../gamecommon/GameObject.h"
#include "../gameserver/SpaceShipServer.h"

//#include <osg/Group>
#include <Magnum/Math/Quaternion.h>
#include <boost/asio/io_service.hpp>

class GameInstanceClient;
class ChaseCam;

class SpaceShipClient : public GameObject
{
public:
    typedef std::shared_ptr<SpaceShipClient> pointer;
    SpaceShipClient(std::string playerName, Vector3 pos, Quaternion orient, uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx);
    virtual ~SpaceShipClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    void setTransform(const Vector3& pos, const Quaternion& orient);
    void tick(float deltaTime);

    // Methods used by ChaseCam
    void sendInput(SpaceShipServer::inputType inType, bool isOn);
    Quaternion getOrientation();
    Vector3 getPivotLocation();

    std::string getPlayerName();
    int getPlayerScore();

protected:
    Object3D* m_rootGroup;
    Object3D* m_shipNode;

    Matrix4 m_transformGroup;
    Vector3 m_projVelocity;
    Vector3 m_lastPosition;
    Quaternion m_lastOrientation;
    bool m_inputCache[6];
    static int m_dummy;

    std::string m_playerName;
    int m_score;

    boost::asio::io_service m_shipUpdateService;
};
