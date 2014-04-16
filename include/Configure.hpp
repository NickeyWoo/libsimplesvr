/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __CONFIGURE_HPP__
#define __CONFIGURE_HPP__

#include <utility>
#include <string>
#include <map>

class Configure
{
public:
	static int Load(const char* szFile);

	static inline std::map<std::string, std::string>& Get(std::string name)
	{
		return Configure::m_ConfigMap[name];
	}

	static inline std::map<std::string, std::string>& Get(const char* name)
	{
		return Configure::m_ConfigMap[name];
	}

	static inline int Reload()
	{
		return Configure::Load(Configure::m_ConfigFile.c_str());
	}

private:
	static std::string m_ConfigFile;
	static std::map<std::string, std::map<std::string, std::string> > m_ConfigMap;
};

#endif // define __CONFIGURE_HPP__
