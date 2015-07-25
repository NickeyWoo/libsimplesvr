/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2015-05-06
 *
--*/
#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include <boost/unordered_map.hpp>
#include "Pool.hpp"
#include "PoolObject.hpp"
#include "Timer.hpp"

#ifndef SESSION_TIMEOUT
    #define SESSION_TIMEOUT 200
#endif

template<typename DataT, typename Token>
struct SessionInfo
{
    DataT stSessionData;
    Token dwSequence;
    typename Timer<SessionInfo<DataT, Token>*>::TimerID dwTimerId;
};

template<typename SessionDataT>
class Session
{
public:
    typedef uint32_t Token;
    typedef boost::function<void(SessionDataT* pData)> CallbackType;

    Session() :
        m_dwSessionTimeout(SESSION_TIMEOUT),
        m_dwSequence(0)
    {
    }

    SessionDataT* GetSessionData(Token dwToken)
    {
        typename SessionMap::iterator iter = m_stSessionMap.find(dwToken);
        if(iter == m_stSessionMap.end())
            return NULL;
        return &iter->second.stSessionData;
    }

    inline Token Allocate()
    {
        return Allocate(NULL);
    }

    Token Allocate(SessionDataT* pData)
    {
        ++m_dwSequence;

        SessionInfo<SessionDataT, Token> stInfo;
        stInfo.dwSequence = m_dwSequence;

        if(pData)
            stInfo.stSessionData = *pData;

        SessionInfo<SessionDataT, Token>* pInfo = &m_stSessionMap.insert(std::make_pair(m_dwSequence, stInfo)).first->second;
        pInfo->dwTimerId = PoolObject<SessionTimer>::Instance().SetTimeout(
                                boost::bind(&Session<SessionDataT>::OnSessionTimeout, this, _1),
                                m_dwSessionTimeout, pInfo);
        return m_dwSequence;
    }

    void Delete(Token dwToken)
    {
        typename SessionMap::iterator iter = m_stSessionMap.find(dwToken);
        if(iter == m_stSessionMap.end())
            return;

        PoolObject<SessionTimer>::Instance().Clear(iter->second.dwTimerId);
        m_stSessionMap.erase(dwToken);
    }

    inline void SetSessionTimeout(uint32_t dwTimeout)
    {
        m_dwSessionTimeout = dwTimeout;
    }

    inline uint32_t GetSessionTimeout()
    {
        return m_dwSessionTimeout;
    }

    template<typename T>
    inline void RegisterCallback(T* pObj)
    {
        RegisterCallback(boost::bind(&T::OnSessionTimeout, pObj, _1));
    }

    inline void RegisterCallback(boost::function<void(SessionDataT*)> callback)
    {
        m_stTimeoutCallback.push_back(callback);
    }

    void OnSessionTimeout(SessionInfo<SessionDataT, Token>* pInfo)
    {
        for(typename std::list<CallbackType>::iterator iter = m_stTimeoutCallback.begin();
            iter != m_stTimeoutCallback.end();
            ++iter)
        {
            (*iter)(&pInfo->stSessionData);
        }

        m_stSessionMap.erase(pInfo->dwSequence);
    }

private:
    typedef boost::unordered_map<Token, SessionInfo<SessionDataT, Token> > SessionMap;
    typedef Timer<SessionInfo<SessionDataT, Token>*> SessionTimer;

    uint32_t m_dwSessionTimeout;
    Token m_dwSequence;
    SessionMap m_stSessionMap;
    std::list<CallbackType> m_stTimeoutCallback;
};

#endif // __SESSION_HPP__-
