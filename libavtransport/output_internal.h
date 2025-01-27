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

#ifndef AVTRANSPORT_OUTPUT_INTERNAL_H
#define AVTRANSPORT_OUTPUT_INTERNAL_H

#include <avtransport/send.h>

#include "common.h"
#include "connection_internal.h"

#include "config.h"

#ifdef CONFIG_HAVE_LIBXXH
#include <xxhash.h>
#else
#define XXH_INLINE_ALL
#include "extern/xxhash.h"
#endif

#ifdef CONFIG_HAVE_LIBZSTD
#include <zstd.h>
#endif

typedef struct AVTSender {
    AVTContext *ctx;
    AVTSenderOptions opts;

    AVTConnection **conn;
    uint32_t nb_conn;
    uint32_t nb_conn_alloc;

    AVTStream streams[UINT16_MAX];
    uint16_t active_stream_idx[UINT16_MAX];
    int nb_streams;

    uint64_t epoch;

    XXH3_state_t *xxh_state;
#ifdef CONFIG_HAVE_LIBZSTD
    ZSTD_CCtx *zstd_ctx;
#endif
} AVTSender;

#endif /* AVTRANSPORT_OUTPUT_INTERNAL_H */
