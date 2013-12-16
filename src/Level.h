#ifndef hLevel_included
#define hLevel_included

#include <list>
#include <vector>
#include <osg/Group>
#include "networking/messages.h"

class AsteroidField;

class Asteroid;

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

    //void setActiveField(char dr);

private:
	const float astMinSize = 0.2f, astMaxSize = 1.0f;
	float asteroidSpaceCubeSidelength;
	AsteroidField *asteroids;

    osg::Group* m_levelData;
    bool m_serverSide;
	//AsteroidField *scatteredAsteroids; //debug!
	//AsteroidField *movedAsteroids; //debug!
	
};

#endif//hLevel_included ndef
