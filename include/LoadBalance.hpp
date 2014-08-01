/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-07-31
 *
--*/
#ifndef __LOADBALANCE_HPP__
#define __LOADBALANCE_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <utility>
#include <string>
#include <vector>
#include <map>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

struct ServicePoint
{
	sockaddr_in stAddress;

	uint32_t dwSendCount;
	uint32_t dwRecvCount;

	uint32_t dwMaxQuotas;
	uint32_t dwCurrentQuotas;
	uint32_t dwRealQuotas;
};

class RoutePolicy
{
public:
	inline uint32_t ResetTime()
	{
		return 30;
	}

	inline void Reset(ServicePoint& point)
	{
		if(point.dwSendCount != 0)
		{
			double dLostRate = (double)(point.dwSendCount - point.dwRecvCount) / point.dwSendCount;
			if(dLostRate > 0.03)
				point.dwCurrentQuotas = 1;
			else if(dLostRate < 0.0005)
			{
				uint32_t quotas = point.dwCurrentQuotas + point.dwMaxQuotas * 0.2;
				point.dwCurrentQuotas = quotas > point.dwMaxQuotas ? point.dwMaxQuotas : quotas;
			}
			else
			{
				uint32_t quotas = point.dwCurrentQuotas + point.dwMaxQuotas * 0.05;
				point.dwCurrentQuotas = quotas > point.dwMaxQuotas ? point.dwMaxQuotas : quotas;
			}
		}
		point.dwRecvCount = 0;
		point.dwSendCount = 0;
	}
};

template<typename T>
class MemCompare :
	public std::binary_function<T, T, bool>
{
public:
	bool operator() (const T& v1, const T& v2)
	{
		return memcmp(&v1, &v2, sizeof(T)) > 0;
	}
};

template<typename PolicyT = RoutePolicy>
class LoadBalance
{
public:
	LoadBalance() :
		m_dwTotalQuotas(0)
	{
	}

	bool LoadConfigure(const char* szFile)
	{
		int fd = open(szFile, O_RDONLY);
		if(fd == -1)
			return false;

		struct stat info;
		bzero(&info, sizeof(struct stat));
		if(fstat(fd, &info) == -1)
		{
			close(fd);
			return false;
		}

		char* buffer = (char*)malloc(info.st_size);
		if(-1 == read(fd, buffer, info.st_size))
		{
			free(buffer);
			close(fd);
			return false;
		}
		std::string strContent = std::string(buffer, info.st_size);
		free(buffer);
		close(fd);

		m_dwTotalQuotas = 0;

		boost::regex stIPPortExpression("^([0-9\\.]+)[ \t]+([0-9]+)[ \t]+([0-9]+)$");
		boost::regex stCommentExpression("#.*$");

		std::list<std::string> vLines;
		boost::algorithm::split(vLines, strContent, boost::algorithm::is_any_of("\n"));
		BOOST_FOREACH(std::string sLine, vLines)
		{
			sLine = boost::regex_replace(sLine, stCommentExpression, "");
			boost::algorithm::trim(sLine);
			if(sLine.empty())
				continue;

			boost::smatch what;
			if(boost::regex_match(sLine, what, stIPPortExpression))
			{
				ServicePoint point;
				bzero(&point, sizeof(ServicePoint));
				point.stAddress.sin_family = PF_INET;
				point.stAddress.sin_addr.s_addr = inet_addr(what[1].str().c_str());
				point.stAddress.sin_port = htons(strtoul(what[2].str().c_str(), NULL, 10));

				point.dwMaxQuotas = strtoul(what[3].str().c_str(), NULL, 10);
				point.dwCurrentQuotas = point.dwMaxQuotas;
				point.dwRealQuotas = point.dwMaxQuotas;
				m_dwTotalQuotas += point.dwMaxQuotas;

				m_stServicesMap.insert(std::make_pair(point.stAddress, point));
			}
		}
		m_LastTimestamp = time(NULL);

		if(m_dwTotalQuotas == 0)
			return false;
		return true;
	}

	void Route(sockaddr_in* pstAddress)
	{
		if(!pstAddress) return;
		time_t now = time(NULL);

		// reset quotas
		bool bResetCurrentQuotas = (now - m_LastTimestamp > m_Policy.ResetTime());
		if(m_dwTotalQuotas == 0 || bResetCurrentQuotas)
		{
			m_dwTotalQuotas = 0;
			for(PointDictionary::iterator iter = m_stServicesMap.begin();
				iter != m_stServicesMap.end();
				++iter)
			{
				if(bResetCurrentQuotas)
					m_Policy.Reset(iter->second);

				iter->second.dwRealQuotas = iter->second.dwCurrentQuotas;
				m_dwTotalQuotas += iter->second.dwRealQuotas;
			}
			if(bResetCurrentQuotas)
				m_LastTimestamp = now;
		}

		srand(now);
		uint32_t dwSeed = round((double)m_dwTotalQuotas * rand() / RAND_MAX);

		for(PointDictionary::iterator iter = m_stServicesMap.begin();
			iter != m_stServicesMap.end();
			++iter)
		{
			if(dwSeed < iter->second.dwRealQuotas)
			{
				memcpy(pstAddress, &iter->second.stAddress, sizeof(sockaddr_in));

				++iter->second.dwSendCount;
				--iter->second.dwRealQuotas;
				--m_dwTotalQuotas;

				return;
			}
			dwSeed -= iter->second.dwRealQuotas;
		}

		memcpy(pstAddress, &m_stServicesMap.begin()->second.stAddress, sizeof(sockaddr_in));
	}

	void Failure(sockaddr_in* pstAddress)
	{
		if(!pstAddress) return;
		PointDictionary::iterator iter = m_stServicesMap.find(*pstAddress);
		if(iter == m_stServicesMap.end())
			return;

		m_dwTotalQuotas -= iter->second.dwRealQuotas;
		iter->second.dwRealQuotas = 0;
		iter->second.dwCurrentQuotas = 1;
	}

	void Success(sockaddr_in* pstAddress)
	{
		if(!pstAddress) return;
		PointDictionary::iterator iter = m_stServicesMap.find(*pstAddress);
		if(iter == m_stServicesMap.end())
			return;

		++iter->second.dwRecvCount;
	}

private:
	typedef std::map<sockaddr_in, ServicePoint, MemCompare<sockaddr_in> > PointDictionary;

	uint32_t m_dwTotalQuotas;
	time_t m_LastTimestamp;
	PolicyT m_Policy;
	PointDictionary m_stServicesMap;
};

#endif // define __LOADBALANCE_HPP__
