/*!
 * \author Ezbeat, Ji Hoon Park
 */

#include "PubSubLite.h"

::CRITICAL_SECTION EzPubSub::PubSubLite::channelInfoListSync_;
std::unordered_map<std::wstring, EzPubSub::ChannelInfo> EzPubSub::PubSubLite::channelInfoList_;

EzPubSub::PubSubLite::PubSubLite()
{

}

EzPubSub::PubSubLite::~PubSubLite()
{

}

EzPubSub::Error EzPubSub::PubSubLite::CreateChannel(
    _In_ const std::wstring& channelName, 
    _In_opt_ uint32_t flushTime /*= kDefaultFlushTime*/, 
    _In_opt_ uint32_t maxBufferSize /*= kDefaultMaxBufferSize*/
)
{
    Error retValue = Error::kUnsuccess;

    ChannelInfo channelInfo;

    // Check Parameter
    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSection(&channelInfoListSync_);
    }

    if (ExistChannel_(channelName) != nullptr)
    {
        retValue = Error::kExistChannel;
        return retValue;
    }

    channelInfo.flushTime = flushTime;
    channelInfo.bufferSize = maxBufferSize;
    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoList_.insert({ channelName, channelInfo });
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::UpdateChannel(
    _In_ const std::wstring& channelName, 
    _In_ uint32_t flushTime,
    _In_ uint32_t maxBufferSize
)
{
    Error retValue = Error::kUnsuccess;

    ChannelInfo* channelInfo = nullptr;

    // Check Parameter
    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSection(&channelInfoListSync_);
    }

    channelInfo = ExistChannel_(channelName);
    if (channelInfo == nullptr)
    {
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfo->flushTime = flushTime;
    channelInfo->bufferSize = maxBufferSize;
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::ChannelInfo* EzPubSub::PubSubLite::ExistChannel_(
    _In_ const std::wstring& channelName
)
{
    ::EnterCriticalSection(&channelInfoListSync_);
    for (auto& channelInfoListEntry : channelInfoList_)
    {
        if (channelInfoListEntry.first == channelName)
        {
            ::LeaveCriticalSection(&channelInfoListSync_);
            return &(channelInfoListEntry.second);
        }
    }
    ::LeaveCriticalSection(&channelInfoListSync_);

    return nullptr;
}
