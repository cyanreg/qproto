# AVTransport

This repository hosts the AVTransport container/media transmission protocol, and the reference implementation code.

Currently, the protocol is a draft, and its syntax is subject to change.

You can read the current rendered specifications by opening the [autogenerated webpage](https://cyanreg.github.io/avtransport/).

## Features

 - **64 bit presentation and duration timestamps**
   * Maximum accuracy of 465.66129 picosecond!
   * Cycle-level synchronization of clocks, up to 4.3 Ghz
   * Support of multiple clocks, each mapped to multiple streams
   * Jitter compensation, avoiding clock drift
   * Support for determining latency, based on UNIX epoch times
 - **Actual timebase** (allows accurate representation of rational framerates)
   * Timestamp jitter is eliminated
   * Lossless representation of common non-integer framerates
 - Sub-frame latency
   * Each packet carries an exact range and offset
 - Concatenation of files is supported
 - 3D stereo support
 - Multicast support
 - Native QUIC support
 - Extensive metadata
   * Per-stream metadata
   * Everything allowed, from strings, integers, to binary data
 - Full multi-channel support
   * Implemented via the IAMF standard
   * Object-based audio ready
 - Extremely flexible FEC, using RaptorQ codes
   * From single packets being protected, to to entire groups of packets from multiple streams
   * Allows mixing of FEC streams and unprotected streams
 - Only container to include mandatory error correction data on the headers themselves
   * Completely avoids decoder corruption caused by muxer errors
   * Full recovery of corrupt data
   * Suitable long term, archival storage
 - Resilient headers
   * Protects against packet loss, and allows salvaging of missing packets without any FEC
   * Each segment of a packet carries a piece of the first packet's header
 - Checksums
   * Optional checksums of packet/metadata/user data via xxHash.
 - Flexible keyframe indexing
   * Allows information about keyframes to be spaced in any way
   * Includes all possible information, timestamps, byte offsets and sequence numbers
 - Lossless cutting of compressed video
   * Allows hiding of keyframes and other dependencies at the start of the video
 - Stream packet data compression
   * Zstd, useful for general compression of packets, and metadata
   * Brotli, useful for subtitlesc
 - Uncompressed video and audio encapsulation
   * Including support for packed video data
   * Supports Bayer tiling, allowing storage of raw camera data
   * Supports DNG data
 - For MPEG codecs, a real 64-bit dts timestamp
   * Eliminates breaking of decoder state
 - Color profile support
   * Regular ICC profiles
   * Professional color lookup table support (Adobe .cube)
 - User data
   * Compression and stream ID are supported
   * Multiple fields reserved in multiple structures
   * Allows users to inject their own flags into packets
 - User codecs
   * Allows for clean implementation and encapsulation of custom codecs

The presence of all of these features future-proofs the design, and makes the protocol useful everywhere,
from deep space transmissions, long-term archival, tape storage, internet streams, broadcasting,
microcontroller to microcontroller I2C, Ethernet, cameras, and final deliverables of productions.

## libavtransport

The project provides a reference implementation, **libavtransport** with full support for the standard.

As well as being clean, segmented and commented, the code is a fully usable, optimized, robust, and secure
library, rather than only being a reference. The library is meant for embedding into different projects.

The reference library is written in standard C23, to high standards of security, reliability and resilience.
An alternative, non-reference implementation written in pure Rust is also in development.

Information on the internals of the library can be found in its own [README](libavtransport/README.md).

### Building

To build the library, run the standard meson build process:

```
meson build
ninja -C build
```

This will create a shared library by default, and will compile all [tools](#tools).

Dependencies:
 - [libcbor](https://github.com/PJK/libcbor)
 - [librqturbo](https://github.com/cyanreg/librqturbo)

The packages can be compiled and statically linked simultaneously as libavtransport is build.
Simply create a `subprojects` directory, and clone each package into it.

The following dependencies are optional:
 - [xxHash](https://github.com/Cyan4973/xxHash)
   * Internal embedded copy is used if not present
 - [OpenSSL](https://github.com/openssl/openssl)
   * Used for QUIC support
 - [libzstd](https://github.com/facebook/zstd)
 - [libbrotli](https://github.com/google/brotli)
 - [liburing](https://github.com/axboe/liburing)

As with non-optional dependencies, repositories can be cloned into `subprojects`.

## Tools

The project provides tools to enable the creation, conversion, and provide information
about AVTransport streams.

### avcat

**avcat** is a swiss army knife program for creating and manipulating AVTransport streams,
and streams of different formats if FFmpeg support was enabled.

Information on its usage is provided in the README of the tool.

avcat is automatically built when [libavtransport](#libavtransport) is enabled.

### avtdump

**avtdump** is a utility to inspect AVTransport streams and files, logging any issues,
stream changes, packet sizes and so on.

## Links

You can talk about the project and get in touch with developers on:
 - IRC: **`#avtransport`** on the [Libera.Chat](ircs://irc.libera.chat:6697) network
  - [Libera.Chat’s guide on how to connect](https://libera.chat/guides/connect)
    - Or use the [Kiwi web client](https://kiwiirc.com/nextclient/irc.libera.chat/?#avtransport)
 - Matrix: **`#avtransport`** on the [pars.ee](https://matrix.to/#/#avtransport:pars.ee) network

## License

The [specifications document)(draft-avtransport-spec.bs), and generated [HTML](https://cyanreg.github.io/avtransport/)
are licensed under the **MIT-Zero** license (public domain).

The reference software (`libavtransport`) is completely licensed under the [**BSD 2-clause license**](LICENSE.md),
in order to permit embedding, and free creation of alternative implementations.
