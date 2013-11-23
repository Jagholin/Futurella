#include <osgDB\ReadFile>
#include <osg\Group>
#include <osg\ShapeDrawable>
#include <osgViewer\Viewer>
#include <chrono>

#include <iostream>

#include "Level.h"

int main()
{
	Level level(1000, 0.5f, 0.5f);

    osg::ref_ptr<osg::Group> root = new osg::Group;
	
	osg::ref_ptr<osg::Geode> asteroids = new osg::Geode;
	for (int i = 0; i < level.getAsteroidLength(); i++)
	{
		osg::ref_ptr<osg::Shape> sphere = new osg::Sphere(level.getAsteroid(i).position, level.getAsteroid(i).radius);
		osg::ref_ptr<osg::ShapeDrawable> ast = new osg::ShapeDrawable(sphere);
		ast->setColor(osg::Vec4(0.7f, 0.4f, 0.1f, 1));
		asteroids->addDrawable(ast);
	}

	root->addChild(asteroids);
	
	osgViewer::Viewer viewer;
	viewer.setSceneData(root.get());
    return viewer.run();
}