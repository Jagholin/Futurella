#pragma once
//#include <osgGA/CameraManipulator>
//#include <osg/Quat>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/SceneGraph/Animable.h>
#include <Magnum/SceneGraph/Object.h>

#include <Magnum/Platform/Platform.h>
#include <Magnum/Platform/Sdl2Application.h>
#include "magnumdefs.h"

//#include "SpaceShip.h"
#include "gameclient/SpaceShipClient.h"

/**
ChaseCam is a camera manipulator which provides functionality to steer the space ship.
The Camera chases behind the ship with a small amount of lazyness
*/

class ChaseCam : public Object3D, public SceneGraph::Animable3D
{
public:

    ChaseCam(SpaceShipClient* spaceShip);

    //virtual const char* className() const { return "ChaseCam"; }

    /** Get the position of the matrix manipulator using a 4x4 Matrix.*/
    //virtual void setByMatrix(const osg::Matrixd& matrix);

    /** Set the position of the matrix manipulator using a 4x4 Matrix.*/
    //virtual void setByInverseMatrix(const osg::Matrixd& matrix) { setByMatrix(osg::Matrixd::inverse(matrix)); }

    /** Get the position of the manipulator as 4x4 Matrix.*/
    //virtual osg::Matrixd getMatrix() const;

    /** Get the position of the manipulator as a inverse matrix of the manipulator, typically used as a model view matrix.*/
    //virtual osg::Matrixd getInverseMatrix() const;

    virtual void computeHomePosition();

    virtual void home();

    virtual void init();

    virtual bool handle();

    //virtual void updateCamera(osg::Camera &camera);

    /** Get the keyboard and mouse usage of this manipulator.*/
    //virtual void getUsage(osg::ApplicationUsage& usage) const;

    virtual ~ChaseCam();

protected:
    SpaceShipClient* ship;

    bool intersect(const Vector3d& start, const Vector3d& end, Vector3d& intersection, Vector3d& normal) const;

    /** Reset the internal GUIEvent stack.*/
    void flushMouseEventStack();

    /** Add the current mouse GUIEvent to internal stack.*/
    void addMouseEvent(const Platform::Sdl2Application::MouseEvent& ea);

    void computeMatrix();

    Matrix4 cameraMatrix;

    //osg::Quat delayRotation;
};
