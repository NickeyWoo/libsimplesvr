/*++
 *
 * Simple Server Library
 * Author: NickWu
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
};
template<>
struct Channel<void> {
	int 			fd;
	sockaddr_in		address;
};

#endif // define __CHANNEL_HPP__
