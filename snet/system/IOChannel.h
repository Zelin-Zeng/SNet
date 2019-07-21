#pragma once

#include <functional>

namespace SNet { namespace System {

class IOChannel
{
public:
    IOChannel(int fd)
        : m_fd(fd)
    {
    }

    enum class CallbackType : uint8_t
    {
        OnSend = 0,
        OnReceive,
        OnClose,
        OnError
    };

    using Callback = std::function<void()>;

    void SetCallback(CallbackType type, Callback&& callback)
    {
        switch (type)
        {
        case CallbackType::OnSend:
            m_onSendCallback = std::move(callback);
            break;
        case CallbackType::OnError:
            m_onErrorCallback = std::move(callback);
            break;
        case CallbackType::OnReceive:
            m_onReceiveCallback = std::move(callback);
            break;
        case CallbackType::OnClose:
            m_onCloseCallback = std::move(callback);
            break;
        default:
            break;
        }
    }

    void HandleEvent(CallbackType type)
    {
        switch (type)
        {
        case CallbackType::OnSend:
            if (m_onSendCallback)
            {
                m_onSendCallback();
            }
            break;
        case CallbackType::OnError:
            if (m_onErrorCallback)
            {
                m_onErrorCallback();
            }
            break;
        case CallbackType::OnReceive:
            if (m_onReceiveCallback)
            {
                m_onReceiveCallback();
            }
            break;
        case CallbackType::OnClose:
            if (m_onCloseCallback)
            {
                m_onCloseCallback();
            }
            break;
        default:
            break;
        }
    }

    int GetFD() const
    {
        return m_fd;
    }

private:
    int m_fd;

    Callback m_onSendCallback;
    Callback m_onReceiveCallback;
    Callback m_onCloseCallback;
    Callback m_onErrorCallback;
};

}}
