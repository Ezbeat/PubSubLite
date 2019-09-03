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
    _In_opt_ uint32_t maxBufferedDataSize /*= kDefaultMaxBufferedDataSize*/
)
{
    Error retValue = Error::kUnsuccess;

    ChannelInfo channelInfo;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSection(&channelInfoListSync_);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    if (SearchChannel_(channelName) != nullptr)
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kExistChannel;
        return retValue;
    }

    channelInfo.flushTime = flushTime;
    channelInfo.maxBufferedDataSize = maxBufferedDataSize;
    channelInfoList_.insert({ channelName, channelInfo });
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::UpdateChannel(
    _In_ const std::wstring& channelName, 
    _In_ uint32_t flushTime,
    _In_ uint32_t maxBufferedDataSize
)
{
    Error retValue = Error::kUnsuccess;

    ChannelInfo* channelInfo = nullptr;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSection(&channelInfoListSync_);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfo = SearchChannel_(channelName);
    if (channelInfo == nullptr)
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    channelInfo->flushTime = flushTime;
    channelInfo->maxBufferedDataSize = maxBufferedDataSize;
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::PublishData(
    _In_ std::wstring& channelName, 
    _In_ const uint8_t* data, 
    _In_ uint32_t dataSize
)
{
    Error retValue = Error::kUnsuccess;

    ChannelInfo* channelInfo = nullptr;
    uint32_t beDeletedDataSize = 0;

    if ((channelName.length() == 0) || (data == nullptr) || (dataSize == 0))
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSection(&channelInfoListSync_);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfo = SearchChannel_(channelName);
    if (channelInfo == nullptr)
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    if (channelInfo->currentBufferedDataSize + dataSize > channelInfo->maxBufferedDataSize)
    {
        beDeletedDataSize = 0;
        for (auto publishedDataListIter = channelInfo->publishedDataList.begin(); 
            publishedDataListIter != channelInfo->publishedDataList.end(); 
            publishedDataListIter++)
        {
            beDeletedDataSize += static_cast<uint32_t>(publishedDataListIter->size());
            if (beDeletedDataSize >= dataSize)
            {
                channelInfo->publishedDataList.erase(channelInfo->publishedDataList.begin(), ++publishedDataListIter);
                channelInfo->currentBufferedDataSize -= beDeletedDataSize;
                break;
            }
        }

        if (channelInfo->currentBufferedDataSize + dataSize > channelInfo->maxBufferedDataSize)
        {
            retValue = Error::kNotEnoughBufferSize;
            return retValue;
        }
    }

    channelInfo->publishedDataList.push_back({ data , data + dataSize });
    channelInfo->currentBufferedDataSize += dataSize;
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::ChannelInfo* EzPubSub::PubSubLite::SearchChannel_(
    _In_ const std::wstring& channelName
)
{
    /*
        If a channel is searched, a global structure pointer is returned, 
        so the caller using this method must synchronize.
    */

    for (auto& channelInfoListEntry : channelInfoList_)
    {
        if (channelInfoListEntry.first == channelName)
        {
            return &(channelInfoListEntry.second);
        }
    }

    return nullptr;
}
