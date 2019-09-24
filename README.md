# PubSubLite
PubSubLite is a data forwarder with a Publisher and Subscriber model within the same process.  
All methods in a class are static and can be called anywhere without a class declaration.  
So you have to pass the channel name as the first argument of every public method.  

Since all channel related data which are global data are synchronized, it works well in multi thread.  

# How to use
## Important method.
**1. Create a channel to connect publishers and subscribers and manage published data.**
```
static Error CreateChannel(
  _In_ const std::wstring& channelName, 
  _In_opt_ uint32_t flushTime = kDefaultFlushTime, 
  _In_opt_ uint32_t maxBufferedDataSize = kDefaultMaxBufferedDataSize
);
```
* flushTime(Milliseconds)  
The time interval for flushing data in the Channel's buffer to registered subscribers.  
* maxBufferedDataSize(byte)  
Total size of published data to buffer in channel. The allocated memory size can be larger because it represents the published data size.  

**2. Register a subscriber to receive published data on the channel.**
```
static Error RegisterSubscriber(
  _In_ const std::wstring& channelName, 
  _In_ const SUBSCRIBER_CALLBACK subscriberCallback
);
```
* subscriberCallback  
Callback function pointer to receive data.  
`typedef void(*SUBSCRIBER_CALLBACK)(_In_ const uint8_t* data, _In_ uint32_t dataSize, _In_opt_ void* userContext);`  
If published data is in the channel's buffer, the data is passed sequentially to the callback function at flush time.  

**3. Send data to publish to the created channel.**
```
static Error PublishData(
  _In_ const std::wstring& channelName, 
  _In_ const uint8_t* data, 
  _In_ uint32_t dataSize, 
  _In_opt_ void* userContext = nullptr,  
  _In_opt_ const std::vector<SUBSCRIBER_CALLBACK>* fireCallbackList = nullptr
);
```
As it is named Lite, we want to provide as simple a feature as possible.  
Therefore, the data to be published can only be transferred as a buffer pointer, and the data size in the buffer.  
The data passed is copied internally into "std::vector", and the allocated "std::vector" is added to the end of "std::list".

* userContext  
The value of this parameter is passed to the UserContext of the Subscriber Callback.

* fireCallbackList  
When passing published data to registered subscribers, this parameter selects the subscriber callback to be delivered.
If nullptr is passed, published data is delivered to all registered subscribers.

**4. Unregister Subscriber or Delete Channel.**
```
static Error UnregisterSubscriber(
  _In_ const std::wstring& channelName, 
  _In_ const SUBSCRIBER_CALLBACK subscriberCallback
);

static Error DeleteChannel(
  _In_ const std::wstring& channelName
);
```
If you delete a channel, registered subscribers are also removed automatically,  
so you do not need to call UnregisterSubscriber before calling DeleteChannel.  

※ If CreateChannel succeeds, a FireThread is created that sends data to the Subscriber, so if you do not call DeleteChannel, a memory leak occurs.  

## Etc method.
* **UpdateChannel**  
Change the flushTime and maxBufferedDataSize of already created channels.
* **PauseFire, ResumeFire**  
Pause or resume sending published data in the channel's buffer to the subscriber.
* **GetFiredDataCount, GetLostDataCount**  
Returns the number of data successfully sent to the subscriber and the number of data deleted from the buffer that could not be delivered to the subscriber.  
If the buffer is full when data is published, old data is deleted from the buffer, and the number of data deleted is lost data count.
