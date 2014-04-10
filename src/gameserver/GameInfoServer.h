#pragma once

#include "../gamecommon/GameObject.h"
#include <Magnum/Magnum.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Vector4.h>

using namespace Magnum;

// BEGIN_DECLGAMEMESSAGE(GameInfoConstructionData, 5006, false)
// uint32_t ownerId;
// END_DECLGAMEMESSAGE()
// 
// BEGIN_DECLGAMEMESSAGE(RoundData, 5007, false)
// osg::Vec3f startPoint;
// osg::Vec3f finishAreaCenter;
// float finishAreaRadius;
// uint32_t ownerId; // why?
// END_DECLGAMEMESSAGE()

typedef GenericGameMessage<5006, uint32_t> GameGameInfoConstructionDataMessage; // varNames: "ownerId"
typedef GenericGameMessage<5007, Vector3, float, uint32_t, uint32_t> GameRoundDataMessage; // varNames: "finishAreaCenter", "finishAreaRadius", "numberOfPlayers", "ownerId"
typedef GenericGameMessage<5008> GameEndGameMessage; // maybe sent information like game time,...

class GameInstanceServer;

class GameInfoServer : public GameObject
{
public:
    typedef std::shared_ptr<GameInfoServer> pointer;

    GameInfoServer(uint32_t ownerId, GameInstanceServer* context);
    virtual ~GameInfoServer();

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    GameMessage::pointer objectiveMessage() const;
    GameMessage::pointer creationMessage() const;
    GameMessage::pointer gameOverMessage() const;

    bool shipInFinishArea(Vector3 shipPosition);
    void setObjective(Vector3 finish, float finishRadius);

    void setGameRunning(bool b);
    bool getGameRunning();

protected:
    bool m_gameRunning;

    Vector3 m_finishArea;
    float m_finishAreaSize;

    // event related functions
    void onControlMessage(uint16_t inputType, bool on);
    void onPhysicsUpdate(const Vector3& newPos, const Quaternion& newRot);
};
