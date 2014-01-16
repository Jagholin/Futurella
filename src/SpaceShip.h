#pragma once
#include <osg/Group>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include "networking/messages.h"

BEGIN_DECLNETMESSAGE(SpaceShipConstructionData, 5002, false)
osg::Vec3f pos;
osg::Vec4f orient;
END_DECLNETMESSAGE()

class SpaceShip : public LocalMessagePeer
{
public:
    SpaceShip(osg::Vec3f pos, osg::Vec4f orient, bool hostSide = false);
    explicit SpaceShip(osg::Node* shipNode = nullptr);
    virtual ~SpaceShip();

    void setPosition(osg::Vec3f position);
    void setOrientation(osg::Quat rotation);

    void setAccelerate(bool state);
    void setLeft(bool state);
    void setRight(bool state);
    void setUp(bool state);
    void setDown(bool state);
    void setBack(bool state);
    void setHostSide(bool hSide);

    void update(float deltaTime);

    osg::Vec3f getCenter();
    float getSpeed();
    osg::Vec3f getVelocity();
    osg::Quat getOrientation();
    osg::MatrixTransform* getTransformGroup();

    // virtual func from LocalMessagePeer
    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);

    NetSpaceShipConstructionDataMessage::pointer createConstructorMessage() const;
private:
    osg::Vec3f m_position, m_velocity;
    osg::Quat m_orientation;

    bool m_stateBack, m_stateAcc, m_stateLeft, m_stateRight, m_stateUp, m_stateDown;

    float m_acceleration, m_steerability;
    osg::ref_ptr<osg::Node> m_shipNode;
    osg::ref_ptr<osg::MatrixTransform> m_transformGroup;
    bool m_hostSide;
};