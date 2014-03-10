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
#include <utility>
#include <vector>
#include <string>
#include <exception>

class IOBuffer :
	public boost::noncopyable
{
public:
	IOBuffer(char* buffer, size_t size);

	ssize_t Write(const char* buffer, size_t size);
	ssize_t Write(const char* buffer, size_t size, size_t pos) const;

	ssize_t Read(char* buffer, size_t size);
	ssize_t Read(char* buffer, size_t size, size_t pos) const;

	template<typename T>
	IOBuffer& operator >> (T& val);

	template<typename T>
	IOBuffer& operator << (T val);

	inline char* GetBuffer()
	{
		return m_Buffer;
	}

	inline size_t GetBufferSize()
	{
		return m_BufferSize;
	}

	inline size_t GetPosition()
	{
		return m_Position;
	}

protected:
	size_t m_Position;
	size_t m_BufferSize;
	char* m_Buffer;
};


#endif // define __IOBUFFER_HPP__
