/*!
 * \author Ezbeat, Ji Hoon Park
 */

#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <list>
#include <thread>

namespace EzPubSub
{

const uint32_t kSyncSpinCount = 2000;

const uint32_t kDefaultFlushTime = 1000; // 1 Second, Unit: Millisecond
const uint32_t kDefaultMaxBufferedDataSize = 10485760; // 10 MB, Unit: Byte

enum class Error : uint32_t
{
    kSuccess = 0x00000000,

    kUnsuccess = 0x80000000,
    kExistChannel,
    kExistSubscriber,
    kNotExistChannel,
    kNotExistSubscriber,
    kNotEnoughBufferSize,
    kBeStoppedFire
};

enum class FireStatus
{
    kRunning,
    kStop,
    kExit
};

typedef void(*SUBSCRIBER_CALLBACK)(_In_ const uint8_t* data, _In_ uint32_t dataSize, _In_opt_ void* userContext);

// External Data Process Pointer(optional), Fired subscriber callback list(optional), Published data
using PublishedData = std::tuple<void*, const std::vector<SUBSCRIBER_CALLBACK>, const std::vector<uint8_t>>;
struct ChannelInfo
{
    ChannelInfo()
    {
        flushTime = 0;
        maxBufferedDataSize = 0;
        fireStatus = FireStatus::kRunning;
        fireThread = nullptr;
        firedDataCount = 0;
        lostDataCount = 0;
        currentBufferedDataSize = 0;
    }

    //std::wstring name; // key of list
    uint32_t flushTime;
    uint32_t maxBufferedDataSize;
    FireStatus fireStatus;
    std::thread* fireThread;
    uint32_t firedDataCount;
    uint32_t lostDataCount;

    uint32_t currentBufferedDataSize;
    std::list<PublishedData> publishedDataList;

    std::list<SUBSCRIBER_CALLBACK> subscriberCallbackList;
};

class PubSubLite
{
public:
    // Channel Method
    static Error CreateChannel(_In_ const std::wstring& channelName, _In_opt_ uint32_t flushTime = kDefaultFlushTime, _In_opt_ uint32_t maxBufferedDataSize = kDefaultMaxBufferedDataSize);
    static Error UpdateChannel(_In_ const std::wstring& channelName, _In_ uint32_t flushTime, _In_ uint32_t maxDataSize);
    static Error DeleteChannel(_In_ const std::wstring& channelName);

    // Publisher Method
    static Error PublishData(
        _In_ const std::wstring& channelName,
        _In_ const uint8_t* data,
        _In_ uint32_t dataSize,
        _In_opt_ void* userContext = nullptr,
        _In_opt_ const std::vector<SUBSCRIBER_CALLBACK>* fireCallbackList = nullptr
    );

    // Subscriber Method
    static Error RegisterSubscriber(_In_ const std::wstring& channelName, _In_ const SUBSCRIBER_CALLBACK subscriberCallback);
    static Error UnregisterSubscriber(_In_ const std::wstring& channelName, _In_ const SUBSCRIBER_CALLBACK subscriberCallback);

    // Etc Method
    static Error Pause(_In_ const std::wstring& channelName);
    static Error Resume(_In_ const std::wstring& channelName, _In_opt_ bool clearBuffer = false);

    // Getter
    static Error GetFiredDataCount(_In_ const std::wstring& channelName, _Out_ uint32_t& firedDataCount);
    static Error GetLostDataCount(_In_ const std::wstring& channelName, _Out_ uint32_t& lostDataCount);

private:
    static std::unordered_map<std::wstring, ChannelInfo>::iterator SearchChannelInfo_(_In_ const std::wstring& channelName);
    static std::list<SUBSCRIBER_CALLBACK>::iterator SearchSubscriberCallback_(_In_ ChannelInfo& channelInfo, _In_ const SUBSCRIBER_CALLBACK subscriberCallback);
    static void FireThread_(ChannelInfo* channelInfo);
    static void AdjustDataBuffer_(_Inout_ ChannelInfo* channelInfo);

private:
    static ::CRITICAL_SECTION channelInfoListSync_;
    static std::unordered_map<std::wstring, ChannelInfo> channelInfoList_;
};

}