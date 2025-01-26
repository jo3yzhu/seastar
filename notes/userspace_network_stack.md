# TCP/IP stack

## Ethernet(Network Access Layer)
This layer includes Ethernet, Wi-Fi, etc. It provides communication and multiplexing for upper protocol. We disscus Ethernet for low latency scenario.
Ethernet dataframe consists of:
- MAC header
    - Destination MAC address
    - Source MAC address
    - EtherType, indicating the protocol type of payload(like IP or ARP)
- Payload: the wrapper of packet of upper protocol
- CRC: checksum
Ethernet client receives destination MAC address first and quickly check if the packet is sent to it then continues or stops receiving.
MTU(maximum transmission unit) is a common configuration for Ethernet, which determines threshold for splitting upper data payload. The packet exceeding MTU will be discarded on client side.
The prototol in Network Access Layer is responsible for communication among devices within one local network.

## IP()

## ARP()
