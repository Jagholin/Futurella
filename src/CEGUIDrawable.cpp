#include <algorithm>
#include <atomic>
//#include "glincludes.h"
#include "CEGUIDrawable.h"
#include "GUIApplication.h"

#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>
//#include <osg/Texture2D>
//#include <osgGA/GUIEventHandler>
//#include <osg/GLExtensions>
//#include <osg/Timer>
//#include <osg/Depth>

/*class GlobalEventHandler : public osgGA::GUIEventHandler
{
    std::atomic_bool m_guiHandlesEvents;
    GUIApplication* m_guiApp;
public:

    GlobalEventHandler(GUIApplication* app)
    {
        m_guiApp = app;
        m_guiHandlesEvents = true;

        using namespace CEGUI;
        // Click on the root window(outside of all other widgets) will switch back to player controls.
        Window *root = System::getSingleton().getDefaultGUIContext().getRootWindow();
        root->subscribeEvent(Window::EventMouseClick, Event::Subscriber(&GlobalEventHandler::resetGuiEventHandling, this));
    }

    virtual bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa, osg::Object *o, osg::NodeVisitor *)
    {
        osgGA::GUIEventAdapter::EventType etype = ea.getEventType();

        CeguiDrawable* myDrawable = static_cast<CeguiDrawable*>(o);

        if (etype & (osgGA::GUIEventAdapter::RELEASE | osgGA::GUIEventAdapter::PUSH | osgGA::GUIEventAdapter::MOVE | osgGA::GUIEventAdapter::DRAG | osgGA::GUIEventAdapter::KEYDOWN | osgGA::GUIEventAdapter::KEYUP))
        {
            if (etype == osgGA::GUIEventAdapter::KEYDOWN && ea.getKey() == osgGA::GUIEventAdapter::KEY_F7)
            {
                myDrawable->addEvent(ea);
                return true;
            }
            if (!m_guiHandlesEvents && etype == osgGA::GUIEventAdapter::KEYDOWN && ea.getKey() == osgGA::GUIEventAdapter::KEY_Tab)
            {
                using namespace CEGUI;

                m_guiHandlesEvents = true;
                GUIContext & gui = System::getSingleton().getDefaultGUIContext();
                gui.getMouseCursor().show();
                m_guiApp->hudGotFocus();
                return true;
            }
            if (!m_guiHandlesEvents)
                return false;
            // HACK: don't send events directly to CEGUI, instead wait until the required OpenGL context is active
            // as CEGUI will probably try to change OpenGL objects/state, which will SILENTLY fail in case of the
            // wrong or no active OpenGL context(thanks to gDEBugger I've actually seen this happen).
            // which means, that we should do the injection in draw phase...
            myDrawable->addEvent(ea);
            return true;
        }
        return false;
    }

    bool resetGuiEventHandling(const CEGUI::EventArgs&)
    {
        m_guiHandlesEvents = false;
        using CEGUI::GUIContext;
        GUIContext & gui = CEGUI::System::getSingleton().getDefaultGUIContext();
        gui.getMouseCursor().hide();
        m_guiApp->hudLostFocus();
        return true;
    }
};*/

/*class TickEvents : public osg::Drawable::UpdateCallback
{
protected:
    osg::Timer_t m_lastTick;
public:
    TickEvents()
    {
        m_lastTick = osg::Timer::instance()->tick();
    }
    virtual void update(osg::NodeVisitor*, osg::Drawable*)
    {
        // Inject tick update into CEGUI system
        osg::Timer_t newTick = osg::Timer::instance()->tick();
        double msPast = osg::Timer::instance()->delta_m(m_lastTick, newTick);
        CEGUI::System::getSingleton().getDefaultGUIContext().injectTimePulse(msPast/1000.0);
        CEGUI::System::getSingleton().injectTimePulse(msPast/1000.0);
        m_lastTick = newTick;
    }
};*/

bool CeguiDrawable::m_exists = false;

CeguiDrawable::CeguiDrawable(GUIApplication* app, Object3D* parent, SceneGraph::DrawableGroup3D* dgroup) :
    Object3D(parent), SceneGraph::Drawable3D(*this, dgroup), m_initDone(false), m_guiApp(app)
{
    if (m_exists)
        throw std::logic_error("No other instances of CeguiDrawable allowed");
    if (!m_exists)
    {
        m_exists = true;
    }
    //setUseDisplayList(false);
    //setSupportsDisplayList(false);
    //m_tex = new osg::Texture2D;
    //m_pr = new osg::Program;
    //m_state = new osg::StateSet;

    //m_state->setTextureAttributeAndModes(0, m_tex, osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF);
    //m_state->setAttributeAndModes(m_pr, osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF);
    //m_state->setAttributeAndModes(new osg::Depth, osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF);

    //m_renderThreadService = std::make_shared<boost::asio::io_service>();

    m_keyboardMap = std::map<int, CEGUI::Key::Scan> { { SDLK_RETURN, CEGUI::Key::Return },
    { SDLK_BACKSPACE, CEGUI::Key::Backspace },
    { SDLK_LEFT, CEGUI::Key::ArrowLeft },
    { SDLK_UP, CEGUI::Key::ArrowUp },
    { SDLK_DOWN, CEGUI::Key::ArrowDown },
    { SDLK_RIGHT, CEGUI::Key::ArrowRight },
    { SDLK_TAB, CEGUI::Key::Tab } };

    init();
}

void CeguiDrawable::passEvent(const Platform::Application::MouseEvent& ea, bool pressed)
{
    using namespace CEGUI;

    MouseButton mbutton;
    mbutton = (ea.button() == Platform::Application::MouseEvent::Button::Left) ? LeftButton : RightButton;
    GUIContext & gui = System::getSingleton().getDefaultGUIContext();
    if (pressed)
    {
        gui.injectMouseButtonDown(mbutton);
    }
    else
    {
        gui.injectMouseButtonUp(mbutton);
    }
}

void CeguiDrawable::passEvent(const Platform::Application::MouseMoveEvent& ea)
{
    using namespace CEGUI;

    GUIContext & gui = System::getSingleton().getDefaultGUIContext();
    gui.injectMousePosition(ea.position().x() /* - ea.getXmin()*/, /*ea.getYmax() -*/ ea.position().y() /*+ ea.getYmin()*/);
    //gui.injectMousePosition(ea.getX(), ea.getY());
}

void CeguiDrawable::passEvent(const Platform::Application::KeyEvent& ea, bool pressed)
{
    using namespace CEGUI;
    if (pressed)
    {
        int keyCode = static_cast<int>(ea.key());
        if (keyCode == (int)Platform::Application::KeyEvent::Key::F7)
        {
            // Special key: reload HUD layout
            WindowManager& win = WindowManager::getSingleton();
            win.destroyAllWindows();
            GUIContext& gui = System::getSingleton().getDefaultGUIContext();
            Window* root = win.loadLayoutFromFile("test.layout");
            gui.setRootWindow(root);
            m_guiApp->registerEvents();
            root->subscribeEvent(Window::EventMouseClick, Event::Subscriber(&GlobalEventHandler::resetGuiEventHandling, const_cast<GlobalEventHandler*>(static_cast<const GlobalEventHandler*>(getEventCallback()))));
            return;
        }
        if (m_keyboardMap.count(keyCode))
        {
            System::getSingleton().getDefaultGUIContext().injectKeyDown(m_keyboardMap.at(keyCode));
        }
        else
        {
            System::getSingleton().getDefaultGUIContext().injectKeyDown(static_cast<CEGUI::Key::Scan>(ea.getKey()));
            System::getSingleton().getDefaultGUIContext().injectChar(static_cast<CEGUI::utf32>(ea.getKey()));
        }
    }
    else if (etype & osgGA::GUIEventAdapter::KEYUP)
    {
        int keyCode = ea.getKey();
        if (m_keyboardMap.count(keyCode))
        {
            System::getSingleton().getDefaultGUIContext().injectKeyUp(m_keyboardMap.at(keyCode));
        }
        else
        {
            System::getSingleton().getDefaultGUIContext().injectKeyUp(static_cast<CEGUI::Key::Scan>(keyCode));
        }
    }
}

void CeguiDrawable::drawImplementation( osg::RenderInfo& renderInfo ) const
{
    osg::State* myState = renderInfo.getState();

    myState->pushStateSet(m_state);
    myState->apply();
    myState->setActiveTextureUnit(0);
    GLuint old_vao = 0, old_vbo = 0;
    myState->disableAllVertexArrays();
    // CEGUI OpenGL renderer uses VAO to pass geometry, but doesn't restore old bindings
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, reinterpret_cast<GLint*>(&old_vao));
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&old_vbo));
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

    if (!m_initDone)
    {
        if (!glBindVertexArray)
            glFuncsInit();
        init();
    }

    // While we still have the right GL context enabled,
    // pass GUI events to CEGUI
    m_eventMutex.lock();
    for (osg::ref_ptr<osgGA::GUIEventAdapter> e : m_eventQueue)
        passEvent(*e.get());
    m_eventQueue.clear();

    // Run all functions that have to be evaluated in the render thread
    m_renderThreadService->poll();
    if (m_renderThreadService->stopped())
        m_renderThreadService->reset();
    m_eventMutex.unlock();

    CEGUI::System::getSingleton().renderAllGUIContexts();

    glPopClientAttrib();
    glPopAttrib();
    glBindVertexArray(old_vao);
    glBindBuffer(GL_ARRAY_BUFFER, old_vbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef _DEBUG
    myState->checkGLErrors("CeguiDrawable::drawImplementation");
#endif
}

CeguiDrawable& CeguiDrawable::init()
{
    if (m_initDone)
        return;
    CEGUI::OpenGL3Renderer::bootstrapSystem();

    CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>(CEGUI::System::getSingleton().getResourceProvider());

    rp->setResourceGroupDirectory("schemes", "./cegui/schemes/");
    rp->setResourceGroupDirectory("imagesets", "./cegui/imagesets/");
    rp->setResourceGroupDirectory("fonts", "./cegui/fonts/");
    rp->setResourceGroupDirectory("looknfeels", "./cegui/looknfeel/");
    rp->setResourceGroupDirectory("layouts", "./cegui/layouts/");
    rp->setResourceGroupDirectory("anims", "./cegui/animations/");

    CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");
    CEGUI::AnimationManager::setDefaultResourceGroup("anims");

    CEGUI::SchemeManager::getSingleton().createFromFile("Generic.scheme");

    CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage("TaharezLook/MouseArrow");
    //CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultFont("DejaVuSans-12");
    //CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultTooltipType("TaharezLook/Tooltip");

    using namespace CEGUI;
    Window* myRoot = WindowManager::getSingleton().loadLayoutFromFile("futurella.xml");
    System::getSingleton().getDefaultGUIContext().setRootWindow(myRoot);

    //AnimationManager::getSingleton().loadAnimationsFromXML("example.anims");

    m_guiApp->registerEvents();

    m_initDone = true;

    return *this;
}
