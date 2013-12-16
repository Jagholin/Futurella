#include <osgDB/ReadFile>
#include <osg/Group>
#include <osg/ShapeDrawable>
#include <osgViewer/Viewer>
#include <CEGUI/CEGUI.h>
#include <chrono>
#include <map>

#include "CEGUIDrawable.h"
#include "GUIApplication.h"

int main()
{
	std::srand(std::chrono::system_clock::now().time_since_epoch().count());
    // setup CEGUI as OSG drawable
    osgViewer::Viewer viewer;
	GUIApplication guiApp(&viewer);

	const std::map<unsigned int, unsigned int> testMap;
	unsigned int myNull = 0;
	for (std::map<unsigned int, unsigned int>::const_iterator it = testMap.begin(); it != testMap.end(); ++it)
	{
		myNull = testMap.count(12);
	}

    osg::ref_ptr<osg::Group> root = new osg::Group;

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