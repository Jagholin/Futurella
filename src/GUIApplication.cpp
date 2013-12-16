
#include "GUIApplication.h"

#include <CEGUI/CEGUI.h>
#include <boost/lexical_cast.hpp>

#include "networking/networking.h"
#include "networking/peermanager.h"

using namespace CEGUI;

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
	m_servicePoint->run();
    m_servicePoint->reset();
}

bool AsioThread::createAndStartNetServer(unsigned int portNumber, std::string& errStr)
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
	bool res = m_serverObject->listen(portNumber, errStr);
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
		MultiLineEditbox* window = static_cast<MultiLineEditbox*>(gui.getRootWindow()->getChild("networkSettings/console"));

		if (window)
		{
			window->appendText(String("New connection added"));
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
		MultiLineEditbox* window = static_cast<MultiLineEditbox*>(gui.getRootWindow()->getChild("networkSettings/console"));

		if (window)
		{
			window->appendText(String("NetServer stopped"));
		}
	});
}

GUIApplication::GUIApplication(osgViewer::Viewer* osgApp)
{
	m_guiContext = nullptr;
	m_osgApp = osgApp;
	m_networkService = std::make_shared<boost::asio::io_service>();
	m_networkThread.setService(m_networkService);
	RemotePeersManager::getManager()->onNewPeerRegistration(std::bind(&GUIApplication::onNewFuturellaPeer, this, std::placeholders::_1), this);
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

		newConnection->connectTo(addrWindow->getText().c_str(), 1778);
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
		bool result = m_networkThread.createAndStartNetServer(port, err);
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
	peer->onActivation(m_renderThreadService->wrap([&](){
		MultiLineEditbox* mBox = static_cast<MultiLineEditbox*>(m_guiContext->getRootWindow()->getChild("networkSettings/console"));
		mBox->appendText("Peer activated");
	}), this);

	peer->onPeerDestruction(m_renderThreadService->wrap([&](){
		MultiLineEditbox* mBox = static_cast<MultiLineEditbox*>(m_guiContext->getRootWindow()->getChild("networkSettings/console"));
		mBox->appendText("Peer destroyed");
	}), this);

    peer->onError(m_renderThreadService->wrap([&](const std::string &str){
        MultiLineEditbox* mBox = static_cast<MultiLineEditbox*>(m_guiContext->getRootWindow()->getChild("networkSettings/console"));
        mBox->appendText(str);
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