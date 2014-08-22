/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
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
#include "Pool.hpp"
#include "Log.hpp"
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
		std::map<std::string, std::string> stServerInterface = Configure::Get(szConfigName);
		if(stServerInterface["port"].empty() || stServerInterface["ip"].empty())
			return false;

		sockaddr_in addr;
		bzero(&addr, sizeof(sockaddr_in));
		addr.sin_family = PF_INET;
		addr.sin_port = atoi(stServerInterface["port"].c_str());
		addr.sin_addr.s_addr = inet_addr(stServerInterface["ip"].c_str());

		static UdpServerStartup<ServerImplT, sockaddr_in> startup;
		startup.Register(addr);
		return true;
	}

	template<typename ServerImplT>
	bool RegisterTcpServer(ServerImplT& server, const char* szConfigName)
	{
		std::map<std::string, std::string> stServerInterface = Configure::Get(szConfigName);
		if(stServerInterface["port"].empty() || stServerInterface["ip"].empty())
			return false;

		sockaddr_in addr;
		bzero(&addr, sizeof(sockaddr_in));
		addr.sin_family = PF_INET;
		addr.sin_port = htons(atoi(stServerInterface["port"].c_str()));
		addr.sin_addr.s_addr = inet_addr(stServerInterface["ip"].c_str());

		if(server.Listen(addr) != 0)
			return false;

		static TcpServerStartup<ServerImplT, sockaddr_in> startup;
		startup.Register(&server, addr);
		return true;
	}

	template<typename ClientImplT>
	bool RegisterTcpClient(const char* szConfigName)
	{
		std::map<std::string, std::string> stClientInterface = Configure::Get(szConfigName);
		if(stClientInterface["port"].empty() || stClientInterface["ip"].empty())
			return false;

		sockaddr_in addr;
		bzero(&addr, sizeof(sockaddr_in));
		addr.sin_family = PF_INET;
		addr.sin_port = htons(atoi(stClientInterface["port"].c_str()));
		addr.sin_addr.s_addr = inet_addr(stClientInterface["ip"].c_str());

		static TcpClientStartup<ClientImplT, sockaddr_in> startup;
		startup.Register(addr);
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

		int fd = client.m_ServerInterface.m_Channel.Socket;

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
		std::map<std::string, std::string> stGlobalConfig = Configure::Get("global");
		m_LogStartup.Register(stGlobalConfig["logs"]);

		uint32_t concurrency = 1;
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
		void Register(StartupDataT data)
		{
			m_Data = data;

			if(Pool::Instance().IsStartup())
				OnStartup();
			else
				Pool::Instance().RegisterStartupCallback(boost::bind(&UdpServerStartup<ServerImplT, StartupDataT>::OnStartup, this));
		}

		bool OnStartup()
		{
			m_Data.sin_port = htons(m_Data.sin_port + Pool::Instance().GetID());
			if(PoolObject<ServerImplT>::Instance().Listen(m_Data) != 0)
				return false;

			EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
			return (scheduler.Register(&PoolObject<ServerImplT>::Instance(), EventScheduler::PollType::POLLIN) == 0);
		}
	
	private:
		StartupDataT m_Data;
	};

	template<typename ServerImplT, typename StartupDataT>
	class TcpServerStartup :
		public boost::noncopyable
	{
	public:
		void Register(ServerImplT* pServer, StartupDataT data)
		{
			m_Data = data;
            m_pServer = pServer;

			if(Pool::Instance().IsStartup())
				OnStartup();
			else
				Pool::Instance().RegisterStartupCallback(boost::bind(&TcpServerStartup<ServerImplT, StartupDataT>::OnStartup, this));
		}

		bool OnStartup()
		{
			EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
			return (scheduler.Register(m_pServer, EventScheduler::PollType::POLLIN) == 0);
		}
	
	private:
		StartupDataT m_Data;
        ServerImplT* m_pServer;
	};

	template<typename ClientImplT, typename StartupDataT>
	class TcpClientStartup :
		public boost::noncopyable
	{
	public:
		void Register(StartupDataT data)
		{
			m_Data = data;

			if(Pool::Instance().IsStartup())
				OnStartup();
			else
				Pool::Instance().RegisterStartupCallback(boost::bind(&TcpClientStartup<ClientImplT, StartupDataT>::OnStartup, this));
		}

		bool OnStartup()
		{
			return (PoolObject<ClientImplT>::Instance().Connect(m_Data) == 0);
		}
	
	private:
		StartupDataT m_Data;
	};

	class LogStartup
	{
	public:
		void Register(std::string path)
		{
			m_LogPath = path;
			Pool::Instance().RegisterStartupCallback(boost::bind(&LogStartup::OnStartup, this));
		}

		bool OnStartup()
		{
			return PoolObject<Log>::Instance().Initialize(m_LogPath);
		}

	private:
		std::string m_LogPath;
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
	LogStartup		m_LogStartup;
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
