#pragma once

#include "../gamecommon/GameObject.h"
#include <osg/Quat>
#include <chrono>

typedef GenericGameMessage<5002, osg::Vec3f, osg::Vec4f, uint32_t, std::string> GameSpaceShipConstructionDataMessage; // varNames: "pos", "orient", "ownerId", "playerName"
typedef GenericGameMessage<5003, uint16_t, bool> GameSpaceShipControlMessage; // varNames: "inputType", "isOn"
typedef GenericGameMessage<5004, osg::Vec3f, osg::Vec4f, osg::Vec3f> GameSpaceShipPhysicsUpdateMessage; // varNames: "pos", "orient", "velocity"
typedef GenericGameMessage<5005> GameSpaceShipCollisionMessage; // varNames:
typedef GenericGameMessage<5015, uint32_t> GameScoreUpdateMessage; // varNames: "score" (and object id)

class GameInstanceServer;
class PhysicsEngine;
class ShipPhysicsActor;

class SpaceShipServer : public GameObject
{
public:
    typedef std::shared_ptr<SpaceShipServer> pointer;
    enum inputType
    {
        ACCELERATE,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        BACK
    };

    SpaceShipServer(std::string playerName, osg::Vec3f startPos, osg::Quat orient, uint32_t ownerId, GameInstanceServer* context, const std::shared_ptr<PhysicsEngine>& eng);
    virtual ~SpaceShipServer();

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    unsigned int getPhysicsId();

    void incrementPlayerScore();
    int getPlayerScore();

    GameMessage::pointer creationMessage() const;

protected:
    std::string playerName;

    float m_acceleration;
    float m_steerability;

    int m_playerScore;

    bool m_inputState[6];
    std::shared_ptr<PhysicsEngine> m_engine;
    ShipPhysicsActor* m_actor;
    std::chrono::steady_clock::time_point m_lastUpdate;
    unsigned int m_physicsId;

    // event related functions
    void onControlMessage(uint16_t inputType, bool on);
    void onPhysicsUpdate(const osg::Vec3f& newPos, const osg::Quat& newRot);
};
