#pragma once

#include <osg/NodeCallback>
#include <boost/asio/io_service.hpp>
// Class NodeCallbackService is a node callback, which runs work units dispatched
// by asio::io_service

class NodeCallbackService : public osg::NodeCallback
{
public:
    NodeCallbackService(boost::asio::io_service& aService);

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) override;

protected:
    boost::asio::io_service& m_service;
};
