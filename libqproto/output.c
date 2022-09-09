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

#include <string.h>

#include "output.h"
#include "utils.h"

extern const PQOutput pq_output_file;
extern const PQOutput pq_output_socket;

static const PQOutput *pq_output_list[] = {
    &pq_output_file,
    &pq_output_socket,

    NULL,
};

int qp_output_open(QprotoContext *qp, QprotoOutputDestination *dst,
                   QprotoOutputOptions *opts)
{
    const PQOutput *out;
    for (out = pq_output_list[0]; out; out++) {
        if (out->type == dst->type)
            break;
    }

    if (!out)
        return QP_ERROR(ENOTSUP);

    qp->dst.cb = out;
    qp->dst.dst = *dst;

    int ret = out->init(qp, &qp->dst.ctx, dst, opts);
    if (ret < 0)
        return ret;

    uint8_t hdr[372];
    uint8_t *h = hdr;

    PQ_WBL(h, 16, 0x5170);
    PQ_WBL(h, 16, 0x0);
    PQ_WBL(h, 32, atomic_fetch_add_explicit(&qp->dst.seq, 1, memory_order_relaxed));

    *h = strlen(PROJECT_NAME);
    h++;

    memcpy(h, PROJECT_NAME, h[-1]);
    h += h[-1];

    PQ_WBL(h, 16, PROJECT_VERSION_MAJOR);
    PQ_WBL(h, 16, PROJECT_VERSION_MINOR);
    PQ_WBL(h, 16, PROJECT_VERSION_MICRO);

    uint64_t raptor = 0;
    PQ_WBL(h, 64, raptor);

    ret = qp->dst.cb->output(qp, qp->dst.ctx, hdr, h - hdr, NULL);
    if (ret < 0)
        qp->dst.cb->close(qp, &qp->dst.ctx);

    return ret;
}

int qp_output_write_stream_data(QprotoContext *qp, QprotoStream *st,
                                QprotoPacket *pkt)
{
    uint8_t hdr[372];
    uint8_t *h = hdr;
    uint64_t raptor;
    uint16_t desc = (0x01 << 8) | (pkt->type & 0xC0);

    PQ_WBL(h, 16, desc);
    PQ_WBL(h, 16, st->id);
    PQ_WBL(h, 32, atomic_fetch_add_explicit(&qp->dst.seq, 1, memory_order_relaxed));
    PQ_WBL(h, 64, pkt->pts);
    PQ_WBL(h, 64, pkt->duration);
    PQ_WBL(h, 64, qp_buffer_get_data_len(pkt->data));

    // TODO
    raptor = 0;
    PQ_WBL(h, 64, raptor);

    return qp->dst.cb->output(qp, qp->dst.ctx, hdr, h - hdr, pkt->data);
}

int qp_output_close(QprotoContext *qp)
{
    if (!qp->dst.ctx)
        return QP_ERROR(EINVAL);

    return qp->dst.cb->close(qp, &qp->dst.ctx);
}
