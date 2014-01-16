#include "SpaceShip.h"
#include <osg/ShapeDrawable>

BEGIN_DECLNETMESSAGE(ShipStateData, 5101, false)
osg::Vec3f position;
osg::Vec4f orientation;
osg::Vec3f velocity;
END_DECLNETMESSAGE()

BEGIN_NETTORAWMESSAGE_QCONVERT(ShipStateData)
outStr << position << orientation << velocity;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(ShipStateData)
inStr >> temp->position >> temp->orientation >> temp->velocity;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(SpaceShipConstructionData)
outStr << pos << orient;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(SpaceShipConstructionData)
inStr >> temp->pos >> temp->orient;
END_RAWTONETMESSAGE_QCONVERT()

REGISTER_NETMESSAGE(ShipStateData);
REGISTER_NETMESSAGE(SpaceShipConstructionData);

SpaceShip::SpaceShip(osg::Vec3f pos, osg::Vec4f orient, bool hostSide /* = false */):
m_position(pos), m_orientation(orient), m_stateBack(false), m_stateAcc(false), m_stateLeft(false), m_stateRight(false), m_stateUp(false), m_stateDown(false), m_hostSide(hostSide)
{
    osg::ref_ptr<osg::Shape> box = new osg::Box(osg::Vec3f(0, 0, 0), 0.1f, 0.1f, 0.3f);
    osg::ref_ptr<osg::ShapeDrawable> s = new osg::ShapeDrawable(box);
    s->setColor(osg::Vec4(1, 1, 1, 1));
    osg::ref_ptr<osg::Geode> d = new osg::Geode();
    d->addDrawable(s);

    m_shipNode = d;
    m_transformGroup = new osg::MatrixTransform;
    m_transformGroup->setMatrix(osg::Matrix::translate(0, 0, 0));
    m_transformGroup->addChild(m_shipNode);
}

SpaceShip::SpaceShip(osg::Node* shipNode):
m_shipNode(shipNode), m_hostSide(true)
{
    if (!m_shipNode)
    {
        osg::ref_ptr<osg::Shape> box = new osg::Box(osg::Vec3f(0, 0, 0), 0.1f, 0.1f, 0.3f);
        osg::ref_ptr<osg::ShapeDrawable> s = new osg::ShapeDrawable(box);
        s->setColor(osg::Vec4(1, 1, 1, 1));
        osg::ref_ptr<osg::Geode> d = new osg::Geode();
        d->addDrawable(s);

        m_shipNode = d;
    }
    m_transformGroup = new osg::MatrixTransform;
    m_transformGroup->setMatrix(osg::Matrix::translate(0, 0, 0));
    m_transformGroup->addChild(m_shipNode);

    m_velocity = osg::Vec3f(0, 0, 0);
    setPosition(osg::Vec3f(0, 0, 0));
    setOrientation(osg::Quat(0, osg::Vec3f(1, 0, 0)));
    m_stateBack = m_stateAcc = m_stateLeft = m_stateRight = m_stateDown = m_stateUp = false;
    m_acceleration = 2;
    m_steerability = 1;
}

SpaceShip::~SpaceShip()
{
    osg::Node::ParentList parents = m_transformGroup->getParents();

    for (osg::Group* parent : parents)
        parent->removeChild(m_transformGroup);
}

void SpaceShip::setPosition(osg::Vec3f p)
{
    m_position = p;
}

void SpaceShip::setOrientation(osg::Quat o)
{
    m_orientation = o;
}

void SpaceShip::setAccelerate(bool state)
{
    m_stateAcc = state;
}

void SpaceShip::setLeft(bool state)
{
    m_stateLeft = state;
}

void SpaceShip::setRight(bool state)
{
    m_stateRight = state;
}

void SpaceShip::setUp(bool state)
{
    m_stateUp = state;
}

void SpaceShip::setDown(bool state)
{
    m_stateDown = state;
}

void SpaceShip::setBack(bool state)
{
    m_stateBack = state;
}

osg::Vec3f SpaceShip::getCenter()
{
    return m_position;
}

float SpaceShip::getSpeed()
{
    return m_velocity.length();
}

osg::Vec3f SpaceShip::getVelocity()
{
    return m_velocity;
}

osg::Quat SpaceShip::getOrientation()
{
    return m_orientation;
}

osg::MatrixTransform* SpaceShip::getTransformGroup()
{
    return m_transformGroup.get();
}

void SpaceShip::update(float dt)
{
    m_position += m_velocity*dt;

    if (m_stateAcc != m_stateBack)
    {
        if (m_stateAcc)
            m_velocity += m_orientation*osg::Vec3f(0, 0, -m_acceleration * dt);
        else
            m_velocity += m_orientation*osg::Vec3f(0, 0, m_acceleration * dt);
    }

    if (m_stateLeft != m_stateRight){
        float amount = m_steerability * dt;
        if (m_stateLeft){
            //roll left
            osg::Quat q(amount, osg::Vec3f(0, 0, 1));
            m_orientation = q * m_orientation;
        }
        else{
            //roll right
            osg::Quat q(-amount, osg::Vec3f(0, 0, 1));
            m_orientation = q * m_orientation;
        }
    }
    if (m_stateUp != m_stateDown){
        float amount = m_steerability * dt;
        if (m_stateUp){
            //rear up
            osg::Quat q(-amount, osg::Vec3f(1, 0, 0));
            m_orientation = q * m_orientation;
        }
        else{
            //rear down
            osg::Quat q(amount, osg::Vec3f(1, 0, 0));
            m_orientation = q * m_orientation;
        }
    }

    m_transformGroup->setMatrix(osg::Matrix::translate(getCenter()));
    osg::Matrix rotationMatrix;
    getOrientation().get(rotationMatrix);
    m_transformGroup->preMult(rotationMatrix);
}

bool SpaceShip::takeMessage(const NetMessage::const_pointer& msg, MessagePeer*)
{
    if (!m_hostSide && msg->gettype() == NetShipStateDataMessage::type)
    {
        NetShipStateDataMessage::const_pointer realMsg = msg->as<NetShipStateDataMessage>();

        // set position, rotation and velocity.
        m_position = realMsg->position;
        m_velocity = realMsg->velocity;
        m_orientation.set(realMsg->orientation);

        update(0);
        return true;
    }
    return false;
}

void SpaceShip::setHostSide(bool hSide)
{
    m_hostSide = hSide;
}

NetSpaceShipConstructionDataMessage::pointer SpaceShip::createConstructorMessage() const
{
    if (!m_hostSide)
        return nullptr;

    NetSpaceShipConstructionDataMessage::pointer result(new NetSpaceShipConstructionDataMessage);
    result->orient = m_orientation.asVec4();
    result->pos = m_position;
    return result;
}

