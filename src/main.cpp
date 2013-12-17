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
	std::srand(std::chrono::system_clock::now().time_since_epoch().count());
    // setup CEGUI as OSG drawable
    osgViewer::Viewer viewer;
	GUIApplication guiApp(&viewer);

    osg::ref_ptr<osg::Group> root = new osg::Group;
	
	osg::ref_ptr<osg::Group> asteroids = new osg::Group;
	Level level(50, 0.5f, 1, asteroids.get());
    guiApp.setCurrentLevel(&level);

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

    viewer.setSceneData(root.get());
	viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

	viewer.realize();
    level.updateField();

	osgViewer::ViewerBase::Windows windowList;
	viewer.getWindows(windowList);
	windowList[0]->useCursor(false);
    return viewer.run();
}