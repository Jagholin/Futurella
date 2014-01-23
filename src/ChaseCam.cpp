#include <stdlib.h>

#include "ChaseCam.h"

#include <osgUtil/LineSegmentIntersector>
#include <osg/Notify>

using namespace osg;
using namespace osgGA;

ChaseCam::ChaseCam(SpaceShip* spaceShip) 
{
    ship = spaceShip;
    delayRotation.makeRotate(0,osg::Vec3f(1,0,0));
}

ChaseCam::~ChaseCam() 
{
    //nop
}

void ChaseCam::computeHomePosition() 
{
    //cam should be point berechnen 
    //setzen
}

void ChaseCam::home(const GUIEventAdapter& ea, GUIActionAdapter& us) 
{
    computeHomePosition();
}

void ChaseCam::init(const GUIEventAdapter& ea, GUIActionAdapter& us) 
{
    //vielleicht verwenden: us.requestContinuousUpdate(true);

}

bool ChaseCam::handle(const GUIEventAdapter& ea, GUIActionAdapter& us) 
{
    if (ea.getHandled()) return false;
    
    osgGA::GUIEventAdapter::EventType et = ea.getEventType();
    if (et == GUIEventAdapter::KEYDOWN || et == GUIEventAdapter::KEYUP)
    {
        bool onoff = (et == GUIEventAdapter::KEYDOWN);
        if (ea.getKey() == GUIEventAdapter::KEY_Space) {
            ship->setInput(SpaceShip::ACCELERATE, onoff);
            return true;
        }
        else if (ea.getKey() == GUIEventAdapter::KEY_Left) {
            ship->setInput(SpaceShip::LEFT, onoff);
            return true;
        }
        else if (ea.getKey() == GUIEventAdapter::KEY_Right) {
            ship->setInput(SpaceShip::RIGHT, onoff);
            return true;
        }
        else if (ea.getKey() == GUIEventAdapter::KEY_Up) {
            ship->setInput(SpaceShip::UP, onoff);
            return true;
        }
        else if (ea.getKey() == GUIEventAdapter::KEY_Down) {
            ship->setInput(SpaceShip::DOWN, onoff);
            return true;
        }
        else if (ea.getKey() == GUIEventAdapter::KEY_S) {
            ship->setInput(SpaceShip::BACK, onoff);
            return true;
        }
    }
    return false;
}

void ChaseCam::updateCamera(osg::Camera& camera)
{
    computeMatrix();
    CameraManipulator::updateCamera(camera);
}

void ChaseCam::getUsage(osg::ApplicationUsage& usage) const
{
    usage.addKeyboardMouseBinding("Drive: Space", "Accelerate");
    usage.addKeyboardMouseBinding("Drive: Left", "Roll left");
    usage.addKeyboardMouseBinding("Drive: Right", "Roll right");
    usage.addKeyboardMouseBinding("Drive: Down", "Hochziehen");
    usage.addKeyboardMouseBinding("Drive: Up", "Runterziehen");
}

void ChaseCam::setByMatrix(const osg::Matrixd& matrix)
{
}

osg::Matrixd ChaseCam::getMatrix() const
{
    return cameraMatrix;
}

osg::Matrixd ChaseCam::getInverseMatrix() const
{
    return osg::Matrixd::inverse(cameraMatrix);
}

void ChaseCam::computeMatrix()
{
    osg::Matrix newCameraMatrix;
    newCameraMatrix.makeTranslate(osg::Vec3d(0, 0, 5));
    osg::Matrix shipTransform;
    delayRotation.slerp(0.05, delayRotation, ship->getOrientation());
    delayRotation.get(shipTransform);
    newCameraMatrix.postMult(shipTransform);
    shipTransform.makeTranslate(ship->getCenter());
    newCameraMatrix.postMult(shipTransform);

    //zoom

    cameraMatrix = newCameraMatrix;
}

