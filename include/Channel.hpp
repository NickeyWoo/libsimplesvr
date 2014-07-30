/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-01
 *
--*/
#ifndef __CHANNEL_HPP__
#define __CHANNEL_HPP__

template<typename ChannelDataT>
struct Channel {
	int				fd;
	sockaddr_in		address;
	ChannelDataT	data;

	Channel() :
		fd(-1)
	{
	}
};
template<>
struct Channel<void> {
	int 			fd;
	sockaddr_in		address;

	Channel() :
		fd(-1)
	{
	}
};

#endif // define __CHANNEL_HPP__
