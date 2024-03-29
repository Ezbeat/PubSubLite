/*!
 * \author Ezbeat, Ji Hoon Park
 */

#include "PubSubLite.h"

::CRITICAL_SECTION EzPubSub::PubSubLite::channelInfoListSync_;
std::unordered_map<std::wstring, EzPubSub::ChannelInfo> EzPubSub::PubSubLite::channelInfoList_;

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
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    channelInfo.flushTime = flushTime;
    channelInfo.maxBufferedDataSize = maxBufferedDataSize;

    ::EnterCriticalSection(&channelInfoListSync_);
    if (SearchChannelInfo_(channelName) != channelInfoList_.end())
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kExistChannel;
        return retValue;
    }

    auto insertResult = channelInfoList_.insert({ channelName, channelInfo });
    insertResult.first->second.fireThread = new std::thread(FireThread_, &(insertResult.first->second));
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

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if ((channelInfoListIter == channelInfoList_.end()) ||
        (channelInfoListIter->second.fireStatus == FireStatus::kExit))
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    channelInfoListIter->second.flushTime = flushTime;
    channelInfoListIter->second.maxBufferedDataSize = maxBufferedDataSize;
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::DeleteChannel(
    _In_ const std::wstring& channelName
)
{
    Error retValue = Error::kUnsuccess;

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if (channelInfoListIter == channelInfoList_.end())
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    // Exit and Delete FireThread of Channel
    if (channelInfoListIter->second.fireThread != nullptr)
    {
        channelInfoListIter->second.fireStatus = FireStatus::kExit;
        ::LeaveCriticalSection(&channelInfoListSync_);
        channelInfoListIter->second.fireThread->join();
        ::EnterCriticalSection(&channelInfoListSync_);
        delete channelInfoListIter->second.fireThread;
        channelInfoListIter->second.fireThread = nullptr;
    }
    channelInfoList_.erase(channelInfoListIter);
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::PublishData(
    _In_ const std::wstring& channelName,
    _In_ const uint8_t* data,
    _In_ uint32_t dataSize,
    _In_opt_ void* userContext /*= nullptr*/,
    _In_opt_ const std::vector<SUBSCRIBER_CALLBACK>* fireCallbackList /*= nullptr*/
)
{
    Error retValue = Error::kUnsuccess;

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;
    const std::vector<SUBSCRIBER_CALLBACK> emptySubscriberList;

    if ((channelName.length() == 0) || (data == nullptr) || (dataSize == 0))
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if ((channelInfoListIter == channelInfoList_.end()) ||
        (channelInfoListIter->second.fireStatus == FireStatus::kExit))
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }
    else if (channelInfoListIter->second.fireStatus == FireStatus::kStop)
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kBeStoppedFire;
        return retValue;
    }

    channelInfoListIter->second.currentBufferedDataSize += dataSize;
    if (fireCallbackList != nullptr)
    {
        channelInfoListIter->second.publishedDataList.push_back({ userContext, *fireCallbackList, { data , data + dataSize } });
    }
    else
    {
        channelInfoListIter->second.publishedDataList.push_back({ userContext, emptySubscriberList, { data , data + dataSize } });
    }
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::RegisterSubscriber(
    _In_ const std::wstring& channelName,
    _In_ const SUBSCRIBER_CALLBACK subscriberCallback
)
{
    Error retValue = Error::kUnsuccess;

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if ((channelInfoListIter == channelInfoList_.end()) ||
        (channelInfoListIter->second.fireStatus == FireStatus::kExit))
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    if (SearchSubscriberCallback_(channelInfoListIter->second, subscriberCallback) !=
        channelInfoListIter->second.subscriberCallbackList.end())
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kExistSubscriber;
        return retValue;
    }

    channelInfoListIter->second.subscriberCallbackList.push_back(subscriberCallback);
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::UnregisterSubscriber(
    _In_ const std::wstring& channelName,
    _In_ const SUBSCRIBER_CALLBACK subscriberCallback
)
{
    Error retValue = Error::kUnsuccess;

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;
    std::list<SUBSCRIBER_CALLBACK>::iterator subscriberCallbackListIter;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if ((channelInfoListIter == channelInfoList_.end()) ||
        (channelInfoListIter->second.fireStatus == FireStatus::kExit))
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    subscriberCallbackListIter = SearchSubscriberCallback_(channelInfoListIter->second, subscriberCallback);
    if (subscriberCallbackListIter == channelInfoListIter->second.subscriberCallbackList.end())
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistSubscriber;
        return retValue;
    }

    channelInfoListIter->second.subscriberCallbackList.erase(subscriberCallbackListIter);
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::Pause(
    _In_ const std::wstring& channelName
)
{
    Error retValue = Error::kUnsuccess;

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if ((channelInfoListIter == channelInfoList_.end()) ||
        (channelInfoListIter->second.fireStatus == FireStatus::kExit))
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    channelInfoListIter->second.fireStatus = FireStatus::kStop;
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::Resume(
    _In_ const std::wstring& channelName,
    _In_opt_ bool clearBuffer /*= false*/
)
{
    Error retValue = Error::kUnsuccess;

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if ((channelInfoListIter == channelInfoList_.end()) ||
        (channelInfoListIter->second.fireStatus == FireStatus::kExit))
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    if ((clearBuffer == true) && (channelInfoListIter->second.publishedDataList.size() != 0))
    {
        channelInfoListIter->second.lostDataCount += static_cast<uint32_t>(channelInfoListIter->second.publishedDataList.size());
        channelInfoListIter->second.currentBufferedDataSize = 0;
        channelInfoListIter->second.publishedDataList.clear();
    }
    channelInfoListIter->second.fireStatus = FireStatus::kRunning;
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::GetFiredDataCount(
    _In_ const std::wstring& channelName,
    _Out_ uint32_t& firedDataCount
)
{
    Error retValue = Error::kUnsuccess;

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if ((channelInfoListIter == channelInfoList_.end()) ||
        (channelInfoListIter->second.fireStatus == FireStatus::kExit))
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    firedDataCount = channelInfoListIter->second.firedDataCount;
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

EzPubSub::Error EzPubSub::PubSubLite::GetLostDataCount(
    _In_ const std::wstring& channelName,
    _Out_ uint32_t& lostDataCount
)
{
    Error retValue = Error::kUnsuccess;

    std::unordered_map<std::wstring, ChannelInfo>::iterator channelInfoListIter;

    if (channelName.length() == 0)
    {
        return retValue;
    }

    if (channelInfoListSync_.LockCount == 0)
    {
        ::InitializeCriticalSectionAndSpinCount(&channelInfoListSync_, kSyncSpinCount);
    }

    ::EnterCriticalSection(&channelInfoListSync_);
    channelInfoListIter = SearchChannelInfo_(channelName);
    if ((channelInfoListIter == channelInfoList_.end()) ||
        (channelInfoListIter->second.fireStatus == FireStatus::kExit))
    {
        ::LeaveCriticalSection(&channelInfoListSync_);
        retValue = Error::kNotExistChannel;
        return retValue;
    }

    lostDataCount = channelInfoListIter->second.lostDataCount;
    ::LeaveCriticalSection(&channelInfoListSync_);

    retValue = Error::kSuccess;
    return retValue;
}

std::unordered_map<std::wstring, EzPubSub::ChannelInfo>::iterator EzPubSub::PubSubLite::SearchChannelInfo_(
    _In_ const std::wstring& channelName
)
{
    /*
        If a channel is searched, a global pointer is returned,
        so the caller using this method must synchronize.
    */

    return channelInfoList_.find(channelName);
}

std::list<EzPubSub::SUBSCRIBER_CALLBACK>::iterator EzPubSub::PubSubLite::SearchSubscriberCallback_(
    _In_ ChannelInfo& channelInfo,
    _In_ const SUBSCRIBER_CALLBACK subscriberCallback
)
{
    /*
        If a subscriberCallback is searched, a global pointer is returned,
        so the caller using this method must synchronize.
    */

    auto subscriberCallbackListIter = channelInfo.subscriberCallbackList.begin();

    for (; subscriberCallbackListIter != channelInfo.subscriberCallbackList.end(); subscriberCallbackListIter++)
    {
        if (*subscriberCallbackListIter == subscriberCallback)
        {
            return subscriberCallbackListIter;
        }
    }

    return subscriberCallbackListIter;
}

void EzPubSub::PubSubLite::FireThread_(
    ChannelInfo* channelInfo
)
{
    uint32_t copiedFlushTime = 0;
    std::list<SUBSCRIBER_CALLBACK> copiedCallbackList;

    while (true)
    {
        ::EnterCriticalSection(&channelInfoListSync_);
        // Terminate thread if fire is exit.
        if (channelInfo->fireStatus == FireStatus::kExit)
        {
            ::LeaveCriticalSection(&channelInfoListSync_);
            break;
        }

        // If no published data or fire status is stop, sleep by flush time.
        if ((channelInfo->publishedDataList.size() == 0) || (channelInfo->fireStatus == FireStatus::kStop))
        {
            copiedFlushTime = channelInfo->flushTime;
            ::LeaveCriticalSection(&channelInfoListSync_);
            Sleep(copiedFlushTime);
            continue;
        }
        else
        {
            AdjustDataBuffer_(channelInfo);
            if (channelInfo->publishedDataList.size() == 0)
            {
                copiedFlushTime = channelInfo->flushTime;
                ::LeaveCriticalSection(&channelInfoListSync_);
                Sleep(copiedFlushTime);
                continue;
            }
        }

        copiedCallbackList = channelInfo->subscriberCallbackList;
        ::LeaveCriticalSection(&channelInfoListSync_);

        // While the subscriber is processing published data, it does not synchronize to store the data sent by the publisher in the buffer.
        // Therefore, it never modifies the front of other methods published data buffer list.
        for (auto copiedCallbackListEntry : copiedCallbackList)
        {
            if (std::get<1>(*(channelInfo->publishedDataList.begin())).size() == 0)
            {
                copiedCallbackListEntry(
                    std::get<2>(*(channelInfo->publishedDataList.begin())).data(),
                    static_cast<uint32_t>(std::get<2>(*(channelInfo->publishedDataList.begin())).size()),
                    reinterpret_cast<void*>(std::get<0>(*(channelInfo->publishedDataList.begin())))
                );
            }
            else
            {
                // Not all callbacks fired published data
                for (auto fireCallbackListEntry : std::get<1>(*(channelInfo->publishedDataList.begin())))
                {
                    if (fireCallbackListEntry == copiedCallbackListEntry)
                    {
                        copiedCallbackListEntry(
                            std::get<2>(*(channelInfo->publishedDataList.begin())).data(),
                            static_cast<uint32_t>(std::get<2>(*(channelInfo->publishedDataList.begin())).size()),
                            reinterpret_cast<void*>(std::get<0>(*(channelInfo->publishedDataList.begin())))
                        );
                    }
                }
            }
        }

        ::EnterCriticalSection(&channelInfoListSync_);
        channelInfo->currentBufferedDataSize -= static_cast<uint32_t>(std::get<2>(*(channelInfo->publishedDataList.begin())).size());
        channelInfo->publishedDataList.pop_front();
        channelInfo->firedDataCount++;
        ::LeaveCriticalSection(&channelInfoListSync_);
    }
}

void EzPubSub::PubSubLite::AdjustDataBuffer_(
    _Inout_ ChannelInfo* channelInfo
)
{
    /*
        The caller using this method must synchronize.
    */

    size_t beforeDeletionDataListSize = 0;
    uint32_t beDeletedDataSize = 0;

    if (channelInfo->currentBufferedDataSize > channelInfo->maxBufferedDataSize)
    {
        beforeDeletionDataListSize = channelInfo->publishedDataList.size();
        beDeletedDataSize = 0;
        for (auto publishedDataListIter = channelInfo->publishedDataList.begin();
            publishedDataListIter != channelInfo->publishedDataList.end();
            publishedDataListIter++)
        {
            beDeletedDataSize += static_cast<uint32_t>(std::get<2>(*(channelInfo->publishedDataList.begin())).size());
            if ((channelInfo->currentBufferedDataSize - beDeletedDataSize) <= channelInfo->maxBufferedDataSize)
            {
                channelInfo->currentBufferedDataSize -= beDeletedDataSize;
                channelInfo->publishedDataList.erase(channelInfo->publishedDataList.begin(), ++publishedDataListIter);
                channelInfo->lostDataCount += static_cast<uint32_t>(beforeDeletionDataListSize - channelInfo->publishedDataList.size());
                return;
            }
        }
    }

    return;
}
