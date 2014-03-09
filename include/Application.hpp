/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include <boost/noncopyable.hpp>
#include "EventScheduler.hpp"
#include "Configure.hpp"
#include "Server.hpp"
#include "UdpServer.hpp"
#include "TcpServer.hpp"

template<typename ApplicationImplT>
class Application :
	public boost::noncopyable
{
public:
	static ApplicationImplT& Instance()
	{
		static ApplicationImplT instance;
		return instance;
	}

	template<typename ServerT>
	bool Register(ServerT& svr, const char* szConfigName)
	{
		std::map<std::string, std::string> stServerInterface = Configure::Get(szConfigName);

		sockaddr_in addr;
		bzero(&addr, sizeof(sockaddr_in));
		addr.sin_family = PF_INET;
		addr.sin_port = htons(atoi(stServerInterface["port"].c_str()));
		addr.sin_addr.s_addr = inet_addr(stServerInterface["ip"].c_str());

		return (ServerT::Listen(svr, &m_Scheduler, addr) == 0);
	}

	std::string GetName()
	{
		char buffer[260];
		bzero(buffer, 260);

		ssize_t size = readlink("/proc/self/exe", buffer, 259);
		if(size == -1)
			return std::string("");

		std::string name = std::string(buffer);
		return name.substr(name.rfind('/') + 1);
	}

	std::string GetRuntimePath()
	{
		char buffer[260];
		bzero(buffer, 260);

		ssize_t size = readlink("/proc/self/exe", buffer, 259);
		if(size == -1)
			return std::string("");

		std::string path = std::string(buffer);
		return path.substr(0, path.rfind('/') + 1);
	}

	bool CreatePidFile(const char* szPidFile)
	{
		m_pid = open(szPidFile, O_RDWR|O_CREAT, 0640);
		if(m_pid == -1)
			return false;

		struct flock stLock;
		bzero(&stLock, sizeof(struct flock));
		stLock.l_type = F_WRLCK;

		if(fcntl(m_pid, F_SETLK, &stLock) == -1)
			return false;
		return true;
	}

	void Run()
	{
		m_Scheduler.Dispatch();
	}

protected:
	int	m_pid;
	EventScheduler m_Scheduler;
};

#define AppRun(app)																			\
	int main(int argc, char* argv[]) {														\
		app& instance = app::Instance();													\
																							\
		if(argc < 2) {																		\
			printf("usage: ./%s [config]\n", instance.GetName().c_str());					\
			return -1;																		\
		}																					\
																							\
		if(Configure::Load(argv[1]) < 0) {													\
			printf("error: load configure fail, %s\n", argv[1]);							\
			return -1;																		\
		}																					\
																							\
		std::map<std::string, std::string> stGlobalConfig = Configure::Get("global");		\
		if(strcmp(stGlobalConfig["daemon"].c_str(), "1") == 0)								\
			daemon(1, 1);																	\
																							\
		if(stGlobalConfig["pid"].empty()) {													\
			stGlobalConfig["pid"] = instance.GetRuntimePath();								\
			stGlobalConfig["pid"].append(instance.GetName());								\
			stGlobalConfig["pid"].append(".pid");											\
		}																					\
																							\
		if(!instance.CreatePidFile(stGlobalConfig["pid"].c_str())) {						\
			printf("error: %s is running.\n", instance.GetName().c_str());					\
			return -1;																		\
		}																					\
																							\
		if(instance.Initialize(argc, argv)) {												\
			printf("[%s] startup ...\n", instance.GetName().c_str());						\
			instance.Run();																	\
		}																					\
		return 0;																			\
	}



#endif // define __APPLICATION_HPP__
