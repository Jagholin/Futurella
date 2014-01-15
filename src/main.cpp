#include <osgDB/ReadFile>
#include <osg/Group>
#include <osg/ShapeDrawable>
#include <osgViewer/Viewer>
#include <CEGUI/CEGUI.h>
#include <osg/MatrixTransform>
#include <chrono>
#include <map>
#include <osgGA/TrackballManipulator>
#include <iostream>

#include "CEGUIDrawable.h"
#include "GUIApplication.h"

#include "Level.h"
#include "Asteroid.h"
#include "SpaceShip.h"
#include "ChaseCam.h"

int main()
{
	std::srand(std::chrono::system_clock::now().time_since_epoch().count());
    // setup CEGUI as OSG drawable
    osgViewer::Viewer viewer;
	GUIApplication guiApp(&viewer);

	osg::ref_ptr<osg::Group> root = new osg::Group;

	osg::ref_ptr<osg::Group> asteroids = new osg::Group;
    std::shared_ptr<SpaceShip> ship(new SpaceShip);
    ChaseCam *chaseCam = new ChaseCam(ship.get());
	Level level(6, 0.5f, 1, asteroids.get());
    guiApp.setCurrentLevel(&level);
    level.setMySpaceShip(ship);

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
	postCamera->addChild(ceguiNode);

    root->addChild(postCamera);
    root->addChild(ship->getTransformGroup());

    viewer.setSceneData(root.get());
	viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

	viewer.realize();
    level.updateField();

	osgViewer::ViewerBase::Windows windowList;
	viewer.getWindows(windowList);
	windowList[0]->useCursor(false);
	viewer.setCameraManipulator(chaseCam);
//	viewer.setCameraManipulator(new osgGA::TrackballManipulator);
	viewer.realize();
	
	std::chrono::duration<float> frameTime(0);
	std::chrono::steady_clock::time_point start;

	while (!viewer.done()){
		start = std::chrono::steady_clock::now();

		ship->update(frameTime.count());
		viewer.frame();

		frameTime = std::chrono::steady_clock::now() - start;
	}
}