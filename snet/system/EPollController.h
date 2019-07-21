#pragma once

#include <sys/epoll.h>

#include <unordered_map>
#include <memory>
#include <vector>
#include <cassert>

#include "IOChannel.h"

namespace SNet { namespace System {

class EPollController
{
public:
    EPollController(size_t epollEventsPoolSize = 16)
        :m_epollEvents(epollEventsPoolSize)
    {
        CreateEpollFD();
    }

    void RegisterChannel(const std::shared_ptr<IOChannel>& spChannel)
    {
        if (spChannel == nullptr)
        {
            return;
        }

        int fd = spChannel->GetFD();
        auto iterator = m_channelMap.find(fd);
        if (iterator != m_channelMap.end())
        {
            m_channelMap[fd].reset();
            m_channelMap.erase(iterator);
        }

        struct epoll_event channelEvent;
        std::memset(&channelEvent, 0, sizeof(epoll_event));
        channelEvent.data.ptr = spChannel.get();
        channelEvent.events = EPOLLIN | EPOLLET;

        if ((::epoll_ctl(m_epollFD, EPOLL_CTL_ADD, spChannel->GetFD(), &channelEvent) < 0))
        {
            return;
        }

        m_channelMap[spChannel->GetFD()] = std::move(spChannel);
    }

    void UnregisterChannel(const std::shared_ptr<IOChannel>& spChannel)
    {
        if (spChannel == nullptr)
        {
            return;
        }

        int fd = spChannel->GetFD();
        auto iterator = m_channelMap.find(fd);
        if (iterator != m_channelMap.end())
        {
            struct epoll_event channelEvent;
            std::memset(&channelEvent, 0, sizeof(epoll_event));
            channelEvent.data.ptr = spChannel.get();

            if ((::epoll_ctl(m_epollFD, EPOLL_CTL_DEL, spChannel->GetFD(), &channelEvent) < 0))
            {
                return;
            }

            m_channelMap[fd].reset();
            m_channelMap.erase(iterator);
        }
    }

    IOChannel* GetChannel(int fd)
    {
        auto iterator = m_channelMap.find(fd);
        if (iterator != m_channelMap.end())
        {
            return iterator->second.get();
        }

        return nullptr;
    }

    void NotifyChannelToSend(const std::shared_ptr<IOChannel>& spChannel)
    {
        struct epoll_event channelEvent;
        std::memset(&channelEvent, 0, sizeof(epoll_event));
        channelEvent.events = EPOLLOUT;
        channelEvent.data.ptr = spChannel.get();

        if ((::epoll_ctl(m_epollFD, EPOLL_CTL_MOD, spChannel->GetFD(), &channelEvent) < 0))
        {
            return;
        }
    }

    void Poll()
    {
        static const int32_t pollTimeOutMs = 10000;
        int32_t numOfEvents = ::epoll_wait(
            m_epollFD,
            &*m_epollEvents.begin(),
            static_cast<int>(m_epollEvents.size()),
            pollTimeOutMs
        );

        if (static_cast<size_t>(numOfEvents) > m_epollEvents.size())
        {
            std::cout << errno << std::endl;
            throw std::runtime_error("EPOLL failed.");
        }

        if (numOfEvents == 0)
        {
            return;
        }

        if (numOfEvents > 0)
        {
            for (int32_t i = 0; i < numOfEvents; ++i)
            {
                IOChannel* pChannel = static_cast<IOChannel*>(m_epollEvents[i].data.ptr);
                if (!pChannel)
                {
                    continue;
                }

                auto iterator = m_channelMap.find(pChannel->GetFD());
                if (iterator == m_channelMap.end())
                {
                    continue;
                }

                TriggerEvent(pChannel, m_epollEvents[i].events);
            }
        }
    }

private:
    static void TriggerEvent(IOChannel* pChannel, uint32_t eventsToNotify)
    {
        assert(pChannel != nullptr);

        if ((eventsToNotify & EPOLLHUP) && !(eventsToNotify & EPOLLIN))
        {
            pChannel->HandleEvent(IOChannel::CallbackType::OnClose);
        }

        if (eventsToNotify & EPOLLERR)
        {
            pChannel->HandleEvent(IOChannel::CallbackType::OnError);
        }
        if (eventsToNotify & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
        {
            pChannel->HandleEvent(IOChannel::CallbackType::OnReceive);
        }
        if (eventsToNotify & EPOLLOUT)
        {
            pChannel->HandleEvent(IOChannel::CallbackType::OnSend);
        }
    }

    void CreateEpollFD()
    {
        m_epollFD = ::epoll_create1(EPOLL_CLOEXEC);
    }

    int m_epollFD;

    std::unordered_map<int, std::shared_ptr<IOChannel>> m_channelMap;
    std::vector<epoll_event> m_epollEvents;
};

}}