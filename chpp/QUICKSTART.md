# CHPP Transport Layer Integration

This guide focuses on integrating and porting CHPP to your desired platform. For implementation details please refer to the included README.md instead.

The minimum typical steps are as follows:

## 1. Platform-specific functionality

Implement the platform-specific functionality utilized by CHPP. These can be found in chpp/platform and include:

1. Memory allocation and deallocation
2. Thread notification mechanisms and thread signalling
3. Mutexes (or other resource sharing mechanism)
4. Logging

Sample Linux implementations are provided.

## 2. Link-Layer APIs

The APIs that are needed to tie CHPP to the serial port are as follows:

1. bool chppRxDataCb(context, \*buf, len)
2. enum ChppLinkErrorCode chppPlatformLinkSend(*params, *buf, len)

In addition, the system must implement and initialize the platform-specific linkParams data structure as part of platform_link.h

## 3. Initialization

In order to initialize CHPP, it is necessary to

1. Allocate the transportContext and appContext structs that hold the state for each instance of the application and transport layers (in any order).
2. Call the layers’ initialization functions, chppTransportInit and chppAppInit (in any order)
3. Initialize the platform-specific linkParams struct (part of the transport struct).
4. Call chppWorkThreadStart to start the main thread for CHPP's Transport Layer.

## 4. Testing

Several unit tests are provided in transport_test.c. In addition, loopback functionality is already implemented in CHPP, and can be used for testing. For details on crafting a loopback datagram, look at README.md and the transport layer unit tests (transport_test.c).

## 5. Termination

In order to terminate CHPP's main transport layer thread, it is necessary to call chppWorkThreadStop. It is then possible to deallocate the transportContext and appContext structs.
