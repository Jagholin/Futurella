#pragma once

#include "../gamecommon/GameObject.h"
#include <osg/Quat>

BEGIN_DECLGAMEMESSAGE(GameInfoConstructionData, 5006, false)
uint32_t ownerId;
END_DECLGAMEMESSAGE()

BEGIN_DECLGAMEMESSAGE(RoundData, 5007, false)
osg::Vec3f startPoint;
osg::Vec3f finishAreaCenter;
float finishAreaRadius;
uint32_t ownerId; // why?
END_DECLGAMEMESSAGE()


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

    bool shipInFinishArea(osg::Vec3f shipPosition);
    void setObjective(osg::Vec3f start, osg::Vec3f finish, float finishRadius);

protected:

    osg::Vec3f m_startingPoint, m_finishArea;
    float m_finishAreaSize;

    // event related functions
    void onControlMessage(uint16_t inputType, bool on);
    void onPhysicsUpdate(const osg::Vec3f& newPos, const osg::Quat& newRot);
};
