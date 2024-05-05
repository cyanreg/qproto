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

#include <string.h>
#include <inttypes.h>
#include <avtransport/avtransport.h>

#include "merger.h"

#include "packet_decode.h"
#include "bytestream.h"
#include "buffer.h"
#include "utils_packet.h"
#include "mem.h"

static inline int fill_ranges(AVTMerger *m, bool is_parity,
                              uint32_t seg_off, uint32_t seg_size)
{
    AVTMergerRange *ranges;
    uint32_t nb_ranges;
    uint32_t *nb_ranges_dst;
    uint32_t ranges_allocated;

    if (!is_parity) {
        ranges = m->ranges;
        nb_ranges = m->nb_ranges;
        nb_ranges_dst = &m->nb_ranges;
        ranges_allocated = m->ranges_allocated;
    } else {
        ranges = m->parity_ranges;
        nb_ranges = m->nb_parity_ranges;
        nb_ranges_dst = &m->nb_parity_ranges;
        ranges_allocated = m->parity_ranges_allocated;
    }

    int dst_idx = 0;
    int consolidate_idx = -1;
    for (auto i = 0; i < nb_ranges; i++) {
        AVTMergerRange *r = &ranges[i];
        if ((seg_off + seg_size) == r->offset) {
            /* Prepend to existing range */
            r->offset = seg_off;
            r->size += seg_size;

            /* If the previous range is not completed with this one, exit */
            if (!i || ((ranges[i - 1].offset + ranges[i - 1].size) < r->offset))
                return AVT_ERROR(EAGAIN);

            consolidate_idx = i - 1;
            break;
        } else if ((r->offset + r->size) == seg_off) {
            /* Extend exisint range */
            r->size += seg_size;

            if ((i == (nb_ranges - 1)) || ((r->offset + r->size) < ranges[i + 1].offset))
                return AVT_ERROR(EAGAIN);

            consolidate_idx = i;
            break;
        } else if (seg_off > (r->offset + r->size)) {
            dst_idx = i + 1;
        }
    }

    /* Add a range */
    if (consolidate_idx < 0) {
        if ((nb_ranges + 1) > ranges_allocated) {
            AVTMergerRange *tmp = avt_reallocarray(ranges,
                                                   (nb_ranges + 1) << 1,
                                                   sizeof(*tmp));
            if (!tmp)
                return AVT_ERROR(ENOMEM);

            ranges = tmp;
            ranges_allocated = (m->nb_ranges + 1) << 1;
            if (!is_parity) {
                m->ranges = ranges;
                m->ranges_allocated = ranges_allocated;
            } else {
                m->parity_ranges = ranges;
                m->parity_ranges_allocated = ranges_allocated;
            }
        }

        memmove(&ranges[dst_idx + 1], &ranges[dst_idx], nb_ranges - dst_idx - 1);
        nb_ranges++;
        *nb_ranges_dst = nb_ranges;
        return AVT_ERROR(EAGAIN);
    }

    ranges[consolidate_idx].size = ranges[consolidate_idx + 1].size;
    memmove(&ranges[consolidate_idx + 1], &ranges[consolidate_idx + 2],
            nb_ranges - consolidate_idx - 2);
    nb_ranges--;
    *nb_ranges_dst = nb_ranges;

    return AVT_ERROR(EAGAIN);
}

static inline int fill_phantom_header(void *log_ctx, AVTMerger *m,
                                      AVTPktd *p, bool is_parity)
{
    int hdr_part = p->pkt.seq % 7;
    if (!is_parity)
        memcpy(&m->p.hdr[4*hdr_part], p->pkt.generic_segment.header_7, 4);
    else
        memcpy(&m->p.hdr[4*hdr_part], p->pkt.generic_parity.header_7, 4);

    m->hdr_mask |= 1 << (6 - hdr_part);

    /* We're not ready to reconstruct the header yet */
    if (!(m->hdr_mask == 0x7F))
        return 0;

    uint16_t tgt_desc = AVT_RB16(&m->p.hdr[0]);
    if ((tgt_desc == (AVT_PKT_TIME_SYNC & 0xFF00)) ||
        (tgt_desc == (AVT_PKT_STREAM_DATA & 0xFF00)))
        tgt_desc &= 0xFF00;

    AVTBytestream bs = avt_bs_init(m->p.hdr, sizeof(m->p.hdr));

    // TODO more sanity checking

    switch (tgt_desc) {
    case AVT_PKT_STREAM_DATA & ~(AVT_PKT_FLAG_LSB_BITMASK):
        avt_decode_stream_data(&bs, &m->p.pkt.stream_data);
        break;
    case AVT_PKT_LUT_ICC:
        avt_decode_lut_icc(&bs, &m->p.pkt.lut_icc);
        /* name will be missing */
        break;
    case AVT_PKT_FONT_DATA:
        avt_decode_font_data(&bs, &m->p.pkt.font_data);
        /* name will be missing */
        break;
    case AVT_PKT_USER_DATA:
        avt_decode_user_data(&bs, &m->p.pkt.user_data);
        break;
    case AVT_PKT_STREAM_CONFIG: [[fallthrough]];
    case AVT_PKT_METADATA:
        avt_decode_generic_data(&bs, &m->p.pkt.generic_data);
        break;
    default:
        /* Any other packets may not be segmented. Perhaps the top part of the
         * reconstructed segment is faulty. Invalidate it. */
        m->hdr_mask &= 0x3F;

        /* If the current packet contributed this part, skip copying the
         * payload, as it's likely wrong. Otherwise continue. */
        return hdr_part == 0 ? AVT_ERROR(EAGAIN) : 0;
    }

    m->p_avail = true;

    return 0;
}

int avt_pkt_merge_seg(void *log_ctx, AVTMerger *m, AVTPktd *p)
{
    bool is_parity;
    uint32_t seg_off, seg_size, tot_size;
    int ret = avt_packet_series(p, &is_parity, &seg_off, &seg_size, &tot_size);

    /* Packet needs nothing else */
    if (!ret)
        return 0;

    size_t src_size;
    uint8_t *src = avt_buffer_get_data(&p->pl, &src_size);
    if (src_size != seg_size) {
        avt_log(log_ctx, AVT_LOG_ERROR, "Mismatch between signalled payload size "
                                        "%u and payload %zu in %" PRIu64,
                seg_size, src_size, p->pkt.seq);
    }

    if (!m->active) {
        m->hdr_mask = 0x0;
        m->nb_ranges = 0;
        m->nb_parity_ranges = 0;
        m->pkt_len_track = 0;
        m->target_tot_len = tot_size;
        m->p_avail = false;

        if (ret > 0) {
            /* Starting with a segment, not the actual start */
            m->p = *p;
            m->p_avail = true;
            m->hdr_mask = 0x7F;
            m->target = p->pkt.seq;
        } else if (!is_parity) {
            /* Starting with a segment */
            const uint32_t hdr_part = p->pkt.seq % 7;
            memcpy(&m->p.hdr[4*hdr_part], p->pkt.generic_segment.header_7, 4);
            m->hdr_mask |= 1 << (6 - hdr_part);
            m->target = p->pkt.generic_segment.target_seq;
        } else {
            /* Starting with a parity segment */
            const uint32_t hdr_part = p->pkt.seq % 7;
            memcpy(&m->p.hdr[4*hdr_part], p->pkt.generic_parity.header_7, 4);
            m->hdr_mask |= 1 << (6 - hdr_part);
            m->target = p->pkt.generic_parity.target_seq;
        }

        /* Ensure there's enough memory for some amount of ranges */
        AVTMergerRange *tmp;
        if (m->ranges_allocated < 128) {
            tmp = avt_reallocarray(m->ranges,
                                   (m->nb_ranges + 1) << 1,
                                   sizeof(*tmp));
            if (!tmp)
                return AVT_ERROR(ENOMEM);

            m->ranges = tmp;
            m->ranges_allocated = (m->nb_ranges + 1) << 1;
        }

        if (m->parity_ranges_allocated < 128) {
            tmp = avt_reallocarray(m->parity_ranges,
                                   (m->nb_parity_ranges + 1) << 1,
                                   sizeof(*tmp));
            if (!tmp)
                return AVT_ERROR(ENOMEM);

            m->parity_ranges = tmp;
            m->parity_ranges_allocated = (m->nb_parity_ranges + 1) << 1;
        }

        /* Setup buffer */
        AVTBuffer *target;
        if (!is_parity)
            target = &m->p.pl;
        else
            target = &m->parity;

        /* Have enough memory for either data or parity */
        if (seg_off || avt_buffer_read_only(&p->pl)) {
            /* In case we have a read-only buffer, copy the data */
            AVTBuffer tmp_buf;
            uint8_t *dst = avt_buffer_quick_alloc(&tmp_buf, tot_size);
            if (!dst)
                return AVT_ERROR(ENOMEM);

            memcpy(dst + seg_off, src, src_size);
            avt_buffer_quick_unref(&p->pl);
            *target = tmp_buf;
        } else {
            /* Resize the buffer if possible */
            int err = avt_buffer_resize(&p->pl, tot_size);
            if (err < 0)
                return err;

            *target = p->pl;
        }

        /* Mark what we have available */
        if (!is_parity) {
            m->ranges[m->nb_ranges++] = (AVTMergerRange) { seg_off, src_size };
            m->pkt_len_track += seg_size;
        } else {
            m->parity_ranges[m->nb_parity_ranges++] = (AVTMergerRange) { seg_off, src_size };
            m->pkt_parity_len_track += seg_size;
        }

        m->active = true;

        return AVT_ERROR(EAGAIN);
    }

    /* Sanity checking */
    if (ret == 0 && m->target == p->pkt.seq) {
        avt_log(log_ctx, AVT_LOG_WARN, "Header packet for %" PRIu64 " indicates "
                                       "no segmentation, but segments received",
                p->pkt.seq);
    }

    /* Packet header state */
    if (!m->p_avail && (ret < -1)) {
        ret = fill_phantom_header(log_ctx, m, p, is_parity);
        if (ret < 0)
            return ret;
    } else if (ret == 0) {
        /* We got a real header packet */
        m->p.pkt = p->pkt;
        memcpy(m->p.hdr, p->hdr, sizeof(p->hdr));
        m->p_avail = true;
    }

    /* Copy new data */
    if (!is_parity) {
        uint8_t *dst = avt_buffer_get_data(&m->p.pl, NULL);
        memcpy(dst + seg_off, src, src_size);
        avt_buffer_quick_unref(&p->pl);
        m->pkt_len_track += seg_size;
    } else {
        uint8_t *dst = avt_buffer_get_data(&m->parity, NULL);
        memcpy(dst + seg_off, src, src_size);
        avt_buffer_quick_unref(&p->pl);
        m->pkt_parity_len_track += seg_size;
    }

    /* Track ranges */
    ret = fill_ranges(m, is_parity, seg_off, src_size /* Using the real payload size*/);
    if (ret < 0)
        return ret;

    if (m->pkt_parity_len_track == m->parity_tot_len) {
        // TODO: do something with the parity data here
        ;
    }

    /* We can output something */
    if (m->pkt_len_track == m->target_tot_len) {
        /* As each packet with a payload is required to have a non-zero amount
         * of payload, this should be impossible, as reconstructed headers don't
         * contain any payload. */
        avt_assert1(m->p_avail);
        *p = m->p;
        m->active = false;
        return 0;
    }

    return AVT_ERROR(EAGAIN);
}
