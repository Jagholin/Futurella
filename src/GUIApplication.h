#pragma once

#include <CEGUI/String.h>
#include <CEGUI/Event.h>
#include <CEGUI/GUIContext.h>
//#include <OpenThreads/Thread>
#include <thread>

//#include <osgViewer/Viewer>

#include <boost/asio.hpp>
#include <memory>

#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>

#include "networking/networking.h"
#include "networking/peermanager.h"
#include "gamecommon/PhysicsEngine.h"

#include "ResourceManager.h"
#include "magnumdefs.h"

using CEGUI::String;
using CEGUI::Window;
class GameInstanceServer;
class GameInstanceClient;
class ShaderProvider;

class AsioThread
{
public:
    AsioThread();
    virtual ~AsioThread();

    void setService(const std::shared_ptr<boost::asio::io_service>& service);
    void setGuiService(const std::shared_ptr<boost::asio::io_service>& service);
    void run();

    bool createAndStartNetServer(unsigned int portNumberTCP, unsigned int portNumberUDP, std::string& errStr);
    bool startUdpListener(unsigned int portUDP, std::string& errStr);

    // virtual funcs from NetServerListener
    virtual void onConnection(NetConnection*);
    virtual void onStop();

protected:
    std::shared_ptr<boost::asio::io_service> m_servicePoint;
    std::shared_ptr<boost::asio::io_service> m_guiService;
    std::shared_ptr<NetServer> m_serverObject;

    addstd::signal<void()> m_destructionSignal;

    std::unique_ptr<std::thread> m_threadObj;

    mutable bool m_finished;
};

class GUIApplication : public Platform::Application
{
public:
    GUIApplication(/*osgViewer::Viewer* osgApp, osg::Group* rootGroup*/const Arguments&);
    virtual ~GUIApplication();
    // register event handlers in CEGUI
    void registerEvents();

    // helper function to register event handlers within CEGUI
    // windowName = path to the window capturing the event in GUI Layout hierarchy
    // eventName = String constant which defines the event type.
    // function = this Event::Subscriber will be called when the event is fired.
    // It will be most likely called deep within CeguiDrawable::drawImplementation context, so be careful about what you do
    // and how long it takes.
    void addEventHandler(const String& windowName, const String& eventName, const CEGUI::Event::Subscriber& function);

    void timeTick(float dt);

    // These Functions are called when CEGUI surface loses or regains input focus
    void hudLostFocus();
    void hudGotFocus();

    // Sets or hides goal cursor(CEGUI image)
    void showGoalCursorAt(float x, float y);
    void hideGoalCursor();

    // Retrieves or creates current Physics engine
    std::shared_ptr<PhysicsEngine> getOrCreatePhysicsEngine();

    // gui-triggered event handlers
    bool onQuitBtnClicked(const CEGUI::EventArgs&);
    bool onWindowCloseClicked(const CEGUI::EventArgs&);
    bool onSendBtnClicked(const CEGUI::EventArgs&);
    bool onConsoleClicked(const CEGUI::EventArgs&);
    bool onConsoleInput(const CEGUI::EventArgs&);
    bool onCreateServersAndJoinClicked(const CEGUI::EventArgs&);
    bool onJoinClicked(const CEGUI::EventArgs&);

    bool onCreateServerClicked(const CEGUI::EventArgs&);
    bool onJoinServerClicked(const CEGUI::EventArgs&);

    // network-related events
    // peermanager: newly connected peer
    void onNewFuturellaPeer(const RemoteMessagePeer::pointer& peer);
    // new message from network
    void onNetworkMessage(NetMessage::const_pointer msg, RemoteMessagePeer* sender);

protected:
    void drawEvent() override;
    void viewportEvent(const Vector2i& size) override;
    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;
    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;

    CEGUI::GUIContext* m_guiContext;
    //osgViewer::Viewer* m_osgApp;
    std::unique_ptr<Scene3D> m_rootGroup;
    CeguiDrawable* m_ceguiSurface; // memory managed by Magnum SceneGraph

    // perhaps this has to be moved somewhere else
    std::shared_ptr<boost::asio::io_service> m_networkService;
    std::shared_ptr<boost::asio::io_service> m_renderThreadService;
    AsioThread m_networkThread;
    
    GameInstanceServer* m_gameServer;
    GameInstanceClient* m_gameClient;
    std::vector<std::tuple<std::string, MessagePeer*>> m_availableGameServers;
    std::vector<Window*> m_animHideTargets;
    std::shared_ptr<ShaderProvider> m_shaderProvider;
    std::weak_ptr<PhysicsEngine> m_physicsEngine;

    // User data =====> TODO: Relocate this to somewhere else, PLEASE!
    bool m_userCreated;
    String m_userName;
    unsigned int m_userListensUdpPort;

    FuturellaResourceManager m_resourceManager;

    SceneGraph::DrawableGroup3D m_nearDrawables, m_farDrawables, m_uiDrawables;


    void doConsoleCommand(const String& command);
    // Functions replying on console commands
    void consoleCreateUser(const std::vector<String>& params, String& output);
    void consoleShowNetwork(const std::vector<String>& params, String& output);
    void consoleClear(const std::vector<String>& params, String& output);
    void consoleConnect(const std::vector<String>& params, String& output);
    void consoleNetServerCommand(const std::vector<String>& params, String& output);
    void consoleStartNetServer(const std::vector<String>& params, String& output);
    void consoleGameServerCommand(const std::vector<String>& params, String& output);
    void consoleStartGameServer(const std::vector<String>& params, String& output);
    void consoleListGameServers(const std::vector<String>& params, String& output);
    void consoleConnectGameServer(const std::vector<String>& params, String& output);
    void consoleMacroCommand(const std::vector<String>& params, String& output);
    void consoleOpenShaderEditor(const std::vector<String>& params, String& output);
};
