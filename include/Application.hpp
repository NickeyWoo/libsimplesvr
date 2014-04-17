/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include <signal.h>
#include <utility>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <sys/select.h>
#include "Configure.hpp"
#include "Log.hpp"
#include "Pool.hpp"
#include "Server.hpp"
#include "UdpServer.hpp"
#include "TcpServer.hpp"
#include "TcpClient.hpp"

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

	template<typename ServerImplT>
	bool RegisterUdpServer(const char* szConfigName)
	{
		UdpServerStartup<ServerImplT, std::string>& startup = PoolObject<UdpServerStartup<ServerImplT, std::string> >::Instance();
		startup.m_Data = std::string(szConfigName);
		startup.Register();
		return true;
	}

	template<typename ServerImplT>
	bool RegisterTcpServer(ServerImplT& server, const char* szConfigName)
	{
		std::map<std::string, std::string> stServerInterface = Configure::Get(szConfigName);

		sockaddr_in addr;
		bzero(&addr, sizeof(sockaddr_in));
		addr.sin_family = PF_INET;
		addr.sin_port = htons(atoi(stServerInterface["port"].c_str()));
		addr.sin_addr.s_addr = inet_addr(stServerInterface["ip"].c_str());

		if(server.Listen(addr) != 0)
			return false;

		return (Pool::Instance().Register(&server, EventScheduler::PollType::POLLIN) == 0);
	}

	template<typename ClientImplT>
	bool RegisterTcpClient(const char* szConfigName)
	{
		TcpClientStartup<ClientImplT, std::string>& startup = PoolObject<TcpClientStartup<ClientImplT, std::string> >::Instance();
		startup.m_Data = std::string(szConfigName);
		startup.Register();
		return true;
	}

	template<typename ClientImplT>
	bool ConnectToServer(ClientImplT& client, const char* szConfigName, int timeout = 0)
	{
		std::map<std::string, std::string> stClientInterface = Configure::Get(szConfigName);

		sockaddr_in addr;
		bzero(&addr, sizeof(sockaddr_in));
		addr.sin_family = PF_INET;
		addr.sin_port = htons(atoi(stClientInterface["port"].c_str()));
		addr.sin_addr.s_addr = inet_addr(stClientInterface["ip"].c_str());

		if(client.Connect(addr) != 0)
			return false;

		if(timeout == 0)
			return true;

		int fd = client.m_ServerInterface.m_Channel.fd;

		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(fd, &writefds);

		timeval tv;
		bzero(&tv, sizeof(timeval));
		tv.tv_usec = 1000 * timeout; // 100 millisec timeout

		int ready = select(fd + 1, NULL, &writefds, NULL, &tv);
		if(ready == -1 || !FD_ISSET(fd, &writefds))
			return false;
		return true;
	}

	void Run()
	{
		uint32_t concurrency = 1;
		std::map<std::string, std::string> stGlobalConfig = Configure::Get("global");
		if(!stGlobalConfig["concurrency"].empty())
			concurrency = strtoul(stGlobalConfig["concurrency"].c_str(), NULL, 10);

		printf("[%s] startup ...\n", GetName().c_str());
		Pool& pool = Pool::Instance();
		if(pool.Startup(concurrency) != 0)
			printf("[error] startup fail, %s.\n", safe_strerror(errno));
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
		m_lock = open(szPidFile, O_RDWR|O_CREAT, 0640);
		if(m_lock == -1)
			return false;

		struct flock stLock;
		bzero(&stLock, sizeof(struct flock));
		stLock.l_type = F_WRLCK;

		if(fcntl(m_lock, F_SETLK, &stLock) == -1)
			return false;
		return true;
	}

	template<typename ServerImplT, typename StartupDataT>
	class UdpServerStartup :
		public boost::noncopyable
	{
	public:
		void Register()
		{
			if(Pool::Instance().IsStartup())
				OnStartup();
			else
				Pool::Instance().RegisterStartupCallback(boost::bind(&UdpServerStartup<ServerImplT, StartupDataT>::OnStartup, this));
		}

		bool OnStartup()
		{
			std::map<std::string, std::string> stServerInterface = Configure::Get(m_Data);

			sockaddr_in addr;
			bzero(&addr, sizeof(sockaddr_in));
			addr.sin_family = PF_INET;
			addr.sin_port = htons(atoi(stServerInterface["port"].c_str()) + Pool::Instance().GetID());
			addr.sin_addr.s_addr = inet_addr(stServerInterface["ip"].c_str());

			ServerImplT& server = PoolObject<ServerImplT>::Instance();
			if(server.Listen(addr) != 0)
				return false;

			EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
			return (scheduler.Register(&server, EventScheduler::PollType::POLLIN) == 0);
		}
	
		StartupDataT m_Data;
	};

	template<typename ClientImplT, typename StartupDataT>
	class TcpClientStartup :
		public boost::noncopyable
	{
	public:
		void Register()
		{
			if(Pool::Instance().IsStartup())
				OnStartup();
			else
				Pool::Instance().RegisterStartupCallback(boost::bind(&TcpClientStartup<ClientImplT, StartupDataT>::OnStartup, this));
		}

		bool OnStartup()
		{
			std::map<std::string, std::string> stClientInterface = Configure::Get(m_Data);

			sockaddr_in addr;
			bzero(&addr, sizeof(sockaddr_in));
			addr.sin_family = PF_INET;
			addr.sin_port = htons(atoi(stClientInterface["port"].c_str()));
			addr.sin_addr.s_addr = inet_addr(stClientInterface["ip"].c_str());

			ClientImplT& server = PoolObject<ClientImplT>::Instance();
			if(server.Connect(addr) != 0)
				return false;

			EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
			return (scheduler.Register(&server, EventScheduler::PollType::POLLOUT) == 0);
		}
	
		StartupDataT m_Data;
	};

protected:
	Application() :
		m_pid(getpid()),
		m_lock(-1)
	{
		signal(SIGCHLD, SIG_IGN);
	}

	int				m_pid;
	int				m_lock;
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
			instance.Run();																	\
		}																					\
		return 0;																			\
	}


#endif // define __APPLICATION_HPP__
