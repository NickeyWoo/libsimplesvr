/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-01
 *
--*/
#ifndef __CHANNEL_HPP__
#define __CHANNEL_HPP__

#include <boost/function.hpp>
#include "IOBuffer.hpp"

class InternalException :
    public std::exception
{
public:
    InternalException(const char* errorMessage);
    virtual ~InternalException() throw();
    virtual const char* what() const throw();

private:
    std::string m_ErrorMessage;
};

extern const char* safe_strerror(int error);

template<typename ChannelDataT>
struct Channel {
    int                 Socket;
    sockaddr_in         Address;

    ChannelDataT        Data;

    Channel() :
        Socket(-1)
    {
    }

    inline int GetErrorCode()
    {
        int error = 0;
        socklen_t len = sizeof(int);
        getsockopt(Socket, SOL_SOCKET, SO_ERROR, (void *)&error, &len);
        return error;
    }

    inline const char* GetError()
    {
        return safe_strerror(GetErrorCode());
    }
};
template<>
struct Channel<void> {
    int                 Socket;
    sockaddr_in         Address;

    Channel() :
        Socket(-1)
    {
    }

    inline int GetErrorCode()
    {
        int error = 0;
        socklen_t len = sizeof(int);
        getsockopt(Socket, SOL_SOCKET, SO_ERROR, (void *)&error, &len);
        return error;
    }

    inline const char* GetError()
    {
        return safe_strerror(GetErrorCode());
    }
};

template<typename ChannelDataT>
Channel<ChannelDataT>& operator >> (Channel<ChannelDataT>& channel, IOBuffer& io)
{
    msghdr msg;
    bzero(&msg, sizeof(msghdr));

    msg.msg_name = &channel.Address;
    msg.msg_namelen = sizeof(sockaddr_in);

    iovec iov;
    iov.iov_base = io.m_Buffer;
    iov.iov_len = io.m_BufferSize;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t recvSize = recvmsg(channel.Socket, &msg, 0);
    if(recvSize == -1)
        throw InternalException((boost::format("[%s:%d][error] recvmsg fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());

    io.m_AvailableReadSize = recvSize;
    io.m_ReadPosition = 0;
    return channel;
}

template<typename ChannelDataT>
inline Channel<ChannelDataT>& operator >> (Channel<ChannelDataT>& channel, IOBuffer* pIOBuffer)
{
    return channel >> *pIOBuffer;
}

template<typename ChannelDataT>
Channel<ChannelDataT>& operator << (Channel<ChannelDataT>& channel, IOBuffer& io)
{
    if(io.m_WritePosition == 0)
        return channel;

    msghdr msg;
    bzero(&msg, sizeof(msghdr));

    msg.msg_name = &channel.Address;
    msg.msg_namelen = sizeof(sockaddr_in);

    iovec iov;
    iov.iov_base = io.m_Buffer;
    iov.iov_len = io.m_WritePosition;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t sendSize = sendmsg(channel.Socket, &msg, 0);
    if(sendSize == -1)
        throw InternalException((boost::format("[%s:%d][error] sendmsg fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());

    io.m_WritePosition = 0;
    return channel;
}

template<typename ChannelDataT>
inline Channel<ChannelDataT>& operator << (Channel<ChannelDataT>& channel, IOBuffer* pIOBuffer)
{
    return channel << *pIOBuffer;
}

#endif // define __CHANNEL_HPP__
