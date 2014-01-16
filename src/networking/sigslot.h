/*
    A signal-slot implementation, based on C++11 standard library functions
    Created on: Nov 13, 2013
    Copyright (C) 2013 - 2014 Pavel Belskiy, github.com/Jagholin
*/
#pragma once
#include <functional>
#include <deque>
#include <memory>
#include <type_traits>
#include <osg/ref_ptr>
#include <osg/observer_ptr>

// !Debug
#include <iostream>

namespace addstd
{

template<typename, typename = void> class signal;

template<typename FuncSig>
class signal_base
{
protected:
    std::string m_name;
public:
    typedef std::function<FuncSig> t_funcHolder;
    typedef typename t_funcHolder::result_type t_result;

    struct t_slot
    {
        t_funcHolder funcPtr;

        t_slot(const t_funcHolder & f) :
            funcPtr(f) {}

        virtual bool isValid() = 0;
        virtual ~t_slot() {}
    };

    template <typename OwnerType>
    struct t_slotSharedPtr : public t_slot
    {
        std::weak_ptr<const OwnerType> funcOwner;

        t_slotSharedPtr(const t_funcHolder& f, const std::weak_ptr<const OwnerType>& o) :
            t_slot(f), funcOwner(o) {}

        bool isValid() { return ! funcOwner.expired(); }
    };

    template <typename OwnerType>
    struct t_slotRefPtr : public t_slot
    {
        osg::observer_ptr<const OwnerType> funcOwner;

        t_slotRefPtr(const t_funcHolder& f, const OwnerType* o) :
            t_slot(f), funcOwner(o) {}

        bool isValid() { return funcOwner.valid(); }
    };

    //template <typename OwnerSignalType>
    struct t_slotSignalledDelete : public t_slot, public std::enable_shared_from_this<t_slotSignalledDelete>
    {
        bool valid;

        t_slotSignalledDelete(const t_funcHolder& f) :
            t_slot(f), valid(true)
        {
        }

        void invalidate() { valid = false; }
        bool isValid() { return valid; }
    };

    struct t_slotUnmanaged : public t_slot
    {
        t_slotUnmanaged(const t_funcHolder& f) : t_slot(f) {}
        bool isValid() { return true; }
    };

    signal_base(std::string const& signalName): m_name(signalName) {}

    void connect(const t_funcHolder& f, osg::Referenced* owner)
    {
        m_slots.push_back(std::make_shared<t_slotRefPtr<osg::Referenced>>(f, owner));
    }

    template <typename OwnerType>
    void connect(const t_funcHolder& f, const std::shared_ptr<OwnerType> &p)
    {
        m_slots.push_back(std::make_shared<t_slotSharedPtr<OwnerType>>(f, p));
    }

    void connect(const t_funcHolder& f, signal<void()> &destructionSignal)
    {
        auto newSlot = std::make_shared<t_slotSignalledDelete>(f);
        m_slots.push_back(newSlot);
        destructionSignal.connect(std::bind(&t_slotSignalledDelete::invalidate, newSlot), newSlot);
    }

    void connect(const t_funcHolder& f, int dummy = -1)
    {
        m_slots.push_back(std::make_shared<t_slotUnmanaged>(f));
    }

protected:

    std::deque<std::shared_ptr<t_slot>> m_slots;
};

template<typename FuncSig, typename Enable>
class signal : public signal_base<FuncSig>
{
public:
    signal() : signal_base<FuncSig>(std::string()) {}
    explicit signal(std::string const& name) : signal_base<FuncSig>(name) {}

    template <typename... ArgT>
    t_result operator()(ArgT... params)
    {
        t_result res;
        std::deque<std::shared_ptr<t_slot>> newSlots;

        bool funcCalled = false;

        for (auto func : m_slots)
        {
            if (func->isValid())
            {
                res = func->funcPtr(params...);
                funcCalled = true;
                newSlots.push_back(func);
            }
        }

        if (!funcCalled && !m_name.empty())
        {
            std::cout << "slot called without receiver: " << m_name << "\n";
        }

        m_slots.swap(newSlots);
        return res;
    }
};

/// Template specialization:
// void return value
// is a special case.
// (see http://beta.boost.org/doc/libs/1_44_0/libs/utility/enable_if.html#htoc6 for the enable_if<> use case)
template<typename FuncSig>
class signal<FuncSig, typename std::enable_if<std::is_void<typename signal_base<FuncSig>::t_result>::value>::type> : public signal_base<FuncSig>
{
public:
    signal() : signal_base<FuncSig>(std::string()) {}
    explicit signal(std::string const& name) : signal_base<FuncSig>(name) {}

    template <typename... ArgT>
    void operator()(ArgT... params)
    {
        std::deque<std::shared_ptr<t_slot>> newSlots;
        bool funcCalled = false;

        for (auto func : m_slots)
        {
            if (func->isValid())
            {
                func->funcPtr(params...);
                funcCalled = true;
                newSlots.push_back(func);
            }
        }

        if (!funcCalled && !m_name.empty())
        {
            std::cout << "slot called without receiver: " << m_name << "\n";
        }

        m_slots.swap(newSlots);
    }
};

}
