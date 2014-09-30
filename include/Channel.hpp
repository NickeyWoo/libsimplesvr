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

extern const char* safe_strerror(int error);

template<typename ChannelDataT>
struct Channel {
    int             Socket;
    sockaddr_in     Address;
    ChannelDataT    Data;

    Channel() :
        Socket(-1)
    {
    }

    int GetErrorCode()
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
    int             Socket;
    sockaddr_in     Address;

    Channel() :
        Socket(-1)
    {
    }

    int GetErrorCode()
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

#endif // define __CHANNEL_HPP__
