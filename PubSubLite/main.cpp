#include "src/PubSubLite.h"

#include <iostream>

void SubscriberCallbackFirst(_In_ const uint8_t* data, _In_ uint32_t dataSize)
{
    //printf("[SUB 1] %p %d %s \n", data, dataSize, reinterpret_cast<const char*>(data));
}

void SubscriberCallbackSecond(_In_ const uint8_t* data, _In_ uint32_t dataSize)
{
    //printf("[SUB 2] %p %d %s \n", data, dataSize, reinterpret_cast<const char*>(data));
}

int main(void)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    std::string stringData = "ezbeattest_";
    std::string publishData;

    std::wstring channelName = L"TestProvider";
    EzPubSub::PubSubLite::CreateChannel(channelName);
    //EzPubSub::PubSubLite::UpdateChannel(channelName, EzPubSub::kDefaultFlushTime, EzPubSub::kDefaultMaxBufferedDataSize);

    EzPubSub::PubSubLite::RegisterSubscriber(channelName, SubscriberCallbackFirst);
    EzPubSub::PubSubLite::RegisterSubscriber(channelName, SubscriberCallbackSecond);

    for (uint32_t index = 0; index < 1000000; index++)
    {
        publishData.assign(stringData + std::to_string(index));
        EzPubSub::PubSubLite::PublishData(
            channelName, 
            reinterpret_cast<const uint8_t*>(publishData.c_str()), 
            static_cast<uint32_t>(publishData.size()) + 1
        );
    }

    uint32_t firedDataCount = 0;
    uint32_t lostDataCount = 0;
    EzPubSub::PubSubLite::GetFiredDataCount(channelName, firedDataCount);
    EzPubSub::PubSubLite::GetLostDataCount(channelName, lostDataCount);
    printf("fired/lost/sum data count: %d %d %d \n", firedDataCount, lostDataCount, firedDataCount + lostDataCount);

    //EzPubSub::PubSubLite::UnregisterSubscriber(channelName, SubscriberCallbackSecond);
    EzPubSub::PubSubLite::DeleteChannel(channelName);

    return 0;
}