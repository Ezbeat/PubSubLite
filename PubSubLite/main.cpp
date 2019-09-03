#include "src/PubSubLite.h"

int main(void)
{
    std::wstring wstringData = L"123456789";
    std::string stringData = "ezbeattest";

    std::wstring channelName = L"TestProvider";
    EzPubSub::PubSubLite::CreateChannel(channelName);
    EzPubSub::PubSubLite::UpdateChannel(channelName, EzPubSub::kDefaultFlushTime, EzPubSub::kDefaultMaxBufferedDataSize);

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

    return 0;
}