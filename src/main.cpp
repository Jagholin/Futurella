#include <osgDB\ReadFile>
#include <osg\Group>
#include <osg\MatrixTransform>
#include <osg\ShapeDrawable>
#include <osg\MatrixTransform>
#include <osgViewer\Viewer>
#include <chrono>
#include <osgGA\TrackballManipulator>
#include <iostream>

#include "Level.h"
#include "Asteroid.h"
#include "SpaceShip.h"
#include "ChaseCam.h"

int main()
{
	Level level(100, 0.5f, 0.5f);
	SpaceShip ship;
	ChaseCam chaseCam(&ship);

	osg::ref_ptr<osg::Group> root = new osg::Group;

	osg::ref_ptr<osg::Geode> asteroids = new osg::Geode;

	for (int i = 0; i < level.getAsteroidLength(); i++)
	{
		osg::ref_ptr<osg::Shape> sphere = new osg::Sphere(level.getAsteroid(i)->getPosition(), level.getAsteroid(i)->getRadius());
		osg::ref_ptr<osg::ShapeDrawable> ast = new osg::ShapeDrawable(sphere);
		ast->setColor(osg::Vec4(0.3f, 0.7f, 0.1f, 1));
		asteroids->addDrawable(ast);
	}

	osg::ref_ptr<osg::Shape> box = new osg::Box(osg::Vec3f(0,0,0), 0.1, 0.1f, 0.3f);
	osg::ref_ptr<osg::ShapeDrawable> s = new osg::ShapeDrawable(box);
	s->setColor(osg::Vec4(1, 1, 1, 1));
	osg::ref_ptr<osg::Geode> d = new osg::Geode();
	osg::ref_ptr<osg::MatrixTransform> spaceShipTransform = new osg::MatrixTransform;
	spaceShipTransform->setMatrix(osg::Matrix::translate(0, 0, 0));
	spaceShipTransform->addChild(d);
	d->addDrawable(s);

	root->addChild(asteroids);
	root->addChild(spaceShipTransform);

	osgViewer::Viewer viewer;
	viewer.setSceneData(root.get());
	viewer.setCameraManipulator(&chaseCam);
//	viewer.setCameraManipulator(new osgGA::TrackballManipulator);
	viewer.realize();
	
	std::chrono::duration<float> frameTime(0);
	std::chrono::steady_clock::time_point start;

	while (!viewer.done()){
		start = std::chrono::steady_clock::now();

		ship.update(frameTime.count());
		spaceShipTransform->setMatrix(osg::Matrix::translate(ship.getCenter()));
		osg::Matrix rotationMatrix;
		ship.getOrientation().get(rotationMatrix);
		spaceShipTransform->preMult(rotationMatrix);
		viewer.frame();

		frameTime = std::chrono::steady_clock::now() - start;
	}
}