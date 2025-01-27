/*
 * Copyright © 2024, Lynne
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AVTRANSPORT_IO_COMMON
#define AVTRANSPORT_IO_COMMON

#include "address.h"
#include "packet_common.h"

/* Main type for offsets */
typedef int64_t avt_pos;

enum AVTIOType {
    AVT_IO_NULL,       /* Takes nothing */
    AVT_IO_FILE,       /* Takes a path */
    AVT_IO_FD,         /* Takes an integer handle fd */
    AVT_IO_UNIX,       /* Takes a Unix-domain integer handle  */
    AVT_IO_UDP,        /* UDP network connection */
    AVT_IO_UDP_LITE,   /* UDP-Lite network connection */
    AVT_IO_CALLBACK,   /* Data-level callback */
    AVT_IO_INVALID,    /* Invalid */
};

enum AVTConnectionStateFlags {
    AVT_CONN_STATE_NEW_MTU,
    AVT_CONN_STATE_NEW_DROPPED_IN,
};

typedef struct AVTConnectionState {
    enum AVTConnectionStateFlags flags;

    /* Raw MTU of the connection */
    uint32_t mtu;

    /* Number of dropped packets on the input */
    uint64_t nb_dropped_in;
} AVTConnectionState;

enum AVTIOReadFlags {
    /* Indicates that the read must be mutable. The IO must not modify
     * the buffer it receives, and must use it. */
    AVT_IO_READ_MUTABLE = 1 << 0,
};

/* Low level interface */
typedef struct AVTIOCtx AVTIOCtx;
typedef struct AVTIO {
    const char *name;
    enum AVTIOType type;

    /* Use avt_io_open instead, which autodetects the best backend */
    int (*init)(AVTContext *ctx, AVTIOCtx **io, AVTAddress *addr);

    /* Get maximum packet size, excluding any headers */
    int (*get_max_pkt_len)(AVTIOCtx *io, size_t *mtu);

    /* Attempt to add a secondary destination, NULL if unsupported */
    int (*add_dst)(AVTIOCtx *io, AVTAddress *addr);

    /* Removes a secondary destination, NULL if unsupported */
    int (*del_dst)(AVTIOCtx *io, AVTAddress *addr);

    /* Write multiple packets.
     * Returns positive offset after writing on success, otherwise negative error.
     * May be NULL if unsupported. */
    avt_pos (*write_vec)(AVTIOCtx *io, AVTPktd *pkt, uint32_t nb_pkt,
                         int64_t timeout);

    /* Write a single packet to the output.
     * Returns positive offset after writing on success, otherwise negative error. */
    avt_pos (*write_pkt)(AVTIOCtx *io, AVTPktd *p, int64_t timeout);

    /* Rewrite a packet at a specific location.
     * The old packet's size must exactly match the new packet. */
    avt_pos (*rewrite)(AVTIOCtx *io, AVTPktd *p, avt_pos off, int64_t timeout);

    /* Read input from IO. Must be called with a valid AVTBuffer, which
     * has enough capacity to hold len number of bytes.
     * IOs are allowed to replace the buffer unless AVT_IO_READ_MUTABLE is set.
     *
     * The amount of bytes read may not match len.
     *
     * Returns positive current offset after reading on success,
     * otherwise negative error. */
    avt_pos (*read_input)(AVTIOCtx *io, AVTBuffer *buf, size_t len,
                          int64_t timeout, enum AVTIOReadFlags flags);

    /* Set the read position */
    avt_pos (*seek)(AVTIOCtx *io, avt_pos off);

    /* Flush data written */
    int (*flush)(AVTIOCtx *io, int64_t timeout);

    /* Close */
    int (*close)(AVTIOCtx **io);
} AVTIO;

/* Initialize an IO (protocols-use, mainly) */
int avt_io_init(AVTContext *ctx, const AVTIO **io, AVTIOCtx **io_ctx,
                AVTAddress *addr);

#endif /* AVTRANSPORT_IO_COMMON */
