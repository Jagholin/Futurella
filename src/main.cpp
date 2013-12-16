#include <osgDB/ReadFile>
#include <osg/Group>
#include <osg/ShapeDrawable>
#include <osgViewer/Viewer>
#include <CEGUI/CEGUI.h>
#include <chrono>
#include <map>

#include "CEGUIDrawable.h"
#include "GUIApplication.h"

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
	std::srand(std::chrono::system_clock::now().time_since_epoch().count());
    // setup CEGUI as OSG drawable
    osgViewer::Viewer viewer;
	GUIApplication guiApp(&viewer);

	Level level(500, 0.5f, 1);

	const std::map<unsigned int, unsigned int> testMap;
	unsigned int myNull = 0;
	for (std::map<unsigned int, unsigned int>::const_iterator it = testMap.begin(); it != testMap.end(); ++it)
	{
		myNull = testMap.count(12);
	}

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

    osg::ref_ptr <osg::Node> cessnaNode = osgDB::readNodeFile("cessna.osg");
	osg::ref_ptr<osg::Geode> cube = new osg::Geode;
	osg::Drawable * myD = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0, 0, 0), 1, 1, 1));
	myD->setUseDisplayList(false);
	myD->setUseVertexBufferObjects(true);
	cube->addDrawable(myD);
    osg::ref_ptr<osg::Geode> ceguiNode = new osg::Geode;
	osg::ref_ptr<CeguiDrawable> guiSurface = new CeguiDrawable;
	guiSurface->setGuiApplication(&guiApp);
    ceguiNode->addDrawable(guiSurface);
	ceguiNode->setCullingActive(false);

	osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
	postCamera->setCullingActive(false);
	postCamera->setRenderOrder(osg::Camera::POST_RENDER);
	postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
	postCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);

	root->addChild(asteroids);
	
	osgViewer::Viewer viewer;
	viewer.setSceneData(root.get());
	postCamera->addChild(ceguiNode);

    root->addChild(cessnaNode);
	//root->addChild(cube);
    root->addChild(postCamera);

    viewer.setSceneData(root.get());
	//viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

	viewer.realize();

	osgViewer::ViewerBase::Windows windowList;
	viewer.getWindows(windowList);
	windowList[0]->useCursor(false);
    return viewer.run();
}