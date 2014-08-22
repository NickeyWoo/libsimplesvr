# Simple Server Library

Simple Server Library is NonBlock Socket Network Framework.

## UDP Server ##
```c++
    class discardd :
        public UdpServer<discardd>
    {
    public:
        void OnMessage(ChannelType& channel, IOBuffer& in)
        {
            // udp message
        }
    };
```

## TCP Server ##
```c++
    struct ServerStatus
    {
        int status;
    };

    class echod :
        public TcpServer<echod, ServerStatus>
    {
    public:
        void OnConnected(ChannelType& channel)
        {
            // a tcp client connected
        }

        void OnDisconnected(ChannelType& channel)
        {
            // a tcp client disconnected
        }

        void OnMessage(ChannelType& channel, IOBuffer& buffer)
        {
            // tcp message
        }
    };
```

## Application ##
```c++
class MyApp :
    public Application<MyApp>
{
public:
    bool Initialize(int argc, char* argv[])
    {
        if(!Register(m_echod, "echod_interface") ||
            !Register(m_discardd, "discardd_interface"))
        {
            printf("error: register server fail.\n");
            return false;
        }

        return true;
    }

private:
    echod m_echod;
    discardd m_discardd;
};

AppRun(MyApp);
```

[More examples...][1]

  [1]: https://github.com/NickeyWoo/libsimplesvr/tree/master/examples










