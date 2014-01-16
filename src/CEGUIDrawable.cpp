#include <algorithm>
#include <atomic>
#include "glincludes.h"
#include "CEGUIDrawable.h"
#include "GUIApplication.h"

#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>
#include <osg/Texture2D>
#include <osgGA/GUIEventHandler>
#include <osg/GLExtensions>
#include <osg/Timer>
#include <osg/Depth>

PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLBINDBUFFERPROC glBindBuffer;

class GlobalEventHandler : public osgGA::GUIEventHandler
{
    std::atomic_bool m_guiHandlesEvents;
public:

    GlobalEventHandler()
    {
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
            if (!m_guiHandlesEvents && etype == osgGA::GUIEventAdapter::KEYDOWN && ea.getKey() == osgGA::GUIEventAdapter::KEY_Tab)
            {
                using namespace CEGUI;

                m_guiHandlesEvents = true;
                GUIContext & gui = System::getSingleton().getDefaultGUIContext();
                gui.getMouseCursor().show();
                return true;
            }
            if (!m_guiHandlesEvents)
                return false;
            // HACK: don't send events directly to CEGUI, instead wait until the required OpenGL context is active
            // as CEGUI will probably try to change OpenGL objects/state, which will SILENTLY fail in case of the
            // wrong or no active OpenGL context(thanks to gDEBugger I've actually seen this happen).
            // which means, that we should do the injection in draw phase...
            myDrawable->addEvent(ea);
        }
        return true;
    }

    bool resetGuiEventHandling(const CEGUI::EventArgs&)
    {
        m_guiHandlesEvents = false;
        using CEGUI::GUIContext;
        GUIContext & gui = CEGUI::System::getSingleton().getDefaultGUIContext();
        gui.getMouseCursor().hide();
        return true;
    }
};

class TickEvents : public osg::Drawable::UpdateCallback
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
};

bool CeguiDrawable::m_exists = false;


CeguiDrawable::CeguiDrawable() :
    m_initDone(false), m_guiApp(nullptr)
{
    if (m_exists)
        throw std::logic_error("No other instances of CeguiDrawable allowed");
    if (!m_exists)
    {
        m_exists = true;
    }
    setUseDisplayList(false);
    setSupportsDisplayList(false);
    m_tex = new osg::Texture2D;
    m_pr = new osg::Program;
    m_state = new osg::StateSet;

    m_state->setTextureAttributeAndModes(0, m_tex, osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF);
    m_state->setAttributeAndModes(m_pr, osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF);
    m_state->setAttributeAndModes(new osg::Depth, osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF);

    m_renderThreadService = std::make_shared<boost::asio::io_service>();

    m_keyboardMap = std::map<int, CEGUI::Key::Scan> { { osgGA::GUIEventAdapter::KEY_Return, CEGUI::Key::Return },
    { osgGA::GUIEventAdapter::KEY_BackSpace, CEGUI::Key::Backspace },
    { osgGA::GUIEventAdapter::KEY_Tab, CEGUI::Key::Tab } };
}

void CeguiDrawable::setGuiApplication(GUIApplication* app)
{
    m_guiApp = app;
    m_guiApp->setGuiService(m_renderThreadService);
}

void CeguiDrawable::addEvent(const osgGA::GUIEventAdapter& e)
{
    if (e.getEventType() & (osgGA::GUIEventAdapter::RELEASE | osgGA::GUIEventAdapter::PUSH))
    {
        passEvent(e);
        return;
    }
    // Yep OSG does multi threaded render by default in my case.
    // So this sync is vital.
    m_eventMutex.lock();
    m_eventQueue.push_back(static_cast<osgGA::GUIEventAdapter*>(e.clone(osg::CopyOp::DEEP_COPY_ALL)));
    m_eventMutex.unlock();
}

void CeguiDrawable::passEvent(const osgGA::GUIEventAdapter& ea) const
{
    using namespace CEGUI;

    osgGA::GUIEventAdapter::EventType etype = ea.getEventType();

    if (etype & (osgGA::GUIEventAdapter::RELEASE | osgGA::GUIEventAdapter::PUSH))
    {
        MouseButton mbutton;
        mbutton = ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON ? LeftButton : RightButton;
        GUIContext & gui = System::getSingleton().getDefaultGUIContext();
        if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            gui.injectMouseButtonDown(mbutton);
        }
        else
        {
            gui.injectMouseButtonUp(mbutton);
        }
    }
    else if (etype & osgGA::GUIEventAdapter::MOVE)
    {
        GUIContext & gui = System::getSingleton().getDefaultGUIContext();
        gui.injectMousePosition(ea.getX() - ea.getXmin(), ea.getYmax() - ea.getY() + ea.getYmin());
        //gui.injectMousePosition(ea.getX(), ea.getY());
    }
    else if (etype & osgGA::GUIEventAdapter::DRAG)
    {
        GUIContext & gui = System::getSingleton().getDefaultGUIContext();
        gui.injectMousePosition(ea.getX() - ea.getXmin(), ea.getYmax() - ea.getY() + ea.getYmin());
        //gui.injectMousePosition(ea.getX(), ea.getY());
    }
    else if (etype & (osgGA::GUIEventAdapter::KEYDOWN))
    {
        int keyCode = ea.getKey();
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
            System::getSingleton().getDefaultGUIContext().injectKeyDown(m_keyboardMap.at(keyCode));
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

    myState->checkGLErrors("CeguiDrawable::drawImplementation");
}

void CeguiDrawable::init() const
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

    CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");

    CEGUI::SchemeManager::getSingleton().createFromFile("TaharezLook.scheme");

    CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage("TaharezLook/MouseArrow");

    using namespace CEGUI;
    Window* myRoot = WindowManager::getSingleton().loadLayoutFromFile("test.layout");
    System::getSingleton().getDefaultGUIContext().setRootWindow(myRoot);

    /// HACKS, hacks hacks all around...
    // well, what else can you do here, i have no idea.
    glBindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(osg::getGLExtensionFuncPtr("glBindVertexArray", "glBindVertexArrayEXT", "glBindVertexArrayARB"));
    glBindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(osg::getGLExtensionFuncPtr("glBindFramebuffer"));
    glBindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(osg::getGLExtensionFuncPtr("glBindBuffer"));

    CeguiDrawable* self = const_cast<CeguiDrawable*>(this);
    self->setEventCallback(new GlobalEventHandler);
    self->setUpdateCallback(new TickEvents);
    self->m_guiApp->registerEvents();

    m_initDone = true;
}
