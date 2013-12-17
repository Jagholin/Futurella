#ifndef hChaseCam_included
#define hChaseCam_included

#include <osgGA/CameraManipulator>
#include <osg/Quat>

#include "SpaceShip.h"

/**
ChaseCam is a camera manipulator which provides functionality to steer the space ship.
The Camera chases behind the ship with a small amount of lazyness
*/

class ChaseCam : public osgGA::CameraManipulator
{
public:

	ChaseCam(SpaceShip* spaceShip);

	virtual const char* className() const { return "ChaseCam"; }

	/** Get the position of the matrix manipulator using a 4x4 Matrix.*/
	virtual void setByMatrix(const osg::Matrixd& matrix);

	/** Set the position of the matrix manipulator using a 4x4 Matrix.*/
	virtual void setByInverseMatrix(const osg::Matrixd& matrix) { setByMatrix(osg::Matrixd::inverse(matrix)); }

	/** Get the position of the manipulator as 4x4 Matrix.*/
	virtual osg::Matrixd getMatrix() const;

	/** Get the position of the manipulator as a inverse matrix of the manipulator, typically used as a model view matrix.*/
	virtual osg::Matrixd getInverseMatrix() const;

	virtual void computeHomePosition();

	virtual void home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);

	virtual void init(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);

	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);

	virtual void updateCamera(osg::Camera &camera);

	/** Get the keyboard and mouse usage of this manipulator.*/
	virtual void getUsage(osg::ApplicationUsage& usage) const;

	virtual ~ChaseCam();

protected:
	SpaceShip* ship;

	bool intersect(const osg::Vec3d& start, const osg::Vec3d& end, osg::Vec3d& intersection, osg::Vec3d& normal) const;

	/** Reset the internal GUIEvent stack.*/
	void flushMouseEventStack();

	/** Add the current mouse GUIEvent to internal stack.*/
	void addMouseEvent(const osgGA::GUIEventAdapter& ea);

	void computeMatrix();

	osg::Matrixd cameraMatrix;

	osg::Quat delayRotation;
};

#endif//hChaseCam_included ndef