#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h>
#include <endian.h>

#include <type_traits>
#include <cstring>
#include <iostream>

namespace SNet { namespace Network {

enum class SocketDomain: uint32_t
{
    ANY = 0,
    IPV4 = AF_INET,
    IPV6 = AF_INET6,
};

enum class SocketType : uint32_t
{
    ANY = 0,
    TCP = SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
    UDP = SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
    RAW = SOCK_RAW | SOCK_NONBLOCK | SOCK_CLOEXEC,
};

template<typename TEnum, typename=std::enable_if_t<std::is_enum<TEnum>::value>>
typename std::underlying_type_t<TEnum> to_underlying(TEnum enumValue)
{
    return static_cast<typename std::underlying_type_t<TEnum>>(enumValue);
}

class IPAddress final
{
public:
    IPAddress(uint16_t port = 8080, SocketDomain domain = SocketDomain::IPV4, bool loopBack = false)
    {
        if (domain == SocketDomain::IPV6)
        {
            std::memset(&m_address6, 0, sizeof(m_address6));
            m_address6.sin6_family = to_underlying(domain);

            in6_addr ip = loopBack ? in6addr_loopback : in6addr_any;
            m_address6.sin6_addr = ip;
            m_address6.sin6_port = htobe16(port);
        }
        else
        {
            std::memset(&m_address, 0, sizeof(m_address));
            m_address.sin_family = to_underlying(domain);

            in_addr_t ip = loopBack ? INADDR_LOOPBACK : INADDR_ANY;
            m_address.sin_addr.s_addr = htobe32(ip);
            m_address.sin_port = htobe16(port);
        }
    }

    IPAddress(const sockaddr_in6& address6)
    {
        m_address6 = address6;
    }

    const sockaddr* GetSockAddr() const
    {
        return (sockaddr*)(&m_address6);
    }

private:

    union
    {
        struct sockaddr_in m_address;
        struct sockaddr_in6 m_address6;
    };
};

class Socket final
{
public:
    Socket(SocketDomain domain = SocketDomain::IPV4, SocketType type = SocketType::TCP)
    {
        m_domain = domain;
        m_type = type;
        InitializeSocket();
    }

    Socket(int fd)
        : m_socketFD(fd)
    {
    }

    Socket(const Socket& otherSocket)
    {
        m_socketFD = otherSocket.m_socketFD;
        m_domain = otherSocket.m_domain;
        m_type = otherSocket.m_type;
    }

    void Bind(const IPAddress& ipAddress)
    {
        int32_t ret = ::bind(m_socketFD, ipAddress.GetSockAddr(),
            static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
        if (ret < 0)
        {
            std::cout << "Failed to bind socket." << std::endl;
        }
    }

    void Listen()
    {
        int ret = ::listen(m_socketFD, SOMAXCONN);
        if (ret < 0)
        {
            std::cout << "Failed to listen socket." << std::endl;
        }
    }

    std::string Read(const size_t readSize)
    {
        std::string buffer(readSize, '\0');

        const ssize_t sizeHasRead = ::read(m_socketFD, &buffer[0], readSize);

        if (sizeHasRead < 0)
        {
            throw std::runtime_error("Failed to read socket");
        }

        buffer.resize(sizeHasRead);
        return buffer;
    }

    void Write(const std::string& message)
    {
        const ssize_t sizeHasWrite = ::write(m_socketFD, &message[0], message.length());
        if (sizeHasWrite < 0)
        {
            throw std::runtime_error("Failed to write to socket");
        }
    }

    IPAddress GetPeerAddress()
    {
        struct sockaddr_in6 peeraddr;
        bzero(&peeraddr, sizeof peeraddr);
        socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
        if (::getpeername(m_socketFD, (sockaddr*)(&peeraddr), &addrlen) < 0)
        {
            std::cout << "Failed to get peer name." << std::endl;
        }
        IPAddress ipaddr(peeraddr);
        return ipaddr;
    }

    IPAddress GetLocalAddress()
    {
        struct sockaddr_in6 localaddr;
        bzero(&localaddr, sizeof localaddr);
        socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
        if (::getpeername(m_socketFD, (sockaddr*)(&localaddr), &addrlen) < 0)
        {
            std::cout << "Failed to get peer name." << std::endl;
        }
        IPAddress ipaddr(localaddr);
        return ipaddr;
    }

    std::pair<int, IPAddress> Accept()
    {
        struct sockaddr_in6 address;
        socklen_t addrlen = static_cast<socklen_t>(sizeof (address));
        std::memset(&address, 0, sizeof(address));

        int acceptFileDescription = ::accept4(m_socketFD,
            (sockaddr*)(&address),
            &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

        IPAddress ipAddress(address);
        return std::make_pair<int, IPAddress>(std::move(acceptFileDescription), std::move(ipAddress));
    }

    void SetReuseAddress(bool on)
    {
        int optval = on ? 1 : 0;
        ::setsockopt(m_socketFD, SOL_SOCKET, SO_REUSEADDR,
            &optval, static_cast<socklen_t>(sizeof optval));
    }

    void SetReusePort(bool on)
    {
        int optval = on ? 1 : 0;
        ::setsockopt(m_socketFD, SOL_SOCKET, SO_REUSEPORT,
            &optval, static_cast<socklen_t>(sizeof optval));
    }

    void SetKeepAlive(bool on)
    {
        int optval = on ? 1 : 0;
        ::setsockopt(m_socketFD, SOL_SOCKET, SO_KEEPALIVE,
            &optval, static_cast<socklen_t>(sizeof optval));
    }

    int GetFileDescription() const
    {
        return m_socketFD;
    }

    ~Socket()
    {
        ::close(m_socketFD);
    }

private:

    SocketDomain m_domain { SocketDomain::ANY };
    SocketType m_type { SocketType::ANY };

    void InitializeSocket()
    {
        m_socketFD = ::socket(to_underlying(m_domain), to_underlying(m_type), 0);

        int flags = 0;
        int ret = 0;

        // non-block
        flags = ::fcntl(m_socketFD, F_GETFL, 0);
        flags |= O_NONBLOCK;
        ret = ::fcntl(m_socketFD, F_SETFL, flags);

        // close-on-exec
        flags = ::fcntl(m_socketFD, F_GETFD, 0);
        flags |= FD_CLOEXEC;
        ret = ::fcntl(m_socketFD, F_SETFD, flags);
    }

    int m_socketFD;
};

}}