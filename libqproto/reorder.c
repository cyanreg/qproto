/*
 * Copyright © 2022 Lynne
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the “Software”), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>

#include "reorder.h"
#include "common.h"
#include "buffer.h"

typedef struct PQBufEntry {
    uint32_t seq;
    ptrdiff_t seg_offset;
    size_t    seg_len;

    QprotoBuffer data;

    struct PQBufEntry *start;
    struct PQBufEntry *prev;
    struct PQBufEntry *next;
} PQBufEntry;

typedef struct PQReorder {
    PQBufEntry *stream_data;

    PQBufEntry **avail;
    int nb_avail;
    PQBufEntry **used;
    int nb_used;
} PQReorder;

int pq_reorder_init(QprotoContext *qp, size_t max_buffer)
{
    PQReorder *ctx = qp->src.rctx = calloc(1, sizeof(PQReorder));
    if (!ctx)
        return QP_ERROR(ENOMEM);

    ctx->nb_avail = 32;

    ctx->avail = calloc(ctx->nb_avail, sizeof(PQBufEntry *));
    if (!ctx->avail)
        return QP_ERROR(ENOMEM);

    ctx->used = calloc(ctx->nb_avail, sizeof(PQBufEntry *));
    if (!ctx->used)
        return QP_ERROR(ENOMEM);

    for (int i = 0; i < ctx->nb_avail; i++) {
        ctx->avail[i] = calloc(1, sizeof(PQBufEntry));
        if (!ctx->avail[i])
            return QP_ERROR(ENOMEM);
    }

    return 0;
}

static int pq_reorder_alloc(PQReorder *ctx)
{
    PQBufEntry **atmp = realloc(ctx->avail, (ctx->nb_avail + 1)*sizeof(PQBufEntry *));
    if (!atmp)
        return QP_ERROR(ENOMEM);

    ctx->avail = atmp;

    atmp = realloc(ctx->used, (ctx->nb_used + 1)*sizeof(PQBufEntry *));
    if (!atmp)
        return QP_ERROR(ENOMEM);

    ctx->used = atmp;

    atmp[ctx->nb_avail] = calloc(1, sizeof(PQBufEntry));
    if (!atmp[ctx->nb_avail])
        return QP_ERROR(ENOMEM);

    ctx->nb_avail++;

    return 0;
}

int pq_reorder_push_pkt(QprotoContext *qp, QprotoBuffer *data,
                        ptrdiff_t offset, uint32_t seq,
                        enum PQPacketType type)
{
    int ret;
    PQReorder *ctx = qp->src.rctx;
    PQBufEntry *ins, *tmp, *entry = NULL;

    if (type == (QP_PKT_STREAM_DATA & 0xFF00) ||
        type == (QP_PKT_STREAM_SEG_DATA) ||
        type == (QP_PKT_STREAM_SEG_END))
        entry = qp->src.rctx->stream_data;

    while (entry->next && (entry->seq < seq) && (entry->seg_offset < offset))
        entry = entry->next;

    if (!ctx->nb_avail) {
        ret = pq_reorder_alloc(ctx);
        if (ret < 0)
            return ret;
    }

    ins = ctx->avail[ctx->nb_avail - 1];
    ctx->nb_avail--;
    ctx->used[ctx->nb_used] = ins;
    ctx->nb_used++;

    ins->seq = seq;
    ins->seg_offset = offset;
    ins->seg_len = qp_buffer_get_data_len(data);

    tmp = entry->next;
    entry->next = ins;
    ins->next = tmp;
    ins->prev = entry;

    return 0;
}

void pq_reorder_uninit(QprotoContext *qp)
{
    PQReorder *ctx = qp->src.rctx;

    for (int i = 0; i < ctx->nb_used; i++) {
        pq_buffer_quick_unref(&ctx->used[i]->data);
        free(ctx->used[i]);
    }
    for (int i = 0; i < ctx->nb_avail; i++)
        free(ctx->avail[i]);

    free(ctx->avail);
    free(ctx->used);

    free(qp->src.rctx);
    qp->src.rctx = NULL;
}