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

template<typename T>
class ServerInterface;

template<typename ChannelDataT>
struct Channel {
	int				Socket;
	sockaddr_in		Address;
	ChannelDataT	Data;

	Channel() :
		Socket(-1)
	{
	}

	ServerInterface<ChannelDataT>* GetInterface()
	{
		char* pInterface = reinterpret_cast<char*>(this);
		return reinterpret_cast<ServerInterface<ChannelDataT>*>(pInterface - 3 * sizeof(boost::function<void(ServerInterface<ChannelDataT>*)>));
	}
};
template<>
struct Channel<void> {
	int 			Socket;
	sockaddr_in		Address;

	Channel() :
		Socket(-1)
	{
	}

	ServerInterface<void>* GetInterface()
	{
		char* pInterface = reinterpret_cast<char*>(this);
		return reinterpret_cast<ServerInterface<void>*>(pInterface - 3 * sizeof(boost::function<void(ServerInterface<void>*)>));
	}
};

#endif // define __CHANNEL_HPP__
