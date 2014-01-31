#include "ShipPhysicsActor.h"
#include <iostream>
#include <cmath>
#include <cassert>

ShipPhysicsActor::ShipPhysicsActor(btRigidBody* slave)
{
    m_forceVec.setZero();
    m_torqueVec.setZero();
    m_slaveBody = slave;
}

ShipPhysicsActor::~ShipPhysicsActor()
{
    // nop
}

void ShipPhysicsActor::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep)
{
    if (m_forceVec.fuzzyZero() && m_torqueVec.fuzzyZero())
        return;

    btTransform trans;
    m_slaveBody->getMotionState()->getWorldTransform(trans);
    btQuaternion worldRot, worldRotInv, force(m_forceVec.x(), m_forceVec.y(), m_forceVec.z(), 0);
    worldRot = trans.getRotation();
    worldRotInv = worldRot.inverse();
    force = worldRot * force * worldRotInv;
    btVector3 myForce(force.x(), force.y(), force.z());
    assert(std::abs(myForce.length2() - m_forceVec.length2()) < 0.01f);

    // rotate torque vector as well
    btQuaternion torque(m_torqueVec.x(), m_torqueVec.y(), m_torqueVec.z(), 0);
    torque = worldRot * torque * worldRotInv;
    btVector3 myTorque(torque.x(), torque.y(), torque.z());
    assert(std::abs(myTorque.length2() - m_torqueVec.length2()) < 0.01f);

    if (!m_forceVec.fuzzyZero())
    {
        m_slaveBody->applyCentralForce(myForce);
    }
    if (!m_torqueVec.fuzzyZero())
    {
        m_slaveBody->applyTorque(myTorque);
    }
    m_slaveBody->activate(true);
}

void ShipPhysicsActor::debugDraw(btIDebugDraw* debugDrawer)
{
    // nop
}

void ShipPhysicsActor::setForceVector(const osg::Vec3f& v)
{
    m_forceVec.setValue(v.x(), v.y(), v.z());
}

void ShipPhysicsActor::setRotationAxis(const osg::Vec3f& v)
{
    m_torqueVec.setValue(v.x(), v.y(), v.z());
}
