#pragma once

#include <osg/Vec3f>
#include <osg/Quat>
#include <functional>
#include <map>
#include <btBulletDynamicsCommon.h>

class ShipPhysicsActor;

class PhysicsEngine
{
public:

    PhysicsEngine();
    virtual ~PhysicsEngine();

    typedef std::function<void(const osg::Vec3f&, const osg::Quat&)> t_motionFunc;

    void physicsInit();

    void addCollisionSphere(osg::Vec3f pos, float radius, float mass = 0.f);
    unsigned int addUserVehicle(const osg::Vec3f& pos, const osg::Vec3f& sizes, const osg::Quat& orient, float mass);
    void removeVehicle(unsigned int id);
    btRigidBody* getBodyById(unsigned int);
    ShipPhysicsActor* getActorById(unsigned int);
    void addMotionCallback(unsigned int, const t_motionFunc& cb);

    void physicsTick(float msDelta);

    enum {
        COLLISION_SHIPGROUP = 0x01,
        COLLISION_ASTEROIDGROUP = 0x02
    };

protected:
    friend class VehicleMotionState;

    btDynamicsWorld* m_physicsWorld;

    btDefaultCollisionConfiguration* m_collisionConfig;
    btCollisionDispatcher* m_collisionDispatcher;
    btBroadphaseInterface* m_broadphase;
    btSequentialImpulseConstraintSolver* m_constraintSolver;

    btAlignedObjectArray<btCollisionShape*> m_collisionShapes;
    btCollisionShape* m_vehicleShape;
    //btAlignedObjectArray<VehicleMotionState*> m_vehicleMotionObservers;
    std::map<unsigned int, t_motionFunc> m_motionCallbacks;
    std::map<unsigned int, btRigidBody*> m_vehicles;
    std::map<unsigned int, ShipPhysicsActor*> m_actors;
    unsigned int    m_nextUsedId;
};
