#pragma once 

#include <list>
#include <vector>
#include <osg/Group>
#include "networking/messages.h"

class AsteroidField;

class Asteroid;
class SpaceShip;

class Level : public LocalMessagePeer//level information
{
protected:

public:
    Level(int numberOfAsteroids, float turbulence, float density, osg::Group* levelGroup); //generates level
    ~Level();

    int getAsteroidLength();
    Asteroid* getAsteroid(int asteroidId);

    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);

    void updateField();

    virtual void connectLocallyTo(MessagePeer* buddy, bool recursive = true);
    void setServerSide(bool sside);
    void setMySpaceShip(const std::shared_ptr<SpaceShip>& myShip);

    virtual void disconnectLocallyFrom(MessagePeer* buddy, bool recursive = true);

    //void setActiveField(char dr);

private:
    const float m_astMinSize = 0.2f, m_astMaxSize = 1.0f;
    float m_asteroidSpaceCubeSidelength;
    AsteroidField *m_asteroids;

    osg::Group* m_levelData;
    bool m_serverSide;
    
    std::shared_ptr<SpaceShip> m_myShip;
    std::map<MessagePeer*, std::shared_ptr<SpaceShip>> m_remoteShips;
};