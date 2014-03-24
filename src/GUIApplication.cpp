#include "GUIApplication.h"

#include <CEGUI/CEGUI.h>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "networking/networking.h"
#include "networking/peermanager.h"
#include "gameclient/GameInstanceClient.h"
#include "gameserver/GameInstanceServer.h"

#include "ShaderWrapper.h"

using namespace CEGUI;

// Some netmessages that inform about existing GameInstanceServers or request a connection
// between GameInstanceServer and GameInstanceClient

typedef GenericNetMessage<1400, std::string> NetGameServerAvailableMessage; // varNames: "serverName"
typedef GenericNetMessage<1401> NetGameConnectRequestMessage; // no vars
typedef GenericNetMessage<1402, std::string> NetGameConnectDeniedMessage; // varNames: "reason"

REGISTER_NETMESSAGE(GameServerAvailable)
REGISTER_NETMESSAGE(GameConnectRequest)
REGISTER_NETMESSAGE(GameConnectDenied)

template<> MessageMetaData
NetGameServerAvailableMessage::m_metaData = MessageMetaData::createMetaData<NetGameServerAvailableMessage>("serverName");

template<> MessageMetaData
NetGameConnectRequestMessage::m_metaData = MessageMetaData::createMetaData<NetGameConnectRequestMessage>("");

template<> MessageMetaData
NetGameConnectDeniedMessage::m_metaData = MessageMetaData::createMetaData<NetGameConnectDeniedMessage>("reason");

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
    catch (...)
    { }

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
        m_guiService->post([portNumber, portNumberUDP](){
            GUIContext& gui = System::getSingleton().getDefaultGUIContext();
            Window* window = gui.getRootWindow()->getChild("console/output");

            if (window)
            {
                window->appendText("\nNetServer listens at tcp port = " + boost::lexical_cast<std::string>(portNumber)
                    +", and udp port = " + boost::lexical_cast<std::string>(portNumberUDP));
            }
        });
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

GUIApplication::GUIApplication(osgViewer::Viewer* osgApp, osg::Group* rootGroup)
{
    m_guiContext = nullptr;
    m_osgApp = osgApp;
    m_networkService = std::make_shared<boost::asio::io_service>();
    m_networkThread.setService(m_networkService);
    //m_currentLevel = nullptr;
    m_rootGroup = rootGroup;
    m_userCreated = false;
    m_gameClient = nullptr;
    m_gameServer = nullptr;
    m_shaderProvider = std::make_shared<ShaderProvider>();
    ShaderWrapper::setDefaultShaderProvider(m_shaderProvider.get());
    RemotePeersManager::getManager()->onNewPeerRegistration(std::bind(&GUIApplication::onNewFuturellaPeer, this, std::placeholders::_1), this);
}

/*void GUIApplication::setCurrentLevel(Level* levelData)
{
    m_currentLevel = levelData;
}*/

void GUIApplication::registerEvents()
{
    m_guiContext = &(System::getSingleton().getDefaultGUIContext());

    addEventHandler("chatWindow/quitBtn", PushButton::EventClicked, Event::Subscriber(&GUIApplication::onQuitBtnClicked, this));
    addEventHandler("networkSettings", FrameWindow::EventCloseClicked, Event::Subscriber(&GUIApplication::onWindowCloseClicked, this));
    addEventHandler("chatWindow", FrameWindow::EventCloseClicked, Event::Subscriber(&GUIApplication::onWindowCloseClicked, this));
    addEventHandler("chatWindow/sendBtn", PushButton::EventClicked, Event::Subscriber(&GUIApplication::onSendBtnClicked, this));
    addEventHandler("chatWindow/input", Editbox::EventTextAccepted, Event::Subscriber(&GUIApplication::onSendBtnClicked, this));

    //addEventHandler("networkSettings/connectBtn", PushButton::EventClicked, Event::Subscriber(&GUIApplication::onConnectBtnClicked, this));
    //addEventHandler("networkSettings/listenBtn", PushButton::EventClicked, Event::Subscriber(&GUIApplication::onListenBtnClicked, this));

    addEventHandler("console/input", Window::EventMouseClick, Event::Subscriber(&GUIApplication::onConsoleClicked, this));
    addEventHandler("console/output", Window::EventMouseClick, Event::Subscriber(&GUIApplication::onConsoleClicked, this));
    addEventHandler("console/input", Editbox::EventTextAccepted, Event::Subscriber(&GUIApplication::onConsoleInput, this));

    addEventHandler("StartMenu/CreateSrvButton", Window::EventMouseClick, Event::Subscriber(&GUIApplication::onCreateServersAndJoinClicked, this));
    addEventHandler("StartMenu/JoinSrvButton", Window::EventMouseClick, Event::Subscriber(&GUIApplication::onJoinClicked, this));
    addEventHandler("StartMenu/EndButton", Window::EventMouseClick, Event::Subscriber(&GUIApplication::onQuitBtnClicked, this));
    m_shaderProvider->registerEvents(this);
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

bool GUIApplication::onWindowCloseClicked(const EventArgs& args)
{
    const WindowEventArgs* realArgs = static_cast<const WindowEventArgs*>(&args);
    realArgs->window->setVisible(false);
    return true;
}

void GUIApplication::onNewFuturellaPeer(const RemoteMessagePeer::pointer& peer)
{
    peer->onActivation([&, peer](){
        if (m_gameServer)
        {
            NetGameServerAvailableMessage::pointer msg{ new NetGameServerAvailableMessage };
            std::get<0>(msg->m_values) = m_gameServer->name();
            peer->send(msg);
        }
        //m_currentLevel->connectLocallyTo(peer);
        std::cout << "peer activated\n";
    }, this);

    peer->onActivation(m_renderThreadService->wrap([&](){
        Window* mBox = m_guiContext->getRootWindow()->getChild("console/output");
        mBox->appendText("\nPeer activated");
    }), this);

    peer->onPeerDestruction(m_renderThreadService->wrap([this, peer](){
        Window* mBox = m_guiContext->getRootWindow()->getChild("console/output");
        mBox->appendText("\nPeer destroyed");

        // Remove all available servers from the peer
        auto serverToRemove = std::find_if(m_availableGameServers.begin(), m_availableGameServers.end(),
            [peer](const std::tuple<std::string, MessagePeer*> item){
                return peer == std::get<1>(item);
        });

        if (serverToRemove != m_availableGameServers.end())
            m_availableGameServers.erase(serverToRemove);
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
        m_renderThreadService->post([this, msg](){
            Window* chatWindow = m_guiContext->getRootWindow()->getChild("chatWindow/output");
            NetChatMessage::const_pointer realMsg{msg->as<NetChatMessage>()};
            chatWindow->appendText(std::get<0>(realMsg->m_values));
        });
    }
    else if (msg->gettype() == NetGameServerAvailableMessage::type)
    {
        m_renderThreadService->post([this, msg, sender](){
            Window* console = m_guiContext->getRootWindow()->getChild("console/output");
            NetGameServerAvailableMessage::const_pointer realMsg{ msg->as<NetGameServerAvailableMessage>() };
            m_availableGameServers.push_back(std::make_tuple(std::get<0>(realMsg->m_values), sender));
            console->appendText(String("\nNew Game server available: \\[") + boost::lexical_cast<std::string>(m_availableGameServers.size() - 1) + "]: " + std::get<0>(realMsg->m_values));
        });
    }
    else if (msg->gettype() == NetGameConnectRequestMessage::type)
    {
        m_renderThreadService->post([this, msg, sender](){
            if (!m_gameServer)
            {
                NetGameConnectDeniedMessage::pointer response{ new NetGameConnectDeniedMessage };
                std::get<0>(response->m_values) = "The server you requested to be connected with, doesn't exist";
                sender->send(response);
                return;
            }
            m_gameServer->connectLocallyTo(sender);

            Window* console = m_guiContext->getRootWindow()->getChild("console/output");
            console->appendText("\nOne remote user connected to the game server");
        });
    }
    else if (msg->gettype() == NetGameConnectDeniedMessage::type)
    {
        m_renderThreadService->post([this, sender](){
            if (m_gameClient){
                // connection refused
                m_gameClient->disconnectLocallyFrom(sender);
            }
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
    String consoleCommand = inputBox->getText();
    inputBox->setText("");

    doConsoleCommand(consoleCommand);

    return true;
}

void GUIApplication::doConsoleCommand(const String& command)
{
    Window* outputBox = m_guiContext->getRootWindow()->getChild("console/output");

    String consoleCommand = command;
    std::vector<String> rawParts, consoleParts;
    boost::trim(consoleCommand);
    boost::split(rawParts, consoleCommand, boost::is_space());
    for (auto s : rawParts)
    {
        boost::trim(s);
        if (s.size() > 0)
            consoleParts.push_back(s);
    }
    if (consoleParts.empty())
        return;

    typedef std::map < String, std::function<void(const std::vector<String>&, String&)> > consoleFuncsMap;
    auto consoleHelpProcedure = [](const consoleFuncsMap& funcs, const std::vector<String>&, String& out){
        out = "\nKnown console commands include:";
        for (auto f : funcs)
        {
            out += " " + f.first;
        }
    };

    static const consoleFuncsMap consoleFuncs{
        { "login", std::bind(&GUIApplication::consoleCreateUser, this, std::placeholders::_1, std::placeholders::_2) },
        { "show_network", std::bind(&GUIApplication::consoleShowNetwork, this, std::placeholders::_1, std::placeholders::_2) },
        { "clear", std::bind(&GUIApplication::consoleClear, this, std::placeholders::_1, std::placeholders::_2) },
        { "netserver", std::bind(&GUIApplication::consoleNetServerCommand, this, std::placeholders::_1, std::placeholders::_2) },
        { "net", std::bind(&GUIApplication::consoleNetServerCommand, this, std::placeholders::_1, std::placeholders::_2) },
        { "gameserver", std::bind(&GUIApplication::consoleGameServerCommand, this, std::placeholders::_1, std::placeholders::_2) },
        { "game", std::bind(&GUIApplication::consoleGameServerCommand, this, std::placeholders::_1, std::placeholders::_2) },
        { "connect", std::bind(&GUIApplication::consoleConnect, this, std::placeholders::_1, std::placeholders::_2) },
        { "help", std::bind(consoleHelpProcedure, std::cref(consoleFuncs), std::placeholders::_1, std::placeholders::_2) },
        { "macro", std::bind(&GUIApplication::consoleMacroCommand, this, std::placeholders::_1, std::placeholders::_2) },
        { "shaders", std::bind(&GUIApplication::consoleOpenShaderEditor, this, std::placeholders::_1, std::placeholders::_2) }
    };

    // Append the command first, in different color
    outputBox->appendText("\n[colour='ff00ff00']>> " + consoleCommand + "[colour='ffffffff']");

    if (consoleFuncs.count(consoleParts[0]) > 0)
    {
        String outputStr;
        consoleFuncs.at(consoleParts[0])(consoleParts, outputStr);
        outputBox->appendText(outputStr);
    }
    else
        outputBox->appendText(String("\nunknown command: ") + consoleParts[0]);
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
    RemotePeersManager::getManager()->setMyUdpPort(m_userListensUdpPort);
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
    if (params.size() < 3)
    {
        output = "\nExpected more parameters: connect <address> <tcp_port>";
        return;
    }

    unsigned int tcpPort = 0;
    try {
        tcpPort = boost::lexical_cast<unsigned int>(params[2]);
    }
    catch (boost::bad_lexical_cast)
    {
        output = "\nBad parameter types. Expected: connect <address> <tcp_port>";
        return;
    }

    NetConnection* newConnection = new NetConnection(*m_networkService);
    RemoteMessagePeer* myPeer = new RemoteMessagePeer(newConnection, false, *m_networkService);

    newConnection->connectTo(params[1].c_str(), tcpPort);
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

    static const consoleFuncsMap consoleFuncs {
        { "start", std::bind(&GUIApplication::consoleStartNetServer, this, std::placeholders::_1, std::placeholders::_2) },
        { "listen", std::bind(&GUIApplication::consoleStartNetServer, this, std::placeholders::_1, std::placeholders::_2) },
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

    //if (m_currentLevel)
    //    m_currentLevel->setServerSide(true);
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

    static const consoleFuncsMap consoleFuncs {
        { "start", std::bind(&GUIApplication::consoleStartGameServer, this, std::placeholders::_1, std::placeholders::_2) },
        { "create", std::bind(&GUIApplication::consoleStartGameServer, this, std::placeholders::_1, std::placeholders::_2) },
        { "connect", std::bind(&GUIApplication::consoleConnectGameServer, this, std::placeholders::_1, std::placeholders::_2) },
        { "join", std::bind(&GUIApplication::consoleConnectGameServer, this, std::placeholders::_1, std::placeholders::_2) },
        { "list", std::bind(&GUIApplication::consoleListGameServers, this, std::placeholders::_1, std::placeholders::_2) },
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
    if (m_gameServer)
    {
        output = "\nGame server appears to be running already";
        return;
    }
    if (params.size() < 3)
    {
        output = "\nExpected more parameters: gameserver start <server_name>";
        return;
    }

    m_gameServer = new GameInstanceServer(params[2].c_str());
    NetGameServerAvailableMessage::pointer msg{ new NetGameServerAvailableMessage };
    std::get<0>(msg->m_values) = params[2].c_str();
    RemotePeersManager::getManager()->broadcast(msg);

    m_availableGameServers.push_back(std::make_tuple(params[2].c_str(), m_gameServer));

    output = "\ngame server created successfully";
}

void GUIApplication::consoleListGameServers(const std::vector<String>& params, String& output)
{
    output = "\nList of available servers:";
    unsigned int a = 0;
    for (auto serverListEntry : m_availableGameServers)
    {
        output += String("\n\\[") + boost::lexical_cast<std::string>(a) + "]: " + std::get<0>(serverListEntry);
    }
}

void GUIApplication::consoleConnectGameServer(const std::vector<String>& params, String& output)
{
    if (m_gameClient)
    {
        output = "\nGame client appears to be running already";
        return;
    }
    if (params.size() < 3)
    {
        output = "\nExpected more parameters: gameserver connect <ServerNumber>";
        return;
    }

    unsigned int serverNumber = 0;
    try {
        serverNumber = boost::lexical_cast<unsigned int>(params[2]);
    }
    catch (boost::bad_lexical_cast)
    {
        output = "\nBad parameter types. Expected: gameserver connect <ServerNumber>";
        return;
    }

    if (serverNumber >= m_availableGameServers.size())
    {
        output = "\nThe server with this number doesn't exist. Type in 'gameserver list' for a list.";
        return;
    }

    m_gameClient = new GameInstanceClient(m_rootGroup, m_osgApp, this);
    m_gameClient->onClientOrphaned(m_renderThreadService->wrap([this](){
        // No server is connected to the game client, so we may just drop it
        delete m_gameClient;
        m_gameClient = nullptr;
        Window* consoleOut = m_guiContext->getRootWindow()->getChild("console/output");
        consoleOut->appendText("\nGame client lost connection, and was automatically deleted.");
    }), this);

    MessagePeer* gameServer = std::get<1>(m_availableGameServers[serverNumber]);
    m_gameClient->connectLocallyTo(gameServer);

    NetGameConnectRequestMessage::pointer msg{ new NetGameConnectRequestMessage };
    gameServer->send(msg);
    output = "\n Game client created";
}

void GUIApplication::timeTick(float dt)
{
    if (m_gameServer)
        m_gameServer->physicsTick(dt);
}

void GUIApplication::consoleMacroCommand(const std::vector<String>& params, String& output)
{
    if (params.size() < 2)
    {
        output = "Usage: macro <filename>";
        return;
    }
    std::string macroFileName = params[1].c_str() + std::string(".txt");
    std::ifstream macroFile(macroFileName);
    std::string commandLine;

    while (!macroFile.fail() && !macroFile.eof())
    {
        std::getline(macroFile, commandLine);
        doConsoleCommand(String(commandLine));
    }
}

void GUIApplication::hudLostFocus()
{
    // Hide all windows in Root
    Window *targets[] = {
        m_guiContext->getRootWindow()->getChild("console"),
        m_guiContext->getRootWindow()->getChild("chatWindow"),
        m_guiContext->getRootWindow()->getChild("networkSettings"),
        m_guiContext->getRootWindow()->getChild("shaderEditor")
    };

    AnimationManager& animManager = AnimationManager::getSingleton();
    m_animHideTargets.clear();
    for (Window* w : targets)
    {
        if (! w->isVisible())
            continue;
        m_animHideTargets.push_back(w);
        AnimationInstance* myAnimInstance = animManager.instantiateAnimation("AreaAnimationHide");
        myAnimInstance->setTargetWindow(w);
        myAnimInstance->start();
    }
}

void GUIApplication::hudGotFocus()
{
    AnimationManager& animManager = AnimationManager::getSingleton();
    for (Window* w : m_animHideTargets)
    {
        if (!w->isVisible())
            continue;
        AnimationInstance* myAnimInstance = animManager.instantiateAnimation("AreaAnimationShow");
        myAnimInstance->setTargetWindow(w);
        myAnimInstance->start();
    }
    m_animHideTargets.clear();

    m_guiContext->getRootWindow()->getChild("console")->show();
    Editbox* inputBox = static_cast<Editbox*>(m_guiContext->getRootWindow()->getChild("console/input"));
    inputBox->activate();
}

void GUIApplication::consoleOpenShaderEditor(const std::vector<String>& params, String& output)
{
    Window* target = m_guiContext->getRootWindow()->getChild("shaderEditor");
    target->show();
    target->activate();
}

std::shared_ptr<PhysicsEngine> GUIApplication::getOrCreatePhysicsEngine()
{
    std::shared_ptr<PhysicsEngine> result;
    if (m_physicsEngine.expired())
    {
        result = std::make_shared<PhysicsEngine>();
        m_physicsEngine = result;
    }
    else
    {
        result = m_physicsEngine.lock();
    }
    return result;
}

void GUIApplication::showGoalCursorAt(float x, float y)
{
    m_renderThreadService->dispatch([this, x, y](){
        Window* goal = m_guiContext->getRootWindow()->getChild("levelGoal");
        goal->setPosition(UVector2(UDim(x, -16), UDim(y, -16)));
        goal->show();
    });
}

void GUIApplication::hideGoalCursor()
{
    m_renderThreadService->dispatch([this](){
        Window* goal = m_guiContext->getRootWindow()->getChild("levelGoal");
        goal->hide();
    });
}

bool GUIApplication::onCreateServersAndJoinClicked(const CEGUI::EventArgs&)
{
    m_guiContext->getRootWindow()->getChild("StartMenu")->hide();
    m_guiContext->getRootWindow()->getChild("CreateServer")->show();
    return true;
}

bool GUIApplication::onJoinClicked(const CEGUI::EventArgs&)
{
    m_guiContext->getRootWindow()->getChild("StartMenu")->hide();
    m_guiContext->getRootWindow()->getChild("JoinServer")->show();
    return true;
}
