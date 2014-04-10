#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Quaternion.h>
#include <functional>
#include <map>
#include <btBulletDynamicsCommon.h>
#include <thread>

using namespace Magnum;
class ShipPhysicsActor;

class PhysicsEngine
{
public:

    PhysicsEngine();
    virtual ~PhysicsEngine();

    typedef std::function<void(const Vector3&, const Quaternion&)> t_motionFunc;

    void physicsInit();

    unsigned int addCollisionSphere(Vector3 pos, float radius, float mass = 0.f);
    void removeCollisionSphere(unsigned int id);
    unsigned int addUserVehicle(const Vector3& pos, const Vector3& sizes, const Quaternion& orient, float mass);
    void removeVehicle(unsigned int id);
    btRigidBody* getBodyById(unsigned int);
    ShipPhysicsActor* getActorById(unsigned int);
    void addMotionCallback(unsigned int, const t_motionFunc& cb);

    void setShipTransformation(unsigned int shipId, btTransform transformation);
    Vector3 getShipPosition(unsigned int shipId);
    

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
    std::map<unsigned int, btCollisionObject*> m_collisionObjects;
    //btAlignedObjectArray<VehicleMotionState*> m_vehicleMotionObservers;
    std::map<unsigned int, t_motionFunc> m_motionCallbacks;
    std::map<unsigned int, btRigidBody*> m_vehicles;
    std::map<unsigned int, ShipPhysicsActor*> m_actors;
    unsigned int    m_nextUsedId;

    const std::thread m_physicsThread;
};
