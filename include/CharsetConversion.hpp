/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-01
 *
--*/
#ifndef __CHARSETCONVERSION_HPP__
#define __CHARSETCONVERSION_HPP__

#include <iconv.h>

#include <utility>
#include <string>

template<uint32_t From, uint32_t To>
class CharsetConversion
{
public:
    CharsetConversion() :
        m_szFromCode(CharsetConversion<From, To>::ENCODER[From]),
        m_szToCode(CharsetConversion<From, To>::ENCODER[To]),
        m_iconv((iconv_t)-1)
    {
        m_iconv = iconv_open(m_szToCode, m_szFromCode);
    }

    ~CharsetConversion()
    {
        if(m_iconv != (iconv_t)-1)
            iconv_close(m_iconv);
    }

    CharsetConversion<From, To>& operator << (const char* str)
    {
        size_t inbytesleft = strlen(str);
        char* inbuf = (char*)str;

        size_t bufferSize = (inbytesleft + 1) * 4;
        char* buffer = (char*)malloc(bufferSize);

        size_t outbytesleft = bufferSize;
        char* outbuf = buffer;

        while(inbytesleft > 0)
        {
            size_t sz = iconv(m_iconv, 
                              &inbuf, &inbytesleft,
                              &outbuf, &outbytesleft);
            if(sz == (size_t)-1)
            {
#if _LIBICONV_VERSION >= 0x0108
                if(errno != EILSEQ)
                    break;

                int discard = 1;
                iconvctl(m_iconv, ICONV_SET_DESCARD_ILSEQ, &discard);
#else
                ++inbuf;
                --inbytesleft;
#endif
            }
        }

        m_strText.append(buffer, bufferSize - outbytesleft);
        free(buffer);
        return *this;
    }

    inline CharsetConversion<From, To>& operator << (std::string str)
    {
        return (*this << str.c_str());
    }

    inline std::string& str()
    {
        return m_strText;
    }

    inline void clear()
    {
        m_strText.clear();
    }

    void ConvertTo(std::string from, std::string& to)
    {
        *this << from;
        to.append(str());
        clear();
    }

private:
    const char* m_szFromCode;
    const char* m_szToCode;
    iconv_t m_iconv;
    std::string m_strText;

    static const char* ENCODER[];
};

#define ENCODER_UTF8        0
#define ENCODER_UTF16       1
#define ENCODER_UTF32       2
#define ENCODER_GBK         3
#define ENCODER_GB18030     4
#define ENCODER_BIG5        5

template<uint32_t From, uint32_t To>
const char* CharsetConversion<From, To>::ENCODER[] = {
    "utf-8",
    "utf-16",
    "utf-32",
    "gbk",
    "gb18030",
    "big5"
};

typedef CharsetConversion<ENCODER_UTF8, ENCODER_GBK> UTF8toGBK;
typedef CharsetConversion<ENCODER_GBK, ENCODER_UTF8> GBKtoUTF8;

typedef CharsetConversion<ENCODER_UTF8, ENCODER_GB18030> UTF8toGB18030;
typedef CharsetConversion<ENCODER_GB18030, ENCODER_UTF8> GB18030toUTF8;

typedef CharsetConversion<ENCODER_GBK, ENCODER_BIG5> BIG5toGBK;
typedef CharsetConversion<ENCODER_BIG5, ENCODER_GBK> GBKtoBIG5;

#endif // define __CHARSETCONVERSION_HPP__
