#pragma once
#include <osg/Group>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include "networking/messages.h"

BEGIN_DECLNETMESSAGE(SpaceShipConstructionData, 5002)
osg::Vec3f pos;
osg::Vec4f orient;
END_DECLNETMESSAGE()

class SpaceShip : public LocalMessagePeer
{
public:
	static const int inputTypesNumber = 6;
	enum inputType
	{
		ACCELERATE,
		LEFT,
		RIGHT,
		UP,
		DOWN,
		BACK
	};

    SpaceShip(osg::Vec3f pos, osg::Vec4f orient, bool hostSide = false);
	explicit SpaceShip(osg::Node* shipNode = nullptr);
    virtual ~SpaceShip();

	void setPosition(osg::Vec3f position);
	void setOrientation(osg::Quat rotation);

	void setInput(inputType type, bool state);

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
	osg::Vec3f position, velocity;
	osg::Quat orientation;

	bool inputState[inputTypesNumber];

	float acceleration, steerability, friction; //Ship Steering Behaviour
    
	osg::ref_ptr<osg::Node> m_shipNode;
    osg::ref_ptr<osg::MatrixTransform> m_transformGroup;
    bool m_hostSide;
};