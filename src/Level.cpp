#include "Level.h"
#include "AsteroidField.h"
#include "Asteroid.h"
#include <random>
#include <cmath>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osgUtil/Optimizer>

BEGIN_DECLNETMESSAGE(AsteroidFieldData, 5001)
    std::vector<osg::Vec3f> position;
    std::vector<float> radius;
END_DECLNETMESSAGE()

BEGIN_RAWTONETMESSAGE_QCONVERT(AsteroidFieldData)
    unsigned int size;
    inStr >> size;
    for (unsigned int i = 0; i < size; ++i)
    {
        osg::Vec3f pos;
        float rad;
        inStr >> pos >> rad;
        temp->position.push_back(pos);
        temp->radius.push_back(rad);
    }
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(AsteroidFieldData)
    outStr << position.size();
    for (int i = 0; i < position.size(); ++i)
    {
        outStr << position.at(i) << radius.at(i);
    }
END_NETTORAWMESSAGE_QCONVERT()

REGISTER_NETMESSAGE(AsteroidFieldData)

Level::Level(int asteroidNumber, float turbulence, float density, osg::Group* group):
m_levelData(group), m_serverSide(false)
{
	std::random_device randDevice;

//Generate Asteroid Field
	//step 1: scatter asteroids. save to AsteroidField
	AsteroidField *scatteredAsteroids = new AsteroidField();
	float astSizeDif = astMaxSize - astMinSize;
	float asteroidSpaceCubeSidelength = astMaxSize * (2 + 9 - density * 9);
	float asteroidFieldSpaceCubeSideLength = pow(pow(asteroidSpaceCubeSidelength, 3)*asteroidNumber, 0.333f);
	float radius;
	osg::Vec3f pos;
	for (int i = 0; i < asteroidNumber; i++) 
	{
		pos.set(
			randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max(),
			randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max(),
			randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max());
		radius = (randDevice()*astSizeDif / randDevice.max() + astMinSize)*0.5f;
		scatteredAsteroids->addAsteroid(pos, radius);
	}

	//step 2: move asteroids to avoid overlappings (heuristic). save to Level::asteroidField
	asteroids = new AsteroidField();
	for (int i = 0; i < asteroidNumber; i++) 
	{
		float favoredDistance = (asteroidSpaceCubeSidelength - astMaxSize) / 2 + scatteredAsteroids->getAsteroid(i)->getRadius();
		float boundingBoxRadius = favoredDistance + astMaxSize/2;
		std::list<Asteroid*> candidates = scatteredAsteroids->getAsteroidsInBlock(scatteredAsteroids->getAsteroid(i)->getPosition(), osg::Vec3f(boundingBoxRadius, boundingBoxRadius, boundingBoxRadius));
		osg::Vec3f shift(0,0,0);
		int numberOfCloseAsteroids = 0;
		for (std::list<Asteroid*>::iterator it = candidates.begin(); it != candidates.end(); it++) {
			if (*it != scatteredAsteroids->getAsteroid(i)) {
				osg::Vec3f d = scatteredAsteroids->getAsteroid(i)->getPosition() - (*it)->getPosition();
				float scale = -((d.length() - (*it)->getRadius()) - favoredDistance);
				if (scale > 0) {
					d.normalize();
					shift += shift + (d * scale *0.5f); //push away from other close asteroids
					numberOfCloseAsteroids++;
				}
			}
		}
		asteroids->addAsteroid(scatteredAsteroids->getAsteroid(i)->getPosition() + shift, scatteredAsteroids->getAsteroid(i)->getRadius());
		
	}

	//movedAsteroids = asteroids;

	delete scatteredAsteroids;

};

Level::~Level()
{
	delete asteroids;
//	delete scatteredAsteroids;
}

Asteroid*
Level::getAsteroid(int id)
{
	return asteroids->getAsteroid(id);
}

void Level::setServerSide(bool sside)
{
    m_serverSide = sside;
}

int
Level::getAsteroidLength()
{
	return asteroids->getLength();
}

bool Level::takeMessage(const NetMessage::const_pointer& msg, MessagePeer*)
{
    if (msg->gettype() == NetAsteroidFieldDataMessage::type)
    {
        NetAsteroidFieldDataMessage::const_pointer realMsg = msg->as<NetAsteroidFieldDataMessage>();
        delete asteroids;
        asteroids = new AsteroidField;

        for (int i = 0; i < realMsg->position.size(); ++i)
        {
            asteroids->addAsteroid(realMsg->position.at(i), realMsg->radius.at(i));
        }
        updateField();
        return true;
    }
    return false;
}

void Level::updateField()
{
    osgUtil::Optimizer levelOptimizer;
    if (m_levelData->getNumChildren() > 0)
        m_levelData->removeChildren(0, m_levelData->getNumChildren());
    //int i = 0;
    for (int i = 0; i < getAsteroidLength(); i++)
    {
        osg::Geode * asteroidsGeode = new osg::Geode;
        osg::ref_ptr<osg::Shape> sphere = new osg::Sphere(getAsteroid(i)->getPosition(), getAsteroid(i)->getRadius());
        osg::ref_ptr<osg::ShapeDrawable> ast = new osg::ShapeDrawable(sphere);
        ast->setUseDisplayList(true);
        //ast->setUseVertexBufferObjects(true);
        ast->setColor(osg::Vec4(0.3f, 0.7f, 0.1f, 1));
        asteroidsGeode->addDrawable(ast);
        m_levelData->addChild(asteroidsGeode);
    }
    levelOptimizer.optimize(m_levelData, osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS | osgUtil::Optimizer::MERGE_GEODES);
}

void Level::connectLocallyTo(MessagePeer* buddy, bool recursive /*= true*/)
{
    MessagePeer::connectLocallyTo(buddy, recursive);

    // if we are the server, send our asteroid field data to client
    if (m_serverSide)
    {
        NetAsteroidFieldDataMessage::pointer msg(new NetAsteroidFieldDataMessage);

        for (int i = 0; i < asteroids->getLength(); ++i)
        {
            msg->position.push_back(asteroids->getAsteroid(i)->getPosition());
            msg->radius.push_back(asteroids->getAsteroid(i)->getRadius());
        }
        buddy->send(msg);
    }
}

/*
void
Level::setActiveField(char dr)
{
	if (dr == 'd')
		asteroids = scatteredAsteroids;
	else
		asteroids = movedAsteroids;
}*/