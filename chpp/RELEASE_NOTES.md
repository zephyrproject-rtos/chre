# CHPP Release Notes

A summary of notable changes is provided in the form of release notes. Dates are provided as yyyy-mm-dd. Note that this is not meant to be a detailed change log; for a detailed change list please refer to git commits.

### 2020-03-04 (4c668b3)

Initial release of CHPP.

- CHPP transport and app layers
- Loopback testing service

### 2020-07-28 (7cebe57)

This release enables service integration with WWAN / WiFi / GNSS devices based on the CHRE PAL API.

- New functionality

  - Reset and reset-ack implementation to allow either peer to initialize the other (e.g. upon boot)
  - Discovery service to provide list of services
  - Discovery client to match clients with discovered services
  - Standard WWAN service based on the CHRE PAL API
  - Standard WiFi service based on the CHRE PAL API
  - Standard GNSS service based on the CHRE PAL API
  - Standard WWAN client based on the CHRE PAL API

- Updates and bug fixes to existing layers, including

  - Better logging to assist verification and debugging
  - Error replies are sent over the wire for transport layer errors
  - Over-the-wire preamble has been corrected (byte shifting error)

- API and integration changes

  - App layer header now includes an error code
  - App layer message type now occupies only the least significant nibble (LSN). The most significant nibble (MSN) is reserved
  - chppPlatformLinkSend() now returns an error code instead of a boolean
  - Added initialization, deinitialization and reset functionality for the link layer (see link.h)
  - Condition variables functionality needs to be supported alongside other platform functionality (see chpp/platform/)
  - Name changes for the logging APIs

### 2020-08-07 (0b41306)

This release contains bug fixes as well as the loopback client.

- New functionality

  - Loopback client to run and verify a loopback test using a provided data buffer

- Cleanup and bug fixes

  - Corrected sequence number handling
  - Updated log messages
  - More accurate casting into enums

### 2020-08-27 (this)

This release contains additional clients, a virtual link layer for testing (e.g., using loopback), and several important bug fixes.

- New functionality

  - Basic implementation of the standard WiFi client based on the CHRE PAL API
  - Basic implementation of the standard GNSS client based on the CHRE PAL API
  - Virtual link layer that connects CHPP with itself on Linux to enable testing, including reset, discovery, and loopback

- Cleanup and bug fixes

  - Client implementation cleanup
  - Client-side handling of close responses
  - Reset / reset-ack handshaking mechanism fixed
  - Loopback client fixed
  - Enhanced log messages
  - Service command #s are now sequential

- API and integration changes

  - Platform-specific time functionality (platform_time.h)
