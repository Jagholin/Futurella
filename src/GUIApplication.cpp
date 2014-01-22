#include "GUIApplication.h"

#include <CEGUI/CEGUI.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "networking/networking.h"
#include "networking/peermanager.h"
#include "Level.h"
#include "gameclient/GameInstanceClient.h"
#include "gameserver/GameInstanceServer.h"

using namespace CEGUI;

// Some netmessages that inform about existing GameInstanceServers or request a connection
// between GameInstanceServer and GameInstanceClient

BEGIN_DECLNETMESSAGE(GameServerAvailable, 1400, false)
std::string serverName;
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(GameConnectRequest, 1401, false)
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(GameConnectDenied, 1402, false)
std::string reason;
END_DECLNETMESSAGE()

BEGIN_NETTORAWMESSAGE_QCONVERT(GameServerAvailable)
out << serverName;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(GameServerAvailable)
in >> serverName;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(GameConnectRequest)
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(GameConnectRequest)
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(GameConnectDenied)
out << reason;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(GameConnectDenied)
in >> reason;
END_RAWTONETMESSAGE_QCONVERT()

REGISTER_NETMESSAGE(GameServerAvailable)
REGISTER_NETMESSAGE(GameConnectRequest)
REGISTER_NETMESSAGE(GameConnectDenied)

AsioThread::AsioThread() :
m_destructionSignal("AsioThread::destructionSignal")
{
    // nop
}

AsioThread::~AsioThread()
{
    if (isRunning())
    {
        if (m_serverObject)
            m_serverObject->stop();
        m_servicePoint->stop();
        join();
    }
    m_destructionSignal();
}

void AsioThread::setService(const std::shared_ptr<boost::asio::io_service>& netService)
{
    m_servicePoint = netService;
}

void AsioThread::setGuiService(const std::shared_ptr<boost::asio::io_service>& service)
{
    m_guiService = service;
}

void AsioThread::run()
{
    m_guiService->post([](){
        GUIContext& gui = System::getSingleton().getDefaultGUIContext();
        Window* window = gui.getRootWindow()->getChild("console/output");

        if (window)
        {
            window->appendText("\n Network service thread is about to start...");
        }
    });

    try 
    {
        m_servicePoint->run();
        m_servicePoint->reset();
    }
    catch (const boost::system::system_error &err)
    {
        m_guiService->post([err](){
            GUIContext& gui = System::getSingleton().getDefaultGUIContext();
            Window* window = gui.getRootWindow()->getChild("console/output");

            if (window)
            {
                window->appendText(String("\n") + err.what());
            }
        });
    }

    m_guiService->post([](){
        GUIContext& gui = System::getSingleton().getDefaultGUIContext();
        Window* window = gui.getRootWindow()->getChild("console/output");

        if (window)
        {
            window->appendText("\n Network service thread stops. Further communication is impossible.");
        }
    });
}

bool AsioThread::createAndStartNetServer(unsigned int portNumber, unsigned int portNumberUDP, std::string& errStr)
{
    std::shared_ptr<boost::asio::io_service::work> tempWork;
    if (!m_servicePoint)
    {
        errStr = "No service object bound. Refusing to start NetServer.";
        return false;
    }
    if (!m_serverObject)
    {
        m_serverObject = std::make_shared<NetServer>(*m_servicePoint);
        m_serverObject->onNewConnection(std::bind(&AsioThread::onConnection, this, std::placeholders::_1), m_destructionSignal);
        m_serverObject->onStopped(std::bind(&AsioThread::onStop, this), m_destructionSignal);
    }
    if (m_serverObject->isRunning())
    {
        // prevent network thread from stopping
        tempWork = std::make_shared<boost::asio::io_service::work>(*m_servicePoint);
        m_serverObject->stop();
    }
    bool res = m_serverObject->listen(portNumber, portNumberUDP, errStr);
    if (res)
    {
        if (!isRunning())
            res = (start() == 0);
    }
    return res;
}

void AsioThread::onConnection(NetConnection* newConnection)
{
    m_guiService->post([](){
        GUIContext& gui = System::getSingleton().getDefaultGUIContext();
        Window* window = gui.getRootWindow()->getChild("console/output");

        if (window)
        {
            window->appendText(String("\nNew connection added"));
        }
    });
    // Create new RemoteMessagePeer object
    // that is automatically registered in RemotePeersManager
    new RemoteMessagePeer(newConnection, true, *m_servicePoint);
}

void AsioThread::onStop()
{
    // TODO
    m_guiService->post([](){
        GUIContext& gui = System::getSingleton().getDefaultGUIContext();
        Window* window = gui.getRootWindow()->getChild("console/output");

        if (window)
        {
            window->appendText(String("\nNetServer stopped"));
        }
    });
}

bool AsioThread::startUdpListener(unsigned int portUDP, std::string& errStr)
{
    if (!m_servicePoint)
    {
        errStr = "No service object bound. Refusing to start NetServer.";
        return false;
    }
    if (!m_serverObject)
    {
        m_serverObject = std::make_shared<NetServer>(*m_servicePoint);
        m_serverObject->onNewConnection(std::bind(&AsioThread::onConnection, this, std::placeholders::_1), m_destructionSignal);
        m_serverObject->onStopped(std::bind(&AsioThread::onStop, this), m_destructionSignal);
    }
    if (m_serverObject->isRunning())
    {
        errStr = "Server is already running, ignoring the command.";
        //m_serverObject->stop();
        return false;
    }
    bool res = m_serverObject->listenUdp(portUDP, errStr);
    if (res)
    {
        if (!isRunning())
            res = (start() == 0);
    }
    return res;
}

GUIApplication::GUIApplication(osgViewer::Viewer* osgApp)
{
    m_guiContext = nullptr;
    m_osgApp = osgApp;
    m_networkService = std::make_shared<boost::asio::io_service>();
    m_networkThread.setService(m_networkService);
    m_currentLevel = nullptr;
    m_userCreated = false;
    m_gameClient = nullptr;
    m_gameServer = nullptr;
    RemotePeersManager::getManager()->onNewPeerRegistration(std::bind(&GUIApplication::onNewFuturellaPeer, this, std::placeholders::_1), this);
}

void GUIApplication::setCurrentLevel(Level* levelData)
{
    m_currentLevel = levelData;
}

void GUIApplication::registerEvents()
{
    m_guiContext = &(System::getSingleton().getDefaultGUIContext());

    addEventHandler("chatWindow/quitBtn", PushButton::EventClicked, Event::Subscriber(&GUIApplication::onQuitBtnClicked, this));
    addEventHandler("networkSettings", FrameWindow::EventCloseClicked, Event::Subscriber(&GUIApplication::onWindowCloseClicked, this));
    addEventHandler("chatWindow", FrameWindow::EventCloseClicked, Event::Subscriber(&GUIApplication::onWindowCloseClicked, this));
    addEventHandler("chatWindow/sendBtn", PushButton::EventClicked, Event::Subscriber(&GUIApplication::onSendBtnClicked, this));
    addEventHandler("chatWindow/input", Editbox::EventTextAccepted, Event::Subscriber(&GUIApplication::onSendBtnClicked, this));

    addEventHandler("networkSettings/connectBtn", PushButton::EventClicked, Event::Subscriber(&GUIApplication::onConnectBtnClicked, this));
    addEventHandler("networkSettings/listenBtn", PushButton::EventClicked, Event::Subscriber(&GUIApplication::onListenBtnClicked, this));

    addEventHandler("console/input", Window::EventMouseClick, Event::Subscriber(&GUIApplication::onConsoleClicked, this));
    addEventHandler("console/output", Window::EventMouseClick, Event::Subscriber(&GUIApplication::onConsoleClicked, this));
    addEventHandler("console/input", Editbox::EventTextAccepted, Event::Subscriber(&GUIApplication::onConsoleInput, this));
}

void GUIApplication::setGuiService(const std::shared_ptr<boost::asio::io_service>& service)
{
    m_renderThreadService = service;
    m_networkThread.setGuiService(service);
}

void GUIApplication::addEventHandler(const String& windowName, const String& eventName, const CEGUI::Event::Subscriber& callback)
{
    Window* wRoot = m_guiContext->getRootWindow();

    if (wRoot->isChild(windowName))
    {
        Window* myWindow = wRoot->getChild(windowName);
        myWindow->subscribeEvent(eventName, callback);
    }
}

bool GUIApplication::onQuitBtnClicked(const EventArgs& args)
{
    m_osgApp->setDone(true);
    return true;
}

bool GUIApplication::onSendBtnClicked(const EventArgs& args)
{
    Window* myOutput = m_guiContext->getRootWindow()->getChild("chatWindow/output");
    MultiLineEditbox* realOutput = static_cast<MultiLineEditbox*>(myOutput);
    Editbox* realInput = static_cast<Editbox*>(m_guiContext->getRootWindow()->getChild("chatWindow/input"));
    if (realOutput && realInput)
    {
        String newLine = realInput->getText();
        if (newLine.empty())
            newLine = "You clicked me!";
        else
        {
            RemotePeersManager::getManager()->sendChatMessage(newLine.c_str());
        }
        realOutput->appendText(newLine);
        realInput->setText("");
    }
    return true;
}

bool GUIApplication::onConnectBtnClicked(const EventArgs& args)
{
    Window* addrWindow = m_guiContext->getRootWindow()->getChild("networkSettings/address");
    if (addrWindow)
    {
        // Create the NetConnection and associated peer
        NetConnection* newConnection = new NetConnection(*m_networkService);
        RemoteMessagePeer* myPeer = new RemoteMessagePeer(newConnection, false, *m_networkService);

        newConnection->connectTo(addrWindow->getText().c_str(), 1778, 1779);
        if (!m_networkThread.isRunning())
            m_networkThread.start();
    }
    return true;
}

bool GUIApplication::onListenBtnClicked(const EventArgs& args)
{
    Window* portNumber = m_guiContext->getRootWindow()->getChild("networkSettings/portNumber");
    if (portNumber)
    {
        // Instantiating the server 
        unsigned int port = boost::lexical_cast<unsigned int>(portNumber->getText());
        std::string err;
        bool result = m_networkThread.createAndStartNetServer(port, m_userListensUdpPort, err);
        if (m_currentLevel)
            m_currentLevel->setServerSide(true);
    }
    return true;
}

bool GUIApplication::onWindowCloseClicked(const EventArgs& args)
{
    const WindowEventArgs* realArgs = static_cast<const WindowEventArgs*>(&args);
    realArgs->window->setVisible(false);
    return true;
}

void GUIApplication::onNewFuturellaPeer(const RemoteMessagePeer::pointer& peer)
{
    peer->onActivation([&, peer](){
        m_currentLevel->connectLocallyTo(peer);
        std::cout << "peer activated\n";
    }, this);

    peer->onActivation(m_renderThreadService->wrap([&](){
        Window* mBox = m_guiContext->getRootWindow()->getChild("console/output");
        mBox->appendText("\nPeer activated");
    }), this);

    peer->onPeerDestruction(m_renderThreadService->wrap([&](){
        Window* mBox = m_guiContext->getRootWindow()->getChild("console/output");
        mBox->appendText("\nPeer destroyed");
    }), this);

    peer->onError(m_renderThreadService->wrap([&](const std::string &str){
        Window* mBox = m_guiContext->getRootWindow()->getChild("console/output");
        mBox->appendText(String("\n") + str);
    }), this);

    peer->onMessage(std::bind(&GUIApplication::onNetworkMessage, this, std::placeholders::_1, std::placeholders::_2), this);
}

void GUIApplication::onNetworkMessage(NetMessage::const_pointer msg, RemoteMessagePeer* sender)
{
    // We are right now inside networking thread(obviously),
    // so be careful
    if (msg->gettype() == NetChatMessage::type)
    {
        // GUI can only be changed from the GUI thread/and context,
        // so go there
        m_renderThreadService->post([&, msg](){
            MultiLineEditbox* chatWindow = static_cast<MultiLineEditbox*>(m_guiContext->getRootWindow()->getChild("chatWindow/output"));
            NetChatMessage::const_pointer realMsg{msg->as<NetChatMessage>()};
            chatWindow->appendText(realMsg->message);
        });
    }
}

bool GUIApplication::onConsoleClicked(const CEGUI::EventArgs& a)
{
    Editbox* inputBox = static_cast<Editbox*>(m_guiContext->getRootWindow()->getChild("console/input"));
    //m_guiContext->setInputCaptureWindow(inputBox);
    inputBox->activate();
    return true;
}

bool GUIApplication::onConsoleInput(const CEGUI::EventArgs&)
{
    Editbox* inputBox = static_cast<Editbox*>(m_guiContext->getRootWindow()->getChild("console/input"));
    Window* outputBox = m_guiContext->getRootWindow()->getChild("console/output");
    String consoleCommand = inputBox->getText();
    inputBox->setText("");

    std::vector<String> rawParts, consoleParts;
    boost::trim(consoleCommand);
    boost::split(rawParts, consoleCommand, boost::is_space());
    for (auto s : rawParts)
    {
        boost::trim(s);
        if (s.size() > 0)
            consoleParts.push_back(s);
    }
    typedef std::map < String, std::function<void(const std::vector<String>&, String&)> > consoleFuncsMap;
    auto consoleHelpProcedure = [](const consoleFuncsMap& funcs, const std::vector<String>&, String& out){
        out = "\nKnown console commands include:";
        for (auto f : funcs)
        {
            out += " " + f.first;
        }
    };

    static const consoleFuncsMap consoleFuncs = consoleFuncsMap {
        { "login", std::bind(&GUIApplication::consoleCreateUser, this, std::placeholders::_1, std::placeholders::_2) },
        { "show_network", std::bind(&GUIApplication::consoleShowNetwork, this, std::placeholders::_1, std::placeholders::_2) },
        { "clear", std::bind(&GUIApplication::consoleClear, this, std::placeholders::_1, std::placeholders::_2) },
        { "netserver", std::bind(&GUIApplication::consoleNetServerCommand, this, std::placeholders::_1, std::placeholders::_2) },
        { "connect", std::bind(&GUIApplication::consoleConnect, this, std::placeholders::_1, std::placeholders::_2) },
        { "help", std::bind(consoleHelpProcedure, std::cref(consoleFuncs), std::placeholders::_1, std::placeholders::_2) }
    };

    if (consoleFuncs.count(consoleParts[0]) > 0)
    {
        String outputStr;
        consoleFuncs.at(consoleParts[0])(consoleParts, outputStr);
        outputBox->appendText(outputStr);
    }
    else
        outputBox->appendText(String("\nunknown command: ") + consoleParts[0]);
    return true;
}

void GUIApplication::consoleCreateUser(const std::vector<String>& params, String& output)
{
    // 1 parameter: name
    // 2 parameter: preferred udp listening port

    if (m_userCreated)
    {
        output = String("\nLogin already performed as: ") + m_userName + ". No need to do it again.";
    }
    if (params.size() < 3)
    {
        output = "\nExpected more parameters: login <userName> <udpPort>";
        return;
    }

    try
    {
        m_userName = params[1];
        m_userListensUdpPort = boost::lexical_cast<unsigned int>(params[2]);
        m_userCreated = true;
    }
    catch (boost::bad_lexical_cast)
    {
        m_userCreated = false;
        output = "\nWrong parameter types. Expected login <userName> <udpPort>";
        return;
    }

    RemotePeersManager::getManager()->setMyName(m_userName.c_str());
    // TODO: start listening at given UDP port.
    std::string errStr;
    if (!m_networkThread.startUdpListener(m_userListensUdpPort, errStr))
    {
        m_userCreated = false;
        output = "\nCannot start udp listener. User initialization failure.";
        return;
    }

    output = "\nLogin successful, " + m_userName;
}

void GUIApplication::consoleShowNetwork(const std::vector<String>& params, String& output)
{
    Window* targetWindow = m_guiContext->getRootWindow()->getChild("networkSettings");
    targetWindow->show();
    output = "\ncommand accepted.";
}

void GUIApplication::consoleClear(const std::vector<String>& params, String& output)
{
    Window* targetWindow = m_guiContext->getRootWindow()->getChild("console/output");
    targetWindow->setText("");
}

void GUIApplication::consoleConnect(const std::vector<String>& params, String& output)
{
    if (!m_userCreated)
    {
        output = "\nThis command is only available after log-in";
        return;
    }
    if (params.size() < 4)
    {
        output = "\nExpected more parameters: connect <address> <tcp_port> <udp_port>";
        return;
    }

    unsigned int tcpPort, udpPort;
    try {
        tcpPort = boost::lexical_cast<unsigned int>(params[2]);
        udpPort = boost::lexical_cast<unsigned int>(params[3]);
    }
    catch (boost::bad_lexical_cast)
    {
        output = "\nBad parameter types. Expected: connect <address> <tcp_port> <udp_port>";
        return;
    }

    NetConnection* newConnection = new NetConnection(*m_networkService);
    RemoteMessagePeer* myPeer = new RemoteMessagePeer(newConnection, false, *m_networkService);

    newConnection->connectTo(params[1].c_str(), tcpPort, udpPort);
    if (!m_networkThread.isRunning())
        m_networkThread.start();
    output = "\ncommand accepted.";
}

void GUIApplication::consoleNetServerCommand(const std::vector<String>& params, String& output)
{
    // 1. Parameter: sub-command
    if (!m_userCreated)
    {
        output = "\nThis command is only available after log-in";
        return;
    }

    if (params.size() < 2)
    {
        output = "\nExpected more parameters: netserver <command> [paramlist]*";
        return;
    }

    typedef std::map < String, std::function<void(const std::vector<String>&, String&)> > consoleFuncsMap;
    auto consoleHelpProcedure = [](const consoleFuncsMap& funcs, const std::vector<String>&, String& out){
        out = "\nKnown netserver commands include:";
        for (auto f : funcs)
        {
            out += " " + f.first;
        }
    };

    static const consoleFuncsMap consoleFuncs = consoleFuncsMap{
        { "start", std::bind(&GUIApplication::consoleStartNetServer, this, std::placeholders::_1, std::placeholders::_2) },
        { "help", std::bind(consoleHelpProcedure, std::cref(consoleFuncs), std::placeholders::_1, std::placeholders::_2) }
    };

    if (consoleFuncs.count(params[1]) > 0)
    {
        consoleFuncs.at(params[1])(params, output);
    }
    else
        output = "\nunknown command: " + params[1];
}


void GUIApplication::consoleStartNetServer(const std::vector<String>& params, String& output)
{
    // 1. Parameter: TCP port to listen
    if (params.size() < 3)
    {
        output = "\nExpected more parameters: netserver start <tcp_port>";
        return;
    }

    try {
        unsigned int portNumber = boost::lexical_cast<unsigned int>(params[2]);
        std::string errStr;
        if (!m_networkThread.createAndStartNetServer(portNumber, m_userListensUdpPort, errStr))
        {
            output = String("\nCannot start the server: ") + errStr;
            return;
        }
    }
    catch (boost::bad_lexical_cast)
    {
        output = "\nBad parameter types. Expected: netserver start <tcp_port>";
        return;
    }

    if (m_currentLevel)
        m_currentLevel->setServerSide(true);
    output = "\ncommand accepted.";
}

void GUIApplication::consoleGameServerCommand(const std::vector<String>& params, String& output)
{
    // 1. Parameter: sub-command
    if (!m_userCreated)
    {
        output = "\nThis command is only available after log-in";
        return;
    }

    if (params.size() < 2)
    {
        output = "\nExpected more parameters: gameserver <command> [paramlist]*";
        return;
    }

    typedef std::map < String, std::function<void(const std::vector<String>&, String&)> > consoleFuncsMap;
    auto consoleHelpProcedure = [](const consoleFuncsMap& funcs, const std::vector<String>&, String& out){
        out = "\nKnown gameserver commands include:";
        for (auto f : funcs)
        {
            out += " " + f.first;
        }
    };

    static const consoleFuncsMap consoleFuncs = consoleFuncsMap{
        { "start", std::bind(&GUIApplication::consoleStartGameServer, this, std::placeholders::_1, std::placeholders::_2) },
        { "help", std::bind(consoleHelpProcedure, std::cref(consoleFuncs), std::placeholders::_1, std::placeholders::_2) }
    };

    if (consoleFuncs.count(params[1]) > 0)
    {
        consoleFuncs.at(params[1])(params, output);
    }
    else
        output = "\nunknown command: " + params[1];
}

void GUIApplication::consoleStartGameServer(const std::vector<String>& params, String& output)
{

}
