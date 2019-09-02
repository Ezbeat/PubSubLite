#include "src/PubSubLite.h"

int main(void)
{
    EzPubSub::PubSubLite::CreateChannel(L"TestProvider");
    EzPubSub::PubSubLite::UpdateChannel(L"TestProvider", EzPubSub::kDefaultFlushTime, EzPubSub::kDefaultMaxBufferSize);

	

    return 0;
}