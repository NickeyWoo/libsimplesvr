/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-07-31
 *
--*/
#ifndef __CONSISTENTHASHRING_HPP__
#define __CONSISTENTHASHRING_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <openssl/md5.h>

#include <utility>
#include <string>
#include <vector>
#include <map>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include "IOBuffer.hpp"

template<typename KeyT>
struct MD5Hash
{
    uint64_t operator()(KeyT key)
    {
        MD5_CTX ctx;
        if(MD5_Init(&ctx) != 1)
            return 0;

        if(MD5_Update(&ctx, &key, sizeof(KeyT)) != 1)
            return 0;

        unsigned char buffer[16];
        if(MD5_Final(buffer, &ctx) != 1)
            return 0;

        return ntohll(*((uint64_t*)(buffer + 4)));
    }
};
template<>
struct MD5Hash<const char*>
{
    uint64_t operator()(const char* key)
    {
        MD5_CTX ctx;
        if(MD5_Init(&ctx) != 1)
            return 0;

        if(MD5_Update(&ctx, key, strlen(key)) != 1)
            return 0;

        unsigned char buffer[16];
        if(MD5_Final(buffer, &ctx) != 1)
            return 0;

        return ntohll(*((uint64_t*)(buffer + 4)));
    }
};
template<>
struct MD5Hash<std::string>
{
    uint64_t operator()(std::string key)
    {
        MD5_CTX ctx;
        if(MD5_Init(&ctx) != 1)
            return 0;

        if(MD5_Update(&ctx, key.c_str(), key.length()) != 1)
            return 0;

        unsigned char buffer[16];
        if(MD5_Final(buffer, &ctx) != 1)
            return 0;

        return ntohll(*((uint64_t*)(buffer + 4)));
    }
};

template<typename ValueT>
struct PointT
{
    ValueT Value;
};

template<typename KeyT, typename ValueT,
         size_t VirtualSize = 10, size_t AlignSize = 1024,
         template<typename> class HashT = MD5Hash >
class ConsistentHash
{
public:
    ValueT* Insert(KeyT key, uint32_t dwQuotas)
    {
        PointPtr pstPoint = new PointT<ValueT>();

        HashT<KeyT> h;
        uint64_t ddwPointKey = h(key);
        ddwPointKey = ddwPointKey / AlignSize * AlignSize;
        m_stHashRing.insert(std::make_pair(ddwPointKey, pstPoint));

        HashT<uint64_t> h2;
        for(size_t i=1; i<VirtualSize * dwQuotas; ++i)
        {
            ddwPointKey = h2(ddwPointKey);
            ddwPointKey = ddwPointKey / AlignSize * AlignSize;
            m_stHashRing.insert(std::make_pair(ddwPointKey, pstPoint));
        }

        return &pstPoint->Value;
    }

    void Delete(KeyT key)
    {
        HashT<KeyT> h;
        uint64_t ddwPointKey = h(key);
        ddwPointKey = ddwPointKey / AlignSize * AlignSize;

        typename std::map<uint64_t, PointPtr>::iterator iter = m_stHashRing.find(ddwPointKey);
        if(iter != m_stHashRing.end())
        {
            delete iter->second;
            m_stHashRing.erase(iter);
        }

        HashT<uint64_t> h2;
        for(size_t i=1; i<VirtualSize; ++i)
        {
            ddwPointKey = h2(ddwPointKey);
            ddwPointKey = ddwPointKey / AlignSize * AlignSize;
            m_stHashRing.erase(ddwPointKey);
        }
    }

    ValueT* Hash(KeyT key)
    {
        HashT<KeyT> h;
        uint64_t ddwPointKey = h(key);
        typename std::map<uint64_t, PointPtr>::iterator iter = m_stHashRing.lower_bound(ddwPointKey);
        if(iter != m_stHashRing.end())
            return &iter->second->Value;
        return NULL;
    }

    void Dump()
    {
        uint64_t ddwLastPointKey = 0;
        uint64_t ddwMaxCount = 0, ddwMinCount = 0xFFFFFFFFFFFFFFFF;
        uint64_t ddwAvgCount = 0;

        for(typename std::map<uint64_t, PointPtr>::iterator iter = m_stHashRing.begin();
            iter != m_stHashRing.end();
            ++iter)
        {
            uint64_t ddwCount = (iter->first - ddwLastPointKey);

            if(ddwCount > ddwMaxCount)
                ddwMaxCount = ddwCount;

            if(ddwCount < ddwMinCount)
                ddwMinCount = ddwCount;

            //printf("ddwPointKey: %lx, count(%lx)\n", iter->first, ddwCount);
            ddwLastPointKey = iter->first;
            ddwAvgCount += iter->first;
        }
        ddwAvgCount /= m_stHashRing.size();

        printf("max: %lx\n", ddwMaxCount);
        printf("min: %lx\n", ddwMinCount);
        printf("avg: %lx\n", ddwAvgCount);
    }
    
private:
    typedef PointT<ValueT>* PointPtr;

    std::map<uint64_t, PointPtr> m_stHashRing;
};



#endif // define __CONSISTENTHASHRING_HPP__
