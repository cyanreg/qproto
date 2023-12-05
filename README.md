# AVTransport
This repository hosts the AVTransport container/media transmission protocol, and the reference implementation code.

Currently, the protocol is a draft, and its syntax is subject to change.

You can read the current rendered draft by opening the [autogenerated webpage](https://cyanreg.github.io/avtransport/).

## Features

 - **64 bit presentation and duration timestamps** (465.66129 picosecond accuracy)
 - **Actual timebase** (allows accurate representation of rational framerates)
 - Sub-frame latency
 - Large FEC range, using RaptorQ
 - Only container to include mandatory error correction data on the headers themselves
 - Extra resilient headers (each FEC segment carries a piece of the main header)
 - Flexible keyframe indexing (indexes for seeking carry additional information)
 - Lossless cutting of compressed video (any frame dependencies can be made invisible)
 - Stream packet data compression
 - Uncompressed video and audio encapsulation
 - For H264 and HEVC, a real 64-bit dts timestamp.
 - ICC profile support
 - Color lookup table support (Adobe .cube)
 - Raw DNG support
 - Timestamp clock, allowing for jitter compensation
 - Unified, cycle-accurate clock for exact synchronization between multiple streams.

## Provisional information

### URI Scheme

The URI scheme shall be:
```
avt://[<transport>[:<mode>]@]<address>[:<port>][?<setting1>=<value1>][/<stream_id>[+<stream_id>][?<param1>=<val1]]
```

`<transport>` may be either missing (default: `udp`), `udp`, `udplite`, `quic`, or `file`.

`<mode>` is an optional setting which has different meanings for senders and receivers,
and transports. For senders, the default value, `active` means to start continuously sending
packets to the given address. `passive` means to wait for receivers to
send packets on the given address to begin transmitting back to them.

For receivers, the default value, `passive` means to listen on the given
address for any packets received. `active` for receivers means that receivers
will actively connect and try to request a stream from the target address.

`<address>` of the remote host, or local host, or multicast group

`<port>` on which to listen on/transmit to

`<stream_id>` of the stream(s) to present as default (overriding those signalled by the sender)

`<setting1>=<value1>` are a key=value pair of settings for the connection.

Alternatively, for UDP-only, the `udp://<address>:<port>` URI scheme can be used,
at the risk of conflict with other protocols (`MPEG-TS`).

### File extension
The `.avt` file extensions should be used.

## Links
You can talk about the project and get in touch with developers on:
 - IRC: **`#avtransport`** on the [Libera.Chat](ircs://irc.libera.chat:6697) network
  - [Libera.Chat’s guide on how to connect](https://libera.chat/guides/connect)
    - Or use the [Kiwi web client](https://kiwiirc.com/nextclient/irc.libera.chat/?#avtransport)

## License
The [specifications document)(draft-avtransport-spec.bs), and generated [HTML](https://cyanreg.github.io/avtransport/) are licensed under the **MIT-Zero** license.

The reference software (`libavtransport`) is licensed under the [**BSD 2-clause license**](LICENSE.md), in order to permit embedding, and free creation of alternative implementations.

## Feature comparison between protocols and containers

 - Streamable - whether a protocol can be reliably streamed
   * AVTransport: Yes. As both a protocol and a container, it can be direcly streamed over networks, pipes, packets, or serial links.
   * WebRTC: yes
   * RTMP: yes
   * MPEG-TS: yes
   * RIST: yes
   * SRT: yes
   * ISOBMFF/MP4: with large hacks. Requires chunking.
   * Matroska: no. With hacks, it could be chunked like ISOBMFF.
   * NUT: no
 - Latency
   * AVTransport: Minimal.
   * WebRTC: Medium. Without FEC, it's a single frame. With FEC, it's high.
   * RTMP: Low.
   * MPEG-TS: Minimal.
   * RIST: Generally high, due to FEC.
   * SRT: Generally high, due to FEC.
   * ISOBMFF/MP4: Huge. Even with the newest low latency extensions.
   * Matroska: Massive.
   * NUT: Medium.
 - Stream overhead - ratio between useful data and actually transmitted data
   * AVTransport: Low. Each packet has a 36 byte header.
   * WebRTC: Low to medium. Each packet has typically between 24 bytes and 76 bytes of headers.
   * RTMP: Low to medium. Each packet has typically 32 bytes of headers.
   * MPEG-TS: Very high. Each media packet is highly segmented due to 188-byte packet limit of MPEG-TS.
   * RIST: Very high, as it encapsulates MPEG-TS.
   * SRT: Very high, as it encapsulates MPEG-TS.
   * Matroska: Very low. It was designed to take the least amount of space possible.
   * Nut: Low. It was designed to not waste space.
 - Error resilience - resilience to receiving incorrect data, bitflips, and desyncs, without using FEC
   * AVTransport: Extremely high. Bitflips in headers are corrected (up to 40% of all bits may be flipped before errors appear). Resynchronization is extremely fast. Senders can send parity data for packets. Receivers can request retransmission of packets.
   * WebRTC: Low. Receivers may request retransmission of packets (which impacts latency).
   * RTMP: Very low.
   * MPEG-TS: Medium. Due to having very high overhead, there is a lot of redundant data. No native support for packet retransmissions.
   * RIST: Low, as it thinly encapsulates MPEG-TS. Relies on packet retransmissions.
   * SRT: Low, as it thinly encapsulates MPEG-TS. Relies on packet retransmissions.
   * Matroska: Very low.
   * ISOBMFF/MP4: Extremely low.
   * NUT: Low.
 - Error correction - forward error correction to reduce impact of packet drops, at the cost of latency
   * AVTransport: Very high. It has a large amount of redundancy, that increases as the bitrate increases. FEC is done via RaptorQ, the most efficient algorithm.
   * WebRTC: Medium, if enabled. The FEC algorithm it uses is simplistic. It requires bidirectional communication to adapt, during which corruption may appear.
   * RTMP: Non-existent.
   * MPEG-TS: Medium to high, due to its high overhead.
   * RIST: Medium to high.
   * SRT: Medium to high.
   * Matroska: Non-existent.
   * ISOBMFF/MP4: Non-existent.
   * NUT: Non-existent.
 - Clock recovery - whether drift compensation is possible, to ensure frames are not dropped at presentation. [Explanation](https://www.obe.tv/why-does-mpeg-ts-still-exist/), Clock Recovery section.
   * AVTransport: High.
   * WebRTC: None.
   * RTMP: None.
   * MPEG-TS: High.
   * RIST: High.
   * SRT: High.
   * Matroska: None.
   * ISOBMFF/MP4: None.
   * NUT: None.
 - Common clock between streams - whether each stream's clock can be synchronized, to ensure there is no drift between streams. [Explanation](https://www.obe.tv/why-does-mpeg-ts-still-exist/), Common Clock for Audio and Video section.
   * AVTransport: Yes.
   * WebRTC: No.
   * RTMP: Yes.
   * MPEG-TS: Yes.
   * RIST: Yes.
   * SRT: Yes.
   * Matroska: Yes.
   * ISOBMFF/MP4: Yes.
   * NUT: Yes.
 - Timestamp precision
   * AVTransport: 465.66129 picoseconds
   * WebRTC: 11 microseconds
   * RTMP: 1 millisecond
   * MPEG-TS: 11 microseconds
   * RIST: 11 microseconds
   * SRT: 11 microseconds
   * Matroska: 1 millisecond
   * ISOBMFF/MP4: Variable.
   * NUT: 1 nanosecond
 - Multicast - whether streams can be transmitted over Multicast
   * AVTransport: Yes.
   * WebRTC: No.
   * RTMP: No.
   * MPEG-TS: Yes.
   * RIST: No.
   * SRT: No.
   * ISOBMFF/MP4: No.
   * Matroska: No.
   * NUT: No.
 - File resilience to being truncated, missing chunks, seeking, and giving decoders invalid data
   * AVTransport: Extremely high. Impossible to mistake the start of a packet. Decoders will never be given wrong data.
   * WebRTC: Cannot be saved.
   * RTMP: Cannot be saved.
   * MPEG-TS: Extremely low to high, depending on the demuxer. Generally, extremely low.
   * RIST: Cannot be saved.
   * SRT: Cannot be saved.
   * ISOBMFF/MP4: Extremely low to impossibly low. If the MOV atom is missing, the entire file is mostly unsalvageable.
   * Matroska: Medium.
   * NUT: High.
 - Standard
   * AVTransport: Minimal. New codecs and packets can be added quickly by anyone. User data and custom codecs are supported.
   * WebRTC: Extremely rigid. Extended through the IETF, which requires many years of work to accept extensions.
   * RTMP: Extremely rigid. It took more than 10 years to support anything except H.264.
   * MPEG-TS: Somewhat rigid. Requests for new codec registrations may take a while. Unextendable, and somewhat incompatible, as all new codecs must fit in previously user-data.
   * RIST: No specifications. Closed source.
   * SRT: Public specifications. Must join an organization.
   * ISOBMFF/MP4: Low rigidity. Actively extended, though some features may take time to be stabilized.
   * Matroska: Low rigidity. It takes a while to add new features.
   * NUT: Low rigidity.
