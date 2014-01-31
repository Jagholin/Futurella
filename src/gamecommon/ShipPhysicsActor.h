#pragma once
#include <btBulletDynamicsCommon.h>
#include <osg/Vec3f>

class ShipPhysicsActor : public btActionInterface
{
public:
    explicit ShipPhysicsActor(btRigidBody* slave);
    virtual ~ShipPhysicsActor();

    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep);

    virtual void debugDraw(btIDebugDraw* debugDrawer);

    void setForceVector(const osg::Vec3f&);
    void setRotationAxis(const osg::Vec3f&);
    
protected:
    btRigidBody* m_slaveBody;

    btVector3 m_forceVec, m_torqueVec;
    bool m_forceNulled, m_torqueNulled;
};
