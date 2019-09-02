/*!
 * \author Ezbeat, Ji Hoon Park
 */

#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <list>

namespace EzPubSub
{

const uint32_t kDefaultFlushTime = 1000; // 1 Second, Unit: Millisecond
const uint32_t kDefaultMaxBufferSize = 104857600; // 100 MB, Unit: Byte

enum class Error : uint32_t
{
    kSuccess,
    kExistChannel,
    kNotExistChannel,

    kUnsuccess
};

struct ChannelInfo
{
    ChannelInfo()
    {
        flushTime = 0;
        bufferSize = 0;
    }

    //std::wstring name; // key of list
    uint32_t flushTime;
    uint32_t bufferSize;
};

class PubSubLite
{
public:
    PubSubLite();
    ~PubSubLite();

    static Error CreateChannel(_In_ const std::wstring& channelName, _In_opt_ uint32_t flushTime = kDefaultFlushTime, _In_opt_ uint32_t maxBufferSize = kDefaultMaxBufferSize);
    static Error UpdateChannel(_In_ const std::wstring& channelName, _In_ uint32_t flushTime, _In_ uint32_t maxBufferSize);
    //static Error DeleteChannel();
    static Error Publish(_In_ std::wstring& channelName, _In_ const uint8_t* data, _In_ uint32_t dataSize);

private:
    static ChannelInfo* SearchChannel_(_In_ const std::wstring& channelName);

private:
    static ::CRITICAL_SECTION channelInfoListSync_;
    static std::unordered_map<std::wstring, ChannelInfo> channelInfoList_;
};

}