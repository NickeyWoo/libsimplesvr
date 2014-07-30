/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-07-31
 *
--*/
#ifndef __LOADBALANCE_HPP__
#define __LOADBALANCE_HPP__

#include <stdint.h>
#include <utility>
#include <string>

template<typename PolicyT>
class LoadBalance
{
public:
	static bool Load(LoadBalance<PolicyT>& lb, const char* szFile)
	{
		lb.m_strConfigureFile = std::string(szFile);

		return true;
	}

private:
	LoadBalance()
	{
	}

	std::string m_strConfigureFile;
};


#endif // define __LOADBALANCE_HPP__
