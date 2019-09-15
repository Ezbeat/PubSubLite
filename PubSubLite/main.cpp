#include "src/PubSubLite.h"

#include <iostream>

void SubscriberCallbackFirst(_In_ const uint8_t* data, _In_ uint32_t dataSize)
{
    wprintf(L"[SUB 1] %p %d \n", data, dataSize);
}

void SubscriberCallbackSecond(_In_ const uint8_t* data, _In_ uint32_t dataSize)
{
    wprintf(L"[SUB 2] %p %d \n", data, dataSize);
}

int main(void)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    std::wstring wstringData = L"123456789";
    std::string stringData = "ezbeattest";

    std::wstring channelName = L"TestProvider";
    EzPubSub::PubSubLite::CreateChannel(channelName);
    //EzPubSub::PubSubLite::UpdateChannel(channelName, EzPubSub::kDefaultFlushTime, EzPubSub::kDefaultMaxBufferedDataSize);

    EzPubSub::PubSubLite::RegisterSubscriber(channelName, SubscriberCallbackFirst);
    EzPubSub::PubSubLite::RegisterSubscriber(channelName, SubscriberCallbackSecond);

    EzPubSub::PubSubLite::PublishData(
        channelName,
        reinterpret_cast<const uint8_t*>(wstringData.c_str()),
        static_cast<uint32_t>(wstringData.length() * sizeof(wchar_t))
    );
    EzPubSub::PubSubLite::PublishData(
        channelName,
        reinterpret_cast<const uint8_t*>(stringData.c_str()),
        static_cast<uint32_t>(stringData.length() * sizeof(char))
    );

    //EzPubSub::PubSubLite::UnregisterSubscriber(channelName, SubscriberCallbackSecond);
    EzPubSub::PubSubLite::DeleteChannel(channelName);

    return 0;
}