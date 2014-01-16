#pragma once

#include <CEGUI/String.h>
#include <CEGUI/Event.h>
#include <CEGUI/GUIContext.h>
#include <OpenThreads/Thread>

#include <osgViewer/Viewer>

#include <boost/asio.hpp>
#include <memory>

#include "networking/networking.h"
#include "networking/peermanager.h"

using CEGUI::String;
class Level;

class AsioThread : public OpenThreads::Thread
{
public:
    AsioThread();
    virtual ~AsioThread();

    void setService(const std::shared_ptr<boost::asio::io_service>& service);
    void setGuiService(const std::shared_ptr<boost::asio::io_service>& service);
    virtual void run();

    bool createAndStartNetServer(unsigned int portNumber, std::string& errStr);

    // virtual funcs from NetServerListener
    virtual void onConnection(NetConnection*);
    virtual void onStop();

protected:
    std::shared_ptr<boost::asio::io_service> m_servicePoint;
    std::shared_ptr<boost::asio::io_service> m_guiService;
    std::shared_ptr<NetServer> m_serverObject;

    addstd::signal<void()> m_destructionSignal;
};

class GUIApplication : public osg::Referenced
{
public:
    GUIApplication(osgViewer::Viewer* osgApp);

    // register event handlers in CEGUI
    void registerEvents();

    void setGuiService(const std::shared_ptr<boost::asio::io_service>& service);
    void setCurrentLevel(Level* levelData);

    // gui-triggered event handlers
    bool onQuitBtnClicked(const CEGUI::EventArgs&);
    bool onWindowCloseClicked(const CEGUI::EventArgs&);
    bool onSendBtnClicked(const CEGUI::EventArgs&);
    bool onConnectBtnClicked(const CEGUI::EventArgs&);
    bool onListenBtnClicked(const CEGUI::EventArgs&);

    // network-related events
    // peermanager: newly connected peer
    void onNewFuturellaPeer(const RemoteMessagePeer::pointer& peer);
    // new message from network
    void onNetworkMessage(NetMessage::const_pointer msg, RemoteMessagePeer* sender);

protected:
    CEGUI::GUIContext* m_guiContext;
    osgViewer::Viewer* m_osgApp;

    // perhaps this has to be moved somewhere else
    std::shared_ptr<boost::asio::io_service> m_networkService;
    std::shared_ptr<boost::asio::io_service> m_renderThreadService;
    AsioThread m_networkThread;
    Level* m_currentLevel;

    // helper function to register event handlers within CEGUI
    // windowName = path to the window capturing the event in GUI Layout hierarchy
    // eventName = String constant which defines the event type.
    // function = this Event::Subscriber will be called when the event is fired.
    // It will be most likely called deep within CeguiDrawable::drawImplementation context, so be careful about what you do
    // and how long it takes.
    void addEventHandler(const String& windowName, const String& eventName, const CEGUI::Event::Subscriber& function);
};
