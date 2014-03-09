/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __IOBUFFER_HPP__
#define __IOBUFFER_HPP__

#include <boost/noncopyable.hpp>

class IOBuffer :
	public boost::noncopyable
{
public:
	IOBuffer(char* buffer, size_t size);

protected:
	size_t m_BufferSize;
	char* m_Buffer;
};

#endif // define __IOBUFFER_HPP__
