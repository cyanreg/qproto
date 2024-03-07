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

#ifndef AVTRANSPORT_ADDRESS_H
#define AVTRANSPORT_ADDRESS_H

#include <uchar.h>

#include <avtransport/connection.h>

/* Extend the <connection.h> definitions further */
#define AVT_PROTOCOL_NOOP    (0)
#define AVT_PROTOCOL_PIPE    (AVT_PROTOCOL_QUIC    + 1)
#define AVT_PROTOCOL_FILE    (AVT_PROTOCOL_PIPE    + 1)
#define AVT_PROTOCOL_FD      (AVT_PROTOCOL_FILE    + 1)
#define AVT_PROTOCOL_PACKET  (AVT_PROTOCOL_FD      + 1)

typedef struct AVTAddress {
    /** FILE PATH */
    char8_t *path;

    /** FILE DESCRIPTOR **/
    int fd;

    /** NETWORK **/
    enum AVTProtocolType proto;
    enum AVTProtocolMode mode;

    uint8_t ip[16]; /* Always mapped to IPv6 */
    uint16_t port;
    uint32_t scope; /* sin6_scope_id */

    /* Interface */
    char8_t *interface;

    /* Server or client */
    bool listen;

    /* Default stream IDs */
    uint16_t *default_sid;
    int nb_default_sid;

    /* Options */
    struct {
        int rx_buf; /* UDP receive buffer size */
        int tx_buf; /* UDP transmit buffer size */
    } opts;
} AVTAddress;

/* Utilities */
int avt_addr_4to6(uint8_t ip6[16], uint32_t ip4);
int avt_addr_get_scope(void *log_ctx, const uint8_t ip[16],
                       uint32_t *scope, const char *iface);

int avt_addr_from_url(void *log_ctx, AVTAddress *addr,
                      bool server, const char *path);

int avt_addr_from_info(void *log_ctx, AVTAddress *addr, AVTConnectionInfo *info);

void avt_addr_free(AVTAddress *addr);

#endif /* AVTRANSPORT_ADDRESS_H */