#include "src/PubSubLite.h"

#include <iostream>

class ExtDataProcessor
{
public:
    ExtDataProcessor() {}
    ~ExtDataProcessor() {}

    void Print()
    {
        printf("ExtDataProcessor::Print \n");
    }

private:
};

void SubscriberCallbackFirst(_In_ const uint8_t* data, _In_ uint32_t dataSize, _In_opt_ void* extDataProcessor)
{
    printf("[SUB 1] %p %d %s \n", data, dataSize, reinterpret_cast<const char*>(data));
    if (extDataProcessor != nullptr)
    {
        static_cast<ExtDataProcessor*>(extDataProcessor)->Print();
    }
}

void SubscriberCallbackSecond(_In_ const uint8_t* data, _In_ uint32_t dataSize, _In_opt_ void* extDataProcessor)
{
    printf("[SUB 2] %p %d %s \n", data, dataSize, reinterpret_cast<const char*>(data));
    if (extDataProcessor != nullptr)
    {
        static_cast<ExtDataProcessor*>(extDataProcessor)->Print();
    }
}

int main(void)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    std::string stringData = "teststring_";
    std::string publishData;
    ExtDataProcessor extDataProcessor;
    std::vector<EzPubSub::SUBSCRIBER_CALLBACK> fireCallbackList = { SubscriberCallbackFirst, SubscriberCallbackSecond };

    std::wstring channelName = L"TestChannel";
    EzPubSub::PubSubLite::CreateChannel(channelName);
    //EzPubSub::PubSubLite::UpdateChannel(channelName, EzPubSub::kDefaultFlushTime, EzPubSub::kDefaultMaxBufferedDataSize);

    EzPubSub::PubSubLite::RegisterSubscriber(channelName, SubscriberCallbackFirst);
    EzPubSub::PubSubLite::RegisterSubscriber(channelName, SubscriberCallbackSecond);

    for (uint32_t index = 0; index < 100000; index++)
    {
        publishData.assign(stringData + std::to_string(index));
        EzPubSub::PubSubLite::PublishData(
            channelName,
            reinterpret_cast<const uint8_t*>(publishData.c_str()),
            static_cast<uint32_t>(publishData.size()) + 1,
            &extDataProcessor,
            &fireCallbackList
        );
    }

    Sleep(10000);

    uint32_t firedDataCount = 0;
    uint32_t lostDataCount = 0;
    EzPubSub::PubSubLite::GetFiredDataCount(channelName, firedDataCount);
    EzPubSub::PubSubLite::GetLostDataCount(channelName, lostDataCount);
    printf("fired/lost/sum data count: %d %d %d \n", firedDataCount, lostDataCount, firedDataCount + lostDataCount);

    //EzPubSub::PubSubLite::UnregisterSubscriber(channelName, SubscriberCallbackSecond);
    EzPubSub::PubSubLite::DeleteChannel(channelName);

    return 0;
}