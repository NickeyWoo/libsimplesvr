/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-01
 *
--*/
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <string>
#include <exception>
#include "Channel.hpp"
#include "Log.hpp"

template<typename ChannelDataT>
class ServerInterface
{
public:
    inline void OnWriteable()
    {
        m_WriteableCallback(this);
    }

    inline void OnReadable()
    {
        m_ReadableCallback(this);
    }

    inline void OnError()
    {
        m_ErrorCallback(this);
    }
    
    boost::function<void(ServerInterface<ChannelDataT>*)> m_ReadableCallback;
    boost::function<void(ServerInterface<ChannelDataT>*)> m_WriteableCallback;
    boost::function<void(ServerInterface<ChannelDataT>*)> m_ErrorCallback;
    Channel<ChannelDataT> m_Channel;
};

template<typename ChannelDataT>
ServerInterface<ChannelDataT>* GetServerInterface(Channel<ChannelDataT>* pstChannel)
{
    char* pInterface = reinterpret_cast<char*>(pstChannel);
    ServerInterface<ChannelDataT>* pObj = NULL;
    size_t offset = reinterpret_cast<size_t>(&pObj->m_Channel);
    return reinterpret_cast<ServerInterface<ChannelDataT>*>(pInterface - offset);
}

int SetCloexecFd(int fd, int flags = FD_CLOEXEC);

#endif // define __SERVER_HPP__
