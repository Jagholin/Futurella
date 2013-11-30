#include <osgDB\ReadFile>
#include <osg\Group>
#include <osg\ShapeDrawable>
#include <osgViewer\Viewer>
#include <chrono>
#include <iostream>

#include "Level.h"
#include "Asteroid.h"

int main()
{
	/* performancetest
	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 1; i < 20000; i += 200){
		Level l(i, 0.5, 0.98f);
		std::cout << "Dauer (" << i << " asteroiden):" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << "\n";
		start = std::chrono::high_resolution_clock::now();
	}*/ 

	Level level(500, 0.5f, 1);

    osg::ref_ptr<osg::Group> root = new osg::Group;
	
	osg::ref_ptr<osg::Geode> asteroids = new osg::Geode;
	
	//level.setActiveField('r');
	for (int i = 0; i < level.getAsteroidLength(); i++)
	{
		osg::ref_ptr<osg::Shape> sphere = new osg::Sphere(level.getAsteroid(i)->getPosition(), level.getAsteroid(i)->getRadius());
		osg::ref_ptr<osg::ShapeDrawable> ast = new osg::ShapeDrawable(sphere);
		ast->setColor(osg::Vec4(0.3f, 0.7f, 0.1f, 1));
		asteroids->addDrawable(ast);
	}

	/*
	level.setActiveField('d');
	for (int i = 0; i < level.getAsteroidLength(); i++)
	{
		osg::ref_ptr<osg::Shape> sphere = new osg::Sphere(level.getAsteroid(i)->getPosition(), level.getAsteroid(i)->getRadius());
		osg::ref_ptr<osg::ShapeDrawable> ast = new osg::ShapeDrawable(sphere);
		ast->setColor(osg::Vec4(0.4f, 0.6f, 0.4f, 1));
		asteroids->addDrawable(ast);
	}*/

	root->addChild(asteroids);
	
	osgViewer::Viewer viewer;
	viewer.setSceneData(root.get());
    return viewer.run();
}