#include "Level.h"
#include "AsteroidField.h"
#include "Asteroid.h"
#include "SpaceShip.h"
#include <random>
#include <cmath>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osgUtil/Optimizer>

BEGIN_DECLNETMESSAGE(AsteroidFieldData, 5001, false)
    std::vector<osg::Vec3f> position;
    std::vector<float> radius;
END_DECLNETMESSAGE()

BEGIN_RAWTONETMESSAGE_QCONVERT(AsteroidFieldData)
    uint16_t size;
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
    uint16_t size = position.size();
    outStr << size;
    for (int i = 0; i < position.size(); ++i)
    {
        osg::Vec3f pos = position.at(i);
        float rad = radius.at(i);
        outStr << position.at(i) << radius.at(i);
    }
END_NETTORAWMESSAGE_QCONVERT()

REGISTER_NETMESSAGE(AsteroidFieldData)

class LevelUpdate : public osg::NodeCallback
{
    Level& m_level;
public:
    LevelUpdate(Level& myLevel) : m_level(myLevel)
    {

    }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        // Node is a group with asteroid geodes
        // delete geode and insert new ones
        osg::Group *realNode = static_cast<osg::Group*>(node);
        osgUtil::Optimizer levelOptimizer;
        if (realNode->getNumChildren() > 0)
            realNode->removeChildren(0, realNode->getNumChildren());
        //int i = 0;
        for (int i = 0; i < m_level.getAsteroidLength(); i++)
        {
            osg::Geode * asteroidsGeode = new osg::Geode;
            osg::ref_ptr<osg::Shape> sphere = new osg::Sphere(m_level.getAsteroid(i)->getPosition(), m_level.getAsteroid(i)->getRadius());
            osg::ref_ptr<osg::ShapeDrawable> ast = new osg::ShapeDrawable(sphere);
            ast->setUseDisplayList(true);
            //ast->setUseVertexBufferObjects(true);
            ast->setColor(osg::Vec4(0.3f, 0.7f, 0.1f, 1));
            asteroidsGeode->addDrawable(ast);
            realNode->addChild(asteroidsGeode);
        }
        //levelOptimizer.optimize(realNode, osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS | osgUtil::Optimizer::MERGE_GEODES);

        realNode->removeUpdateCallback(this);
    }
};

Level::Level(int asteroidNumber, float turbulence, float density, osg::Group* group):
m_levelData(group), m_serverSide(false)
{
    std::random_device randDevice;

//Generate Asteroid Field
    //step 1: scatter asteroids. save to AsteroidField
    AsteroidField *scatteredAsteroids = new AsteroidField();
    float astSizeDif = m_astMaxSize - m_astMinSize;
    float asteroidSpaceCubeSidelength = m_astMaxSize * (2 + 2 - density * 2);
    float asteroidFieldSpaceCubeSideLength = pow(pow(asteroidSpaceCubeSidelength, 3)*asteroidNumber, 0.333f);
    float radius;
    osg::Vec3f pos;
    for (int i = 0; i < asteroidNumber; i++) 
    {
        pos.set(
            randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max(),
            randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max(),
            randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max());
        radius = (randDevice()*astSizeDif / randDevice.max() + m_astMinSize)*0.5f;
        scatteredAsteroids->addAsteroid(pos, radius);
    }

    //step 2: move asteroids to avoid overlappings (heuristic). save to Level::asteroidField
    m_asteroids = new AsteroidField();
    for (int i = 0; i < asteroidNumber; i++) 
    {
        float favoredDistance = (asteroidSpaceCubeSidelength - m_astMaxSize) / 2 + scatteredAsteroids->getAsteroid(i)->getRadius();
        float boundingBoxRadius = favoredDistance + m_astMaxSize/2;
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
        m_asteroids->addAsteroid(scatteredAsteroids->getAsteroid(i)->getPosition() + shift, scatteredAsteroids->getAsteroid(i)->getRadius());
        
    }
    delete scatteredAsteroids;

};

Level::~Level()
{
    delete m_asteroids;
}

Asteroid*
Level::getAsteroid(int id)
{
    return m_asteroids->getAsteroid(id);
}

void Level::setServerSide(bool sside)
{
    m_serverSide = sside;
}

int
Level::getAsteroidLength()
{
    return m_asteroids->getLength();
}

bool Level::takeMessage(const NetMessage::const_pointer& msg, MessagePeer* sender)
{
    if (msg->gettype() == NetAsteroidFieldDataMessage::type)
    {
        NetAsteroidFieldDataMessage::const_pointer realMsg = msg->as<NetAsteroidFieldDataMessage>();
        delete m_asteroids;
        m_asteroids = new AsteroidField;

        for (int i = 0; i < realMsg->position.size(); ++i)
        {
            m_asteroids->addAsteroid(realMsg->position.at(i), realMsg->radius.at(i));
        }
        m_levelData->setUpdateCallback(new LevelUpdate(*this));
        return true;
    }
    else if (msg->gettype() == NetSpaceShipConstructionDataMessage::type)
    {
        NetSpaceShipConstructionDataMessage::const_pointer realMsg = msg->as<NetSpaceShipConstructionDataMessage>();
        std::shared_ptr<SpaceShip> newShip(new SpaceShip(realMsg->pos, realMsg->orient));
        m_levelData->getParent(0)->addChild(newShip->getTransformGroup());
        m_remoteShips[sender] = newShip;
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
    //levelOptimizer.optimize(m_levelData, osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS | osgUtil::Optimizer::MERGE_GEODES);
}

void Level::connectLocallyTo(MessagePeer* buddy, bool recursive /*= true*/)
{
    MessagePeer::connectLocallyTo(buddy, recursive);

    // if we are the server, send our asteroid field data to client
    if (m_serverSide)
    {
        NetAsteroidFieldDataMessage::pointer msg(new NetAsteroidFieldDataMessage);

        for (int i = 0; i < m_asteroids->getLength(); ++i)
        {
            msg->position.push_back(m_asteroids->getAsteroid(i)->getPosition());
            msg->radius.push_back(m_asteroids->getAsteroid(i)->getRadius());
        }
        buddy->send(msg);
    }

    // inform buddy about our space ship
    NetSpaceShipConstructionDataMessage::pointer shipMsg = m_myShip->createConstructorMessage();
    buddy->send(shipMsg);
}

void Level::disconnectLocallyFrom(MessagePeer* buddy, bool recursive /*= true*/)
{
    MessagePeer::disconnectLocallyFrom(buddy, recursive);

    if (m_remoteShips.count(buddy) > 0)
    {
        //SpaceShip* peerToRemove = m_remoteShips[buddy];
        //delete peerToRemove;
        m_remoteShips.erase(buddy);
    }
}

void Level::setMySpaceShip(const std::shared_ptr<SpaceShip>& myShip)
{
    m_myShip = myShip;
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