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

#ifndef AVTRANSPORT_H
#define AVTRANSPORT_H

#include "connection.h"
#include "send.h"
#include "receive.h"

enum AVTLogLevel {
    AVT_LOG_QUIET    = -(1 << 0),
    AVT_LOG_FATAL    =  (0 << 0),
    AVT_LOG_ERROR    = +(1 << 0),
    AVT_LOG_WARN     = +(1 << 1),
    AVT_LOG_INFO     = +(1 << 2),
    AVT_LOG_VERBOSE  = +(1 << 3),
    AVT_LOG_DEBUG    = +(1 << 4),
    AVT_LOG_TRACE    = +(1 << 5),
};

/* Library context-level options */
typedef struct AVTContextOptions {
    /* Logging context */
    void *log_opaque;

    /* Logging callback */
    void (*log_cb)(void *log_opaque, enum AVTLogLevel level,
                   const char *format, va_list args, int error);

    char producer_name[16];   /* Name of the project linking to libavtransport */
    uint16_t producer_ver[3]; /* Major, minor, micro version */
} AVTContextOptions;

/* Allocate an AVTransport context with the given context options. */
AVT_API int avt_init(AVTContext **ctx, AVTContextOptions *opts);

/* Uninitialize a context, closing all connections and files gracefully,
 * and free all memory used. */
AVT_API void avt_close(AVTContext **ctx);

/* Main logging function */
AVT_API void avt_log(void *ctx, enum AVTLogLevel level, const char *fmt, ...) avt_printf_format(3, 4);

#endif /* LIBAVTRANSPORT_VIDEO_HEADER */
