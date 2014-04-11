#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Resource.h>
#include <Magnum/Texture.h>
#include <Magnum/AbstractShaderProgram.h>

#include <Magnum/Platform/Sdl2Application.h>
#include <CEGUI/CEGUI.h>
#include <deque>

#include <boost/asio.hpp>
#include "magnumdefs.h"

class GUIApplication;

class CeguiDrawable: public Object3D, public SceneGraph::Drawable3D
{
public:
    CeguiDrawable(GUIApplication* app, Object3D* parent = nullptr, SceneGraph::DrawableGroup3D* dgroup = nullptr);
    CeguiDrawable(const CeguiDrawable& rhs) = delete;

    // We should initialize GUIApplication somewhere after CEGUI itself is initialized,
    // which is udoable from main() function.
    CeguiDrawable& init();

    //void addEvent(const osgGA::GUIEventAdapter&);
    void passEvent(const Platform::Application::KeyEvent&, bool pressed);
    void passEvent(const Platform::Application::MouseEvent&, bool pressed);
    void passEvent(const Platform::Application::MouseMoveEvent&);
protected:
    //virtual void drawImplementation( osg::RenderInfo& renderInfo ) const;
    virtual void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) override;

protected:
    mutable bool m_initDone;
    static bool m_exists;
    //osg::ref_ptr<osg::Texture2D> m_tex;
    //Resource<Texture2D> m_tex;
    //osg::ref_ptr<osg::Program> m_pr;
    //Resource<AbstractShaderProgram> m_pr;
    //osg::ref_ptr<osg::StateSet> m_state;

    //mutable std::deque<osg::ref_ptr<osgGA::GUIEventAdapter> > m_eventQueue;
    //mutable OpenThreads::Mutex m_eventMutex;

    mutable unsigned int m_skipCounter;

    GUIApplication* m_guiApp;
    //std::shared_ptr<boost::asio::io_service> m_renderThreadService;

    std::map<int, CEGUI::Key::Scan> m_keyboardMap;
};