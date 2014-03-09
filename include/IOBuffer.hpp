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

class IOBuffer :
	public boost::noncopyable
{
public:
	IOBuffer(char* buffer, size_t size);

	ssize_t Write(const char* buffer, size_t size);
	ssize_t Write(const char* buffer, size_t size, size_t pos);

	ssize_t Read(char* buffer, size_t size);
	ssize_t Read(char* buffer, size_t size, size_t pos);

	template<typename ProtoBufferT>
	IOBuffer& operator >> (ProtoBufferT& obj)
	{
		uint16_t len = 0;
		Read((char*)&len, sizeof(uint16_t));

		char* buffer = (char*)malloc(len);
		Read(buffer, len);

		obj.ParseFromArray((void*)buffer, len);

		free(buffer);
		return *this;
	}

	template<typename ProtoBufferT>
	IOBuffer& operator << (ProtoBufferT obj)
	{
		std::string str = obj.SerializeAsString();
		uint16_t len = str.length();
		Write((char*)&len, sizeof(uint16_t));
		Write(str.c_str(), len);
		return *this;
	}

protected:
	size_t m_Position;
	size_t m_BufferSize;
	char* m_Buffer;
};

#endif // define __IOBUFFER_HPP__
