/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-19
 *
--*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <utility>
#include <string>
#include <list>
#include <vector>
#include <map>
#include "Configure.hpp"

std::string Configure::m_ConfigFile;
std::map<std::string, std::map<std::string, std::string> > Configure::m_ConfigMap;

int Configure::Load(const char* szFile)
{
	Configure::m_ConfigFile = std::string(szFile);

	int fd = open(Configure::m_ConfigFile.c_str(), O_RDONLY|O_EXCL);
	if(fd == -1)
		return -1;

	struct stat buf;
	if(fstat(fd, &buf) == -1)
	{
		close(fd);
		return -1;
	}

	char* lpConfigBuffer = new char[buf.st_size + 1];
	if(-1 == read(fd, lpConfigBuffer, buf.st_size))
	{
		delete [] lpConfigBuffer;
		close(fd);
		return -1;
	}
	lpConfigBuffer[buf.st_size] = 0;
	std::string sConfigContent = std::string(lpConfigBuffer);
	delete [] lpConfigBuffer;
	close(fd);

	boost::regex stBlockExpression("^\\[([_a-zA-Z0-9]+)\\]$");
	boost::regex stKeyValExpression("^([^=]+)=(.*)$");
	boost::regex stCommentExpression("#.*$");

	std::string sBlockKey;
	std::list<std::string> vLines;
	boost::algorithm::split(vLines, sConfigContent, boost::algorithm::is_any_of("\n"));
	BOOST_FOREACH(std::string sLine, vLines)
	{
		sLine = boost::regex_replace(sLine, stCommentExpression, "");
		boost::algorithm::trim(sLine);
		if(sLine.empty())
			continue;

		boost::smatch what;
		if(boost::regex_match(sLine, what, stBlockExpression))
		{
			sBlockKey = what[1];
			if(Configure::m_ConfigMap.find(sBlockKey) == Configure::m_ConfigMap.end())
				Configure::m_ConfigMap.insert(std::make_pair(sBlockKey, std::map<std::string, std::string>()));
		}
		else if(boost::regex_match(sLine, what, stKeyValExpression) && !sBlockKey.empty())
		{
			std::string sKey = what[1].str();
			std::string sVal = what[2].str();
			boost::algorithm::trim(sKey);
			boost::algorithm::trim(sVal);
			Configure::m_ConfigMap[sBlockKey][sKey] = sVal;
		}
		else
		{
		}
	}

	return 0;
}


