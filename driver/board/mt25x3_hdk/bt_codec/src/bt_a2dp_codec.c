/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "bt_a2dp_codec_internal.h"
//#define BT_A2DP_BITSTREAM_DUMP_DEBUG
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
/* #define AWS_DEBUG_CODE */
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */

const bt_codec_sbc_t sink_capability_sbc[1] = {
    {
        18,  /* min_bit_pool       */
        75,  /* max_bit_pool       */
        0xf, /* block_len: all     */
        0xf, /* subband_num: all   */
        0x3, /* both snr/loudness  */
        0xf, /* sample_rate: all   */
        0xf  /* channel_mode: all  */
    }
};

const bt_codec_aac_t sink_capability_aac[1] = {
    {
        true,    /*VBR         */
        0xc0,    /*Object type */
        0x03,    /*Channels    */
        0x0ff8,  /*Sample_rate */
        0x60000  /*bit_rate, 384 Kbps */
    }
};

static void bt_set_buffer(bt_media_handle_t *handle, uint8_t *buffer, uint32_t length)
{
    handle->buffer_info.buffer_base = buffer;
    length &= ~0x1; // make buffer size even
    handle->buffer_info.buffer_size = length;
    handle->buffer_info.write = 0;
    handle->buffer_info.read = 0;
    handle->waiting = false;
    handle->underflow = false;
}


static void bt_set_get_data_function(bt_media_handle_t *handle, bt_codec_get_data func)
{
    handle->directly_access_dsp_function = func;
}

static void bt_set_get_data_count_function(bt_media_handle_t *handle, bt_codec_get_data_count func)
{
    handle->get_data_count_function = func;
}

static void bt_get_write_buffer(bt_media_handle_t *handle, uint8_t **buffer, uint32_t *length)
{
    int32_t count = 0;

    if (handle->buffer_info.read > handle->buffer_info.write) {
        count = handle->buffer_info.read - handle->buffer_info.write - 1;
    } else if (handle->buffer_info.read == 0) {
        count = handle->buffer_info.buffer_size - handle->buffer_info.write - 1;
    } else {
        count = handle->buffer_info.buffer_size - handle->buffer_info.write;
    }
    *buffer = handle->buffer_info.buffer_base + handle->buffer_info.write;
    *length = count;
}

static void bt_get_read_buffer(bt_media_handle_t *handle, uint8_t **buffer, uint32_t *length)
{
    int32_t count = 0;

    if (handle->buffer_info.write >= handle->buffer_info.read) {
        count = handle->buffer_info.write - handle->buffer_info.read;
    } else {
        count = handle->buffer_info.buffer_size - handle->buffer_info.read;
    }
    *buffer = handle->buffer_info.buffer_base + handle->buffer_info.read;
    *length = count;
}

static void bt_write_data_done(bt_media_handle_t *handle, uint32_t length)
{
    handle->buffer_info.write += length;
#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_I("[A2DP]write--wr: %d, len: %d\n", handle->buffer_info.write, length);
#endif
    if (handle->buffer_info.write == handle->buffer_info.buffer_size) {
        handle->buffer_info.write = 0;
    }
}

static void bt_finish_write_data(bt_media_handle_t *handle)
{
    handle->waiting = false;
    handle->underflow = false;
}

static void bt_reset_share_buffer(bt_media_handle_t *handle)
{
    handle->buffer_info.write = 0;
    handle->buffer_info.read = 0;
    handle->waiting = false;
    handle->underflow = false;
}

static void bt_read_data_done(bt_media_handle_t *handle, uint32_t length)
{
    handle->buffer_info.read += length;
#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_I("[A2DP]read--rd: %d, len: %d\n", handle->buffer_info.read, length);
#endif
    if (handle->buffer_info.read == handle->buffer_info.buffer_size) {
        handle->buffer_info.read = 0;
    }
}

static int32_t bt_get_free_space(bt_media_handle_t *handle)
{
    int32_t count = 0;

    count = handle->buffer_info.read - handle->buffer_info.write - 1;
    if (count < 0) {
        count += handle->buffer_info.buffer_size;
    }
    return count;
}

static int32_t bt_get_data_count(bt_media_handle_t *handle)
{
    int32_t count = 0;

    count = handle->buffer_info.write - handle->buffer_info.read;
    if (count < 0) {
        count += handle->buffer_info.buffer_size;
    }
    return count;
}

static void bt_codec_buffer_function_init(bt_media_handle_t *handle)
{
    handle->set_buffer         = bt_set_buffer;
    handle->set_get_data_function = bt_set_get_data_function;
    handle->set_get_data_count_function = bt_set_get_data_count_function;
    handle->get_write_buffer   = bt_get_write_buffer;
    handle->get_read_buffer    = bt_get_read_buffer;
    handle->write_data_done    = bt_write_data_done;
    handle->finish_write_data  = bt_finish_write_data;
    handle->reset_share_buffer = bt_reset_share_buffer;
    handle->read_data_done     = bt_read_data_done;
    handle->get_free_space     = bt_get_free_space;
    handle->get_data_count     = bt_get_data_count;
}

#if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG)
#define BT_A2DP_BS_LEN 160000
uint32_t bt_a2dp_ptr = 0;
uint8_t bt_a2dp_bitstream[BT_A2DP_BS_LEN];
#endif /* BT_A2DP_BITSTREAM_DUMP_DEBUG */

#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
static uint32_t bt_codec_a2dp_aws_convert_sampling_rate_from_index_to_value(uint32_t sampling_rate_index)
{
    uint32_t sampling_rate_value;
    if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_8KHZ) {
        sampling_rate_value = 8000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_11_025KHZ) {
        sampling_rate_value = 11025;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_12KHZ) {
        sampling_rate_value = 12000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_16KHZ) {
        sampling_rate_value = 16000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_22_05KHZ) {
        sampling_rate_value = 22050;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_24KHZ) {
        sampling_rate_value = 24000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_32KHZ) {
        sampling_rate_value = 32000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_44_1KHZ) {
        sampling_rate_value = 44100;
    } else {
        sampling_rate_value = 48000;
    }
    return sampling_rate_value;
}

void bt_codec_a2dp_aws_src_process (void *src_hdl, int16_t *p_in, uint32_t in_byte_cnt, int16_t *p_ou, uint32_t ou_byte_cnt)
{   /* Input / output channel count are fixed to stereo */
    uint32_t in_smpl_cnt = in_byte_cnt / sizeof(int16_t) / 2;
    uint32_t ou_smpl_cnt = ou_byte_cnt / sizeof(int16_t) / 2;
    if (in_smpl_cnt > 0 && ou_smpl_cnt > 0) {
        bt_codec_aws_src_handle_t *p_src_info = (bt_codec_aws_src_handle_t *)src_hdl;
        uint32_t loop_idx;
        {   /* Part 1: Write beginning output samples which need to reference to previous samples */
            int32_t prev_L = (int32_t)p_src_info->previous_sample_L;
            int32_t prev_R = (int32_t)p_src_info->previous_sample_R;
            int32_t first_L = (int32_t)p_in[0];
            int32_t first_R = (int32_t)p_in[1];
            for (loop_idx = 1; loop_idx < ou_smpl_cnt; loop_idx++) {
                uint32_t phase = (loop_idx * in_smpl_cnt << AWS_SRC_PHASE_BITS) / ou_smpl_cnt;
                uint32_t idx_lo = phase >> AWS_SRC_PHASE_BITS << 1;
                if (idx_lo > 0) {
                    break;
                }
                phase &= AWS_SRC_PHASE_MASK;
                *p_ou++ = (int16_t)AWS_SRC_INTERPOLATION(prev_L, first_L, (int32_t)phase);
                *p_ou++ = (int16_t)AWS_SRC_INTERPOLATION(prev_R, first_R, (int32_t)phase);
            }
        }
        {   /* Part 2: Write normal output samples */
            for (; loop_idx < ou_smpl_cnt; loop_idx++) {
                uint32_t phase = (loop_idx * in_smpl_cnt << AWS_SRC_PHASE_BITS) / ou_smpl_cnt;
                uint32_t idx_lo = ((phase >> AWS_SRC_PHASE_BITS) - 1) << 1;
                int32_t val_lo_L = (int32_t)p_in[idx_lo + 0];
                int32_t val_lo_R = (int32_t)p_in[idx_lo + 1];
                int32_t val_hi_L = (int32_t)p_in[idx_lo + 2];
                int32_t val_hi_R = (int32_t)p_in[idx_lo + 3];
                phase &= AWS_SRC_PHASE_MASK;
                *p_ou++ = (int16_t)AWS_SRC_INTERPOLATION(val_lo_L, val_hi_L, (int32_t)phase);
                *p_ou++ = (int16_t)AWS_SRC_INTERPOLATION(val_lo_R, val_hi_R, (int32_t)phase);
            }
        }
        {   /* Part 3: Write last output samples & update previous samples */
            int16_t val_idx = (in_smpl_cnt - 1) << 1;
            int16_t val_L = p_in[val_idx + 0];
            int16_t val_R = p_in[val_idx + 1];
            *p_ou++ = val_L;
            *p_ou++ = val_R;
            p_src_info->previous_sample_L = val_L;
            p_src_info->previous_sample_R = val_R;
        }
    }
    return;
}

static void bt_codec_a2dp_aws_pcm_route_isr(void *data)
{
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)data;
    uint16_t word_cnt = *DSP_PCM_ROUTE_DATA_LENGTH & 0x7FFF;
    uint16_t dsp_addr = *DSP_PCM_ROUTE_DATA_ADDRESS;
    uint16_t page_num = *DSP_PCM_ROUTE_DATA_PAGENUM;
    uint16_t *p_frm_buf = p_info->pcm_route_frame_buffer;
    uint16_t *p_src_buf = p_info->pcm_route_src_buffer;
    volatile uint16_t *p_dsp_buf = DSP_DM_ADDR(page_num, dsp_addr);
    ring_buffer_information_t *p_ring = &p_info->pcm_route_ring_buffer;
    uint32_t byte_cnt = (uint32_t)word_cnt * sizeof(uint16_t);
#if defined(AWS_DEBUG_CODE)
    /* TASK_LOG_I("[AWS] PCM route ISR\r\n"); */
#endif  /* defined(AWS_DEBUG_CODE) */
    {   /* DSP buffer --> Frame buffer */
        audio_idma_read_from_dsp(p_frm_buf, p_dsp_buf, word_cnt);
    }
    {   /* Frame buffer --> Ring buffer */
        uint8_t *p_src_ring;
        uint32_t need_cnt;
        uint32_t in_byte_cnt = byte_cnt;
        uint32_t ou_byte_cnt;
        {   /* Update sample count */
            int32_t frm_smpl_cnt = (int32_t)audio_service_get_frame_sample_count();
            int32_t adjust_smpl_cnt = 0;
            bt_codec_aws_clock_skew_status_t status = p_info->clock_skew_status;
            if (status == BT_CODEC_AWS_CLOCK_SKEW_STATUS_BUSY) {    /* Adjust sample count */
                int32_t remained_smpl_cnt = p_info->remained_sample_count;
                int32_t pre_buf_smpl_cnt  = p_info->prebuffer_sample_count;
                adjust_smpl_cnt = remained_smpl_cnt;
                adjust_smpl_cnt = BOUNDED(adjust_smpl_cnt, AWS_CLOCK_SKEW_MAX_SAMPLE_COUNT, -frm_smpl_cnt); /* Bound control */
                if (pre_buf_smpl_cnt + adjust_smpl_cnt < 0) {   /* Underflow */
                    TASK_LOG_I("[AWS] skew buf underflow, pre %d + adjust %d < 0\r\n", (int)pre_buf_smpl_cnt, (int)adjust_smpl_cnt);
                    adjust_smpl_cnt = -pre_buf_smpl_cnt;
                    remained_smpl_cnt = 0;
                } else if (pre_buf_smpl_cnt + adjust_smpl_cnt + frm_smpl_cnt > AWS_CLOCK_SKEW_RING_BUFFER_SAMPLE_COUNT) {   /* Overflow */
                    TASK_LOG_I("[AWS] skew buf overflow, pre %d + adjust %d + frm %d > %d\r\n", (int)pre_buf_smpl_cnt, (int)adjust_smpl_cnt, (int)frm_smpl_cnt, (int)AWS_CLOCK_SKEW_RING_BUFFER_SAMPLE_COUNT);
                    adjust_smpl_cnt = AWS_CLOCK_SKEW_RING_BUFFER_SAMPLE_COUNT - pre_buf_smpl_cnt - frm_smpl_cnt;
                    remained_smpl_cnt = 0;
                } else {    /* Normal */
                    {   /* Speed control */
                        /* adjust_smpl_cnt = BOUNDED(adjust_smpl_cnt, increase_step, decrease_step); */
                    }
                    /* TASK_LOG_I("[AWS] skew buf normal, adjust %d\r\n", adjust_smpl_cnt); */
                    remained_smpl_cnt -= adjust_smpl_cnt;
                }
                pre_buf_smpl_cnt += adjust_smpl_cnt;
                frm_smpl_cnt += adjust_smpl_cnt;
                p_info->prebuffer_sample_count = pre_buf_smpl_cnt;
                p_info->remained_sample_count  = remained_smpl_cnt;
                if (remained_smpl_cnt == 0) {
                    status = BT_CODEC_AWS_CLOCK_SKEW_STATUS_IDLE;
                }
            }
            audio_dsp_update_audio_counter(-adjust_smpl_cnt);
            ou_byte_cnt = frm_smpl_cnt * 2 * sizeof(int16_t);
            p_info->clock_skew_status = status;
            p_info->accumulated_sample_count += (uint32_t)frm_smpl_cnt;
        }
        if (in_byte_cnt != ou_byte_cnt) {   /* Frame buffer --> SRC buffer */
            bt_codec_aws_src_handle_t *src_handle = &p_info->src_handle;
            p_src_ring = (uint8_t *)p_src_buf;
            need_cnt = ou_byte_cnt;
            bt_codec_a2dp_aws_src_process ((void *)src_handle, (int16_t *)p_frm_buf, in_byte_cnt, (int16_t *)p_src_buf, ou_byte_cnt);
        } else {
            p_src_ring = (uint8_t *)p_frm_buf;
            need_cnt = in_byte_cnt;
        }
        {   /* Frame / SRC buffer --> Ring buffer */
            uint32_t spce_cnt = ring_buffer_get_space_byte_count(p_ring);
            if (spce_cnt >= need_cnt) {
                int32_t loop_idx;
                uint8_t *p_src = p_src_ring;
                uint32_t src_cnt = need_cnt;
                for (loop_idx = 0; loop_idx < 2; loop_idx++) {
                    uint8_t *p_dst = NULL;
                    uint32_t dst_cnt = 0;
                    uint32_t write_cnt;
                    ring_buffer_get_write_information(p_ring, &p_dst, &dst_cnt);
                    write_cnt = MINIMUM(dst_cnt, src_cnt);
                    memcpy(p_dst, p_src, write_cnt);
                    p_src += write_cnt;
                    src_cnt -= write_cnt;
                    ring_buffer_write_done(p_ring, write_cnt);
                }
            } else {
                TASK_LOG_I("[AWS] ring buffer overflow, space byte %d < need byte %d\r\n", (int)spce_cnt, (int)need_cnt);
            }
        }
    }
    {   /* Ring buffer --> Frame buffer */
        uint32_t need_cnt = byte_cnt;
        uint32_t data_cnt = ring_buffer_get_data_byte_count(p_ring);
        if (data_cnt >= need_cnt) {
            int32_t loop_idx;
            uint8_t *p_dst = (uint8_t *)p_frm_buf;
            uint32_t dst_cnt = need_cnt;
            for (loop_idx = 0; loop_idx < 2; loop_idx++) {
                uint8_t *p_src = NULL;
                uint32_t src_cnt = 0;
                uint32_t read_cnt;
                ring_buffer_get_read_information(p_ring, &p_src, &src_cnt);
                read_cnt = MINIMUM(dst_cnt, src_cnt);
                memcpy(p_dst, p_src, read_cnt);
                p_dst += read_cnt;
                dst_cnt -= read_cnt;
                ring_buffer_read_done(p_ring, read_cnt);
            }
        } else {
            TASK_LOG_I("[AWS] ring buffer underflow, data byte %d < need byte %d\r\n", (int)data_cnt, (int)need_cnt);
            memset(p_frm_buf, 0, need_cnt);
        }
    }
    {   /* Frame buffer --> DSP buffer */
        audio_idma_write_to_dsp(p_dsp_buf, p_frm_buf, word_cnt);
    }
    {   /* DSP handshake */
        *DSP_TASK4_COSIM_HANDSHAKE = 0;
    }
    {   /* Callback functions */
        bt_media_handle_t *handle = (bt_media_handle_t *)p_info;
        handle->handler(handle, BT_CODEC_MEDIA_AWS_CHECK_UNDERFLOW);
        handle->handler(handle, BT_CODEC_MEDIA_AWS_CHECK_CLOCK_SKEW);
    }
    return;
}

static int32_t bt_codec_a2dp_aws_open_setting(bt_a2dp_audio_internal_handle_t *p_info)
{
    int32_t result = 0;
    bt_codec_a2dp_audio_t *p_codec = &p_info->codec_info;
    p_info->aws_flag = false;
    p_info->aws_internal_flag = false;
    p_info->clock_skew_status = BT_CODEC_AWS_CLOCK_SKEW_STATUS_IDLE;
    if (p_codec->role == BT_A2DP_SINK) {
        bt_codec_capability_t *p_cap = &p_codec->codec_cap;
        bt_a2dp_codec_type_t type = p_cap->type;
        uint16_t frm_len;
        ring_buffer_information_t *p_ring = &p_info->pcm_route_ring_buffer;
        bool valid_codec_type;
        if (type == BT_A2DP_CODEC_SBC) {
            frm_len = audio_service_get_frame_length(ASP_TYPE_SBC_DEC);
            valid_codec_type = true;
        }
#if defined(MTK_BT_A2DP_AAC_ENABLE)
        else if (type == BT_A2DP_CODEC_AAC) {
            frm_len = audio_service_get_frame_length(ASP_TYPE_AAC_DEC);
            valid_codec_type = true;
        }
#endif  /* defined(MTK_BT_A2DP_AAC_ENABLE) */
        else {  /* Invalid codec type */
            frm_len = 0;
            valid_codec_type = false;
        }
        if (valid_codec_type) {
            uint32_t frm_buf_size = frm_len * sizeof(uint16_t);
            uint32_t src_buf_size = frm_buf_size + AWS_CLOCK_SKEW_MAX_SAMPLE_COUNT * 2 * sizeof(uint16_t);
            uint32_t ring_buf_size = AWS_CLOCK_SKEW_RING_BUFFER_SAMPLE_COUNT * 2 * sizeof(uint16_t);
            p_ring->write_pointer = 0;
            p_ring->read_pointer = 0;
            p_ring->buffer_base_pointer = (uint8_t *)pvPortMalloc(ring_buf_size);
            p_ring->buffer_byte_count = ring_buf_size;
            p_info->pcm_route_src_buffer = (uint16_t *)pvPortMalloc(src_buf_size);
            p_info->pcm_route_src_size = src_buf_size;
            p_info->pcm_route_frame_buffer = (uint16_t *)pvPortMalloc(frm_buf_size);
            p_info->pcm_route_frame_size = frm_buf_size;
            if (p_ring->buffer_base_pointer == NULL || p_info->pcm_route_src_buffer == NULL || p_info->pcm_route_frame_buffer == NULL) {
                result = -1;
            }
        } else {
            p_ring->write_pointer = 0;
            p_ring->read_pointer = 0;
            p_ring->buffer_base_pointer = (uint8_t *)NULL;
            p_ring->buffer_byte_count = 0;
            p_info->pcm_route_src_buffer = (uint16_t *)NULL;
            p_info->pcm_route_src_size = 0;
            p_info->pcm_route_frame_buffer = (uint16_t *)NULL;
            p_info->pcm_route_frame_size = 0;
        }
    }
    return result;
}

static void bt_codec_a2dp_aws_close_setting(bt_a2dp_audio_internal_handle_t *p_info)
{
    uint16_t *p_frm_buf = p_info->pcm_route_frame_buffer;
    uint16_t *p_src_buf = p_info->pcm_route_src_buffer;
    ring_buffer_information_t *p_ring = &p_info->pcm_route_ring_buffer;
    uint8_t *p_base = p_ring->buffer_base_pointer;
    if (p_frm_buf != NULL) {
        vPortFree(p_frm_buf);
        p_info->pcm_route_frame_buffer = (uint16_t *)NULL;
    }
    if (p_src_buf != NULL) {
        vPortFree(p_src_buf);
        p_info->pcm_route_src_buffer = (uint16_t *)NULL;
    }
    if (p_base != NULL) {
        vPortFree(p_base);
        p_ring->buffer_base_pointer = (uint8_t *)NULL;
    }
    return;
}

static void bt_codec_a2dp_aws_play_setting(bt_a2dp_audio_internal_handle_t *p_info)
{
    p_info->accumulated_sample_count = 0;
    p_info->aws_internal_flag = true;
    audio_service_aws_set_flag(true);
    memset(&p_info->src_handle, 0, sizeof(bt_codec_aws_src_handle_t));
    {   /* Reset ring buffer */
        ring_buffer_information_t *p_ring = &p_info->pcm_route_ring_buffer;
        uint32_t ring_buf_size = AWS_CLOCK_SKEW_RING_BUFFER_SAMPLE_COUNT * 2 * sizeof(uint16_t);
        uint32_t sampling_rate = bt_codec_a2dp_aws_convert_sampling_rate_from_index_to_value(p_info->sample_rate);
        uint32_t pre_buf_smpl_cnt = AWS_CLOCK_SKEW_PREBUFFER_MILLISECOND_COUNT * sampling_rate / 1000;
        uint32_t pre_buf_size = pre_buf_smpl_cnt * 2 * sizeof(uint16_t);
        p_ring->write_pointer = 0;
        p_ring->read_pointer = 0;
        if (p_ring->buffer_base_pointer != NULL) {
            memset(p_ring->buffer_base_pointer, 0, ring_buf_size);
            ring_buffer_write_done(p_ring, pre_buf_size);
            p_info->prebuffer_sample_count = (int32_t)pre_buf_smpl_cnt;
        } else {
            p_info->prebuffer_sample_count = 0;
        }
    }
    {   /* PCM route setting */
        uint16_t mask    = AUDIO_DSP_POST_PROCESSING_ENABLE_MASK | AUDIO_DSP_POST_PROCESSING_PCM_ROUTE_MASK;
        uint16_t control = AUDIO_DSP_POST_PROCESSING_ENABLE_ON   | AUDIO_DSP_POST_PROCESSING_PCM_ROUTE_ON;
        audio_service_set_post_process_control(mask, control);
        audio_service_hook_isr(DSP_D2M_PCM_ROUTE_INT, bt_codec_a2dp_aws_pcm_route_isr, (void *)p_info);
    }
    return;
}

static void bt_codec_a2dp_aws_stop_setting(bt_a2dp_audio_internal_handle_t *p_info)
{
    {   /* PCM route setting */
        uint16_t mask    = AUDIO_DSP_POST_PROCESSING_ENABLE_MASK | AUDIO_DSP_POST_PROCESSING_PCM_ROUTE_MASK;
        uint16_t control = AUDIO_DSP_POST_PROCESSING_ENABLE_OFF  | AUDIO_DSP_POST_PROCESSING_PCM_ROUTE_OFF;
        audio_service_unhook_isr(DSP_D2M_PCM_ROUTE_INT);
        audio_service_set_post_process_control(mask, control);
    }
    audio_service_aws_set_flag(false);
    p_info->aws_internal_flag = false;
    return;
}
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */

static void bt_write_bs_to_dsp(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    bt_media_handle_t *handle = (bt_media_handle_t *)internal_handle;
    bt_a2dp_codec_type_t codec_type = internal_handle->codec_info.codec_cap.type;
    ring_buffer_information_t *p_ring = &internal_handle->ring_info;
    uint16_t bs_page;
    uint16_t bs_addr;
    uint16_t bs_size;
    uint16_t bs_wptr;
    uint16_t bs_rptr;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        bs_page = *DSP_SBC_DEC_DM_BS_PAGE;
        bs_addr = *DSP_SBC_DEC_DM_BS_ADDR;
        bs_size = *DSP_SBC_DEC_DM_BS_LEN;
        bs_wptr = *DSP_SBC_DEC_DM_BS_MCU_W_PTR;
        bs_rptr = *DSP_SBC_DEC_DM_BS_DSP_R_PTR;
        p_ring->write_pointer       = (uint32_t)bs_wptr;
        p_ring->read_pointer        = (uint32_t)bs_rptr;
        p_ring->buffer_byte_count   = (uint32_t)bs_size;
        p_ring->buffer_base_pointer = (uint8_t *)DSP_DM_ADDR(bs_page, bs_addr);
    }
#ifdef MTK_BT_A2DP_AAC_ENABLE
    else if (codec_type == BT_A2DP_CODEC_AAC) {
        bs_addr = *DSP_AAC_DEC_DM_BS_ADDR;
        bs_wptr = *DSP_AAC_DEC_DM_BS_MCU_W_PTR;    // in word
        bs_rptr = *DSP_AAC_DEC_DM_BS_DSP_R_PTR;    // in word
        p_ring->write_pointer       = (uint32_t)((bs_wptr - bs_addr) << 1); // in byte
        p_ring->read_pointer        = (uint32_t)((bs_rptr - bs_addr) << 1);
    }
#endif /*MTK_BT_A2DP_AAC_ENABLE*/
    {
        uint32_t loop_idx;
        uint32_t loop_cnt = 4;
        for (loop_idx = 0; loop_idx < loop_cnt; loop_idx++) {
            uint32_t           read_byte_cnt  = 0;
            uint32_t           write_byte_cnt = 0;
            uint32_t           move_byte_cnt = 0;
            uint8_t           *p_mcu_buf      = NULL;
            volatile uint16_t *p_dsp_buf      = NULL;

            if (codec_type == BT_A2DP_CODEC_SBC) {
                ring_buffer_get_write_information(p_ring, (uint8_t **)&p_dsp_buf, &write_byte_cnt);
            }
#ifdef MTK_BT_A2DP_AAC_ENABLE
            else if (codec_type == BT_A2DP_CODEC_AAC) {
                ring_buffer_get_write_information_non_mirroring(p_ring, (uint8_t **)&p_dsp_buf, &write_byte_cnt);
            }
#endif /*MTK_BT_A2DP_AAC_ENABLE*/

            write_byte_cnt &= ~0x1;     // Make it even

            if (handle->directly_access_dsp_function == NULL) {
                handle->get_read_buffer(handle, &p_mcu_buf, &read_byte_cnt);
                read_byte_cnt  &= ~0x1;     // Make it even

                move_byte_cnt = MINIMUM(write_byte_cnt, read_byte_cnt);
                {
                    // Move data
                    uint32_t move_word_cnt = move_byte_cnt >> 1;
                    if (move_word_cnt > 0) {
#if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG)
                        if (bt_a2dp_ptr + move_byte_cnt <= BT_A2DP_BS_LEN) {
                            memcpy(bt_a2dp_bitstream + bt_a2dp_ptr, p_mcu_buf, move_byte_cnt);
                            bt_a2dp_ptr += move_byte_cnt;
                        }
#endif /* BT_A2DP_BITSTREAM_DUMP_DEBUG */
                        audio_idma_write_to_dsp(p_dsp_buf, (uint16_t *)p_mcu_buf, move_word_cnt);
                    } else {    // Read buffer empty or write buffer full
                        break;
                    }
                }

                handle->read_data_done (handle, move_byte_cnt);
            } else {
                uint32_t write_word_cnt = write_byte_cnt >> 1;
                uint32_t wrote_word_cnt = 0;    // how many word written by directly_access_dsp_function
                wrote_word_cnt = handle->directly_access_dsp_function(p_dsp_buf, write_word_cnt);
                move_byte_cnt = wrote_word_cnt << 1;
                if (wrote_word_cnt == 0) {
                    break;
                }
            }

            if (codec_type == BT_A2DP_CODEC_SBC) {
                ring_buffer_write_done (p_ring, move_byte_cnt);
                *DSP_SBC_DEC_DM_BS_MCU_W_PTR = (uint16_t)p_ring->write_pointer;
            }
#ifdef MTK_BT_A2DP_AAC_ENABLE
            else if (codec_type == BT_A2DP_CODEC_AAC) {
                ring_buffer_write_done_non_mirroring(p_ring, move_byte_cnt);
                *DSP_AAC_DEC_DM_BS_MCU_W_PTR = (uint16_t)(p_ring->write_pointer >> 1) + bs_addr;
            }
#endif /*MTK_BT_A2DP_AAC_ENABLE*/
        }
    }
    return;
}

static void sbc_decoder_isr_handler(void *data)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)data;
    bt_media_handle_t *handle = (bt_media_handle_t *)internal_handle;
#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_I("[A2DP][SBC]ISR");
#endif
    if ((GET_DSP_VALUE(RG_SBC_DEC_FSM) == DSP_SBC_STATE_IDLE) || (handle->state == BT_CODEC_STATE_ERROR)) {
        return;
    }

    /* error handling */
    if (GET_DSP_VALUE(RG_SBC_DEC_FSM) == DSP_SBC_STATE_RUNNING) {
        if (   (GET_DSP_VALUE(RG_SBC_DEC_STATUS) == DSP_SBC_DEC_SYNC_ERR)
                || (GET_DSP_VALUE(RG_SBC_DEC_STATUS) == DSP_SBC_DEC_CRC_ERR)
                || (GET_DSP_VALUE(RG_SBC_DEC_STATUS) == DSP_SBC_DEC_BITPOOL_ERR)
                || (GET_DSP_VALUE(RG_SBC_PAR_STATUS) == DSP_SBC_PAR_MAGIC_WORD_ERR)
                || (GET_DSP_VALUE(RG_SBC_PAR_STATUS) == DSP_SBC_PAR_INVALID_NON_FRAGMENTED_PAYLOAD)
                || (GET_DSP_VALUE(RG_SBC_PAR_STATUS) == DSP_SBC_PAR_START_PAYLOAD_ERR)
                || (GET_DSP_VALUE(RG_SBC_PAR_STATUS) == DSP_SBC_PAR_INVALID_FRAGMENT_INDEX_1)
                || (GET_DSP_VALUE(RG_SBC_PAR_STATUS) == DSP_SBC_PAR_INVALID_FRAGMENT_INDEX_2)
                || (GET_DSP_VALUE(RG_SBC_PAR_STATUS) == DSP_SBC_PAR_INVALID_FRAME_NUMBER)
           ) {
            LISR_LOG_E("[A2DP][SBC]DECODER ERR, PAR:%d  DEC=%d\n", GET_DSP_VALUE(RG_SBC_PAR_STATUS), GET_DSP_VALUE(RG_SBC_DEC_STATUS));
            handle->handler(handle, BT_CODEC_MEDIA_ERROR);
            handle->state = BT_CODEC_STATE_ERROR;
            return;
        }
    }
    internal_handle->frame_count++;
    /* fill bitstream */
    bt_write_bs_to_dsp(internal_handle);
    if (!handle->waiting) {
        handle->waiting = true;
        if ((!handle->underflow) && (GET_DSP_VALUE(RG_SBC_DEC_STATUS) == DSP_SBC_DEC_BS_UNDERFLOW)) {
            handle->underflow = true;
            LISR_LOG_I("[A2DP][SBC] DSP underflow \n");
            handle->handler(handle, BT_CODEC_MEDIA_UNDERFLOW);
        } else {
            handle->handler(handle, BT_CODEC_MEDIA_REQUEST);
        }
    }
}

static bt_codec_media_status_t bt_open_sink_sbc_codec(bt_media_handle_t *handle)
{
    uint16_t I = 0;
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;

    TASK_LOG_CTRL("[SBC]open_codec\r\n");
    audio_service_hook_isr(DSP_D2C_SBC_DEC_INT, sbc_decoder_isr_handler, internal_handle);
    audio_service_setflag(handle->audio_id);
    if (GET_DSP_VALUE(RG_SBC_DEC_FSM) != DSP_SBC_STATE_IDLE) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
#if defined(AWS_DEBUG_CODE)
    TASK_LOG_I("[AWS] set AWS flag\r\n");
    bt_codec_a2dp_aws_set_flag(handle, true);
#endif  /* defined(AWS_DEBUG_CODE) */
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    if (internal_handle->aws_flag) {
        bt_codec_a2dp_aws_play_setting(internal_handle);
    }
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    internal_handle->frame_count = 0;
    *DSP_AUDIO_CTRL2 |= DSP_PCM_R_DIS;
    *DSP_AUDIO_FLEXI_CTRL |= (FLEXI_VBI_ENABLE | FLEXI_SD_ENABLE);

    *DSP_SBC_PAR_MAGIC_WORD = 0x3453;
    SET_DSP_VALUE(RG_SBC_PARSER_EN, 1);
    SET_DSP_VALUE(RG_SBC_DEC_FSM, DSP_SBC_STATE_START);
    *DSP_SBC_DEC_DM_BS_MCU_W_PTR = 0;
    dsp_audio_fw_dynamic_download(DDID_SBC_DECODE);
    afe_set_path_type(HAL_AUDIO_PLAYBACK_MUSIC);
    audio_playback_on(ASP_TYPE_SBC_DEC, internal_handle->sample_rate);
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    if (internal_handle->aws_internal_flag == false)
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    {
        for (I = 0; ; I++) {
            if (GET_DSP_VALUE(RG_SBC_DEC_FSM) == DSP_SBC_STATE_RUNNING) {
                break;
            }
            if (I > 80) {
                return BT_CODEC_MEDIA_STATUS_ERROR;
            }
            hal_gpt_delay_ms(9);
        }
    }
#if defined(AWS_DEBUG_CODE)
    else {  /* Manually reset DSP for unit test */
        bt_codec_a2dp_aws_set_initial_sync(handle);
        for (I = 0; ; I++) {
            if (GET_DSP_VALUE(RG_SBC_DEC_FSM) == DSP_SBC_STATE_RUNNING) {
                break;
            }
            if (I > 80) {
                return BT_CODEC_MEDIA_STATUS_ERROR;
            }
            hal_gpt_delay_ms(9);
        }
    }
#endif  /* defined(AWS_DEBUG_CODE) */
    handle->state = BT_CODEC_STATE_PLAY;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_close_sink_sbc_codec(bt_media_handle_t *handle)
{
    uint16_t I = 0;
    TASK_LOG_CTRL("[SBC]close_codec\r\n");
    for (I = 0; ; I++) {
        if (GET_DSP_VALUE(RG_SBC_DEC_FSM) == DSP_SBC_STATE_IDLE) {
            break;
        }
        if (GET_DSP_VALUE(RG_SBC_DEC_FSM) == DSP_SBC_STATE_RUNNING) {
            SET_DSP_VALUE(RG_SBC_DEC_FSM, DSP_SBC_STATE_FLUSH);
        }
        if (I > 80) {
            return BT_CODEC_MEDIA_STATUS_ERROR;
        }
        hal_gpt_delay_ms(9);
    }
    SET_DSP_VALUE(RG_SBC_PARSER_EN, 0);

    *DSP_AUDIO_CTRL2 &= ~DSP_PCM_R_DIS;
    *DSP_AUDIO_FLEXI_CTRL &= ~(FLEXI_VBI_ENABLE | FLEXI_SD_ENABLE);

    audio_playback_off();
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    {
        bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;
        if (internal_handle->aws_internal_flag) {
            bt_codec_a2dp_aws_stop_setting(internal_handle);
        }
    }
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
#if defined(AWS_DEBUG_CODE)
    TASK_LOG_I("[AWS] clear AWS flag\r\n");
    bt_codec_a2dp_aws_set_flag(handle, false);
#endif  /* defined(AWS_DEBUG_CODE) */
    audio_service_unhook_isr(DSP_D2C_SBC_DEC_INT);
    audio_service_clearflag(handle->audio_id);
    handle->state = BT_CODEC_STATE_STOP;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_sink_sbc_play(bt_media_handle_t *handle)
{
    TASK_LOG_CTRL("[SBC]play\r\n");
    if (handle->state != BT_CODEC_STATE_READY && handle->state != BT_CODEC_STATE_STOP) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
#if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG)
    bt_a2dp_ptr = 0;
    memset(bt_a2dp_bitstream, 0, BT_A2DP_BS_LEN * sizeof(uint8_t));
#endif /* BT_A2DP_BITSTREAM_DUMP_DEBUG */
    return bt_open_sink_sbc_codec(handle);
}

static bt_codec_media_status_t bt_a2dp_sink_sbc_stop(bt_media_handle_t *handle)
{
    TASK_LOG_CTRL("[SBC]stop--state: %d\r\n", handle->state);
    if (handle->state == BT_CODEC_STATE_READY) {
        handle->state = BT_CODEC_STATE_STOP;
        return BT_CODEC_MEDIA_STATUS_OK;
    }

    if (handle->state != BT_CODEC_STATE_PLAY && handle->state != BT_CODEC_STATE_ERROR) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return bt_close_sink_sbc_codec(handle);
}

static bt_codec_media_status_t bt_a2dp_sink_sbc_process(bt_media_handle_t *handle, bt_codec_media_event_t event)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    if (internal_handle == NULL) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_sink_parse_sbc_info(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    uint8_t channel_mode, sample_rate;
    bt_codec_a2dp_audio_t *pParam = (bt_codec_a2dp_audio_t *)&internal_handle->codec_info;

    channel_mode = pParam->codec_cap.codec.sbc.channel_mode;
    sample_rate  = pParam->codec_cap.codec.sbc.sample_rate;
    TASK_LOG_I("[A2DP][SBC]sample rate=%d, channel=%d \n", sample_rate, channel_mode);
    switch (channel_mode) {
        case 8:
            internal_handle->channel_number = 1;
            break;
        case 4:
        case 2:
        case 1:
            internal_handle->channel_number = 2;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }

    switch (sample_rate) {
        case 8:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_16KHZ;
            break;
        case 4:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_32KHZ;
            break;
        case 2:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_44_1KHZ;
            break;
        case 1:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_48KHZ;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
}

#ifdef MTK_BT_A2DP_AAC_ENABLE
/* aac */
static void aac_get_silence_pattern(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    uint8_t channel_number, frequence_index;
    uint16_t sample_rate;
    uint8_t *silence_frame = internal_handle->aac_silence_pattern;

    channel_number = internal_handle->channel_number;
    sample_rate  = internal_handle->sample_rate;

    memcpy(silence_frame, AAC_ADTS_HEADER, ADTS_HEADER_LENGTH);

    switch (sample_rate) {
        case HAL_AUDIO_SAMPLING_RATE_8KHZ:
            frequence_index = 11;
            break;
        case HAL_AUDIO_SAMPLING_RATE_11_025KHZ:
            frequence_index = 10;
            break;
        case HAL_AUDIO_SAMPLING_RATE_12KHZ:
            frequence_index = 9;
            break;
        case HAL_AUDIO_SAMPLING_RATE_16KHZ:
            frequence_index = 8;
            break;
        case HAL_AUDIO_SAMPLING_RATE_22_05KHZ:
            frequence_index = 7;
            break;
        case HAL_AUDIO_SAMPLING_RATE_24KHZ:
            frequence_index = 6;
            break;
        case HAL_AUDIO_SAMPLING_RATE_32KHZ:
            frequence_index = 5;
            break;
        case HAL_AUDIO_SAMPLING_RATE_44_1KHZ:
            frequence_index = 4;
            break;
        case HAL_AUDIO_SAMPLING_RATE_48KHZ:
            frequence_index = 3;
            break;
        default:
            frequence_index = 0;
            break;
    }
    /* fill AAC silence data */
    if(channel_number == 1) {
        memcpy(silence_frame+ADTS_HEADER_LENGTH, aac_silence_data_mono, SILENCE_DATA_LENGTH);
    } else {
        memcpy(silence_frame+ADTS_HEADER_LENGTH, aac_silence_data_stereo, SILENCE_DATA_LENGTH);
    }
    /* update AAC ADTS header */
    silence_frame[2] |= (frequence_index << 2);
    silence_frame[3] |= ((channel_number) << 6) | ((SILENCE_TOTAL_LENGTH) >> 11);
    silence_frame[4] = ((SILENCE_TOTAL_LENGTH) >> 3) & 0xFF;
    silence_frame[5] |= ((SILENCE_TOTAL_LENGTH) & 0x07) << 5;

}

static void aac_write_silence(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    ring_buffer_information_t *p_ring = &internal_handle->ring_info;
    uint16_t bs_addr;
    uint16_t bs_wptr;
    uint16_t bs_rptr;

    bs_addr = *DSP_AAC_DEC_DM_BS_ADDR;
    bs_wptr = *DSP_AAC_DEC_DM_BS_MCU_W_PTR;    // in word
    bs_rptr = *DSP_AAC_DEC_DM_BS_DSP_R_PTR;    // in word
    p_ring->write_pointer       = (uint32_t)((bs_wptr - bs_addr) << 1); // in byte
    p_ring->read_pointer        = (uint32_t)((bs_rptr - bs_addr) << 1);

    {
        uint32_t loop_idx;
        uint32_t loop_cnt = 2;
        int32_t read_byte_cnt = SILENCE_TOTAL_LENGTH;
        for (loop_idx = 0; loop_idx < loop_cnt; loop_idx++) {
            uint32_t           write_byte_cnt = 0;
            uint32_t           move_byte_cnt;
            uint8_t            *p_mcu_buf      = internal_handle->aac_silence_pattern;
            volatile uint16_t  *p_dsp_buf      = NULL;

            p_mcu_buf += (SILENCE_TOTAL_LENGTH - read_byte_cnt);
            ring_buffer_get_write_information_non_mirroring(p_ring, (uint8_t **)&p_dsp_buf, &write_byte_cnt);

            write_byte_cnt &= ~0x1;     // Make it even
            move_byte_cnt = MINIMUM(write_byte_cnt, read_byte_cnt);
            {
                // Move data
                uint32_t move_word_cnt = move_byte_cnt >> 1;
                if (move_word_cnt > 0) {
                    audio_idma_write_to_dsp(p_dsp_buf, (uint16_t *)p_mcu_buf, move_word_cnt);
                    read_byte_cnt -= (move_word_cnt << 1);
                } else {    // Read buffer empty or write buffer full
                    break;
                }
            }
            ring_buffer_write_done_non_mirroring(p_ring, move_byte_cnt);
            *DSP_AAC_DEC_DM_BS_MCU_W_PTR = (uint16_t)(p_ring->write_pointer >> 1) + bs_addr;
        }
    }
    return;
}

static void aac_decoder_isr_handler(void *data)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)data;
    bt_media_handle_t *handle = (bt_media_handle_t *)internal_handle;
    
#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_I("[A2DP[AAC]ISR");
#endif

    *DSP_TASK4_COSIM_HANDSHAKE = 0;
    if ((*DSP_AAC_DEC_FSM == DSP_AAC_STATE_IDLE) || (handle->state == BT_CODEC_STATE_ERROR)) {
        return;
    }

    /* error handling, but bypass the buffer underflow warning from DSP */
    if ((*DSP_AAC_DEC_ERROR_REPORT != DSP_AAC_REPORT_NONE)
            && (*DSP_AAC_DEC_ERROR_REPORT != DSP_AAC_REPORT_UNDERFLOW)) {
      internal_handle->error_count ++;

        /* fill silence when underflow continuously */
        if (*DSP_AAC_DEC_ERROR_REPORT == DSP_AAC_REPORT_BUFFER_NOT_ENOUGH) {

            int32_t mcu_data_count;
            if (handle->get_data_count_function) {
                mcu_data_count = handle->get_data_count_function();
            } else {
                mcu_data_count = handle->get_data_count(handle);
            }

            if(mcu_data_count < AAC_FILL_SILENCE_TRHESHOLD) {
                aac_write_silence(internal_handle);
            }
        } else {
            LISR_LOG_E("[A2DP]DECODER ERR(%x), FSM:%x REPORT=%x\n", (unsigned int)internal_handle->error_count, *DSP_AAC_DEC_FSM, *DSP_AAC_DEC_ERROR_REPORT);
        }

        LISR_LOG_E("[A2DP][AAC]DECODER ERR, FSM:%x  REPORT=%x\n", *DSP_AAC_DEC_FSM, *DSP_AAC_DEC_ERROR_REPORT);
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
        if (internal_handle->aws_internal_flag) {
            LISR_LOG_E("[A2DP][AAC]DECODER ERR AWS\n");
            *DSP_AAC_DEC_FSM = DSP_AAC_STATE_STOP;
            handle->state = BT_CODEC_STATE_ERROR;
            handle->handler(handle, BT_CODEC_MEDIA_ERROR);
            return;
        }
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
        if (internal_handle->error_count >= AAC_ERROR_FRAME_THRESHOLD) {
            LISR_LOG_E("[A2DP][AAC]DECODER ERR OVER THRESHOLD\n");
            *DSP_AAC_DEC_FSM = DSP_AAC_STATE_STOP;
            handle->state = BT_CODEC_STATE_ERROR;
            handle->handler(handle, BT_CODEC_MEDIA_ERROR);
            return;
        }
    } else { //if error is not consecutive, reset to 0
        internal_handle->error_count = 0;
    }

    /* bitstream buffer initialization */
    if (!internal_handle->ring_info.buffer_base_pointer) {
        uint16_t bs_page = DSP_AAC_PAGE_NUM;
        uint16_t bs_addr = *DSP_AAC_DEC_DM_BS_ADDR;
        uint16_t bs_size = *DSP_AAC_DEC_DM_BS_LEN << 1;
        internal_handle->ring_info.buffer_byte_count = (uint32_t)bs_size;
        internal_handle->ring_info.buffer_base_pointer = (uint8_t *)DSP_DM_ADDR(bs_page, bs_addr);
    }
    internal_handle->frame_count++;
    if (internal_handle->frame_count == 1) {
        uint16_t bs_addr = *DSP_AAC_DEC_DM_BS_ADDR;
        *DSP_AAC_DEC_DM_BS_MCU_W_PTR = bs_addr;
    }
    /* fill bitstream */
    bt_write_bs_to_dsp(internal_handle);
    if (!handle->waiting) {
        handle->waiting = true;
        if ((!handle->underflow) && (*DSP_AAC_DEC_ERROR_REPORT == DSP_AAC_REPORT_UNDERFLOW)) {
            handle->underflow = true;
            LISR_LOG_I("[A2DP][AAC]DSP underflow \n");
            handle->handler(handle, BT_CODEC_MEDIA_UNDERFLOW);
        } else {
            handle->handler(handle, BT_CODEC_MEDIA_REQUEST);
        }
    }
}

static bt_codec_media_status_t bt_open_sink_aac_codec(bt_media_handle_t *handle)
{
    uint16_t I = 0;
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;

    TASK_LOG_CTRL("[AAC]open_codec\r\n");
    internal_handle->error_count = 0;

    audio_service_hook_isr(DSP_D2C_AAC_DEC_INT, aac_decoder_isr_handler, internal_handle);
    audio_service_setflag(handle->audio_id);
    *DSP_AUDIO_ASP_COMMON_FLAG_1 = 0;    //clear dsp audio common flag

    *DSP_AAC_DEC_ALLERROR_REPORT = 0;
    *DSP_AAC_DEC_DUAL_SCE = 0;

    if (*DSP_AAC_DEC_FSM != DSP_AAC_STATE_IDLE) {
        TASK_LOG_E("[A2DP]AAC OPEN STATE ERROR(%x)\n", *DSP_AAC_DEC_FSM);
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
#if defined(AWS_DEBUG_CODE)
    TASK_LOG_I("[AWS] set AWS flag\r\n");
    bt_codec_a2dp_aws_set_flag(handle, true);
#endif  /* defined(AWS_DEBUG_CODE) */
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    if (internal_handle->aws_flag) {
        bt_codec_a2dp_aws_play_setting(internal_handle);
    }
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    internal_handle->frame_count = 0;
    *DSP_AAC_DEC_FSM = DSP_AAC_STATE_START;

    *DSP_AUDIO_CTRL2 |= (DSP_AAC_CTRL_ASP | DSP_PCM_R_DIS);
    *DSP_AUDIO_FLEXI_CTRL |= (FLEXI_VBI_ENABLE | FLEXI_SD_ENABLE);
    afe_set_path_type(HAL_AUDIO_PLAYBACK_MUSIC);
    dsp_audio_fw_dynamic_download(DDID_AAC);

    aac_get_silence_pattern(internal_handle);
    audio_playback_on(ASP_TYPE_AAC_DEC, internal_handle->sample_rate);

#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    if (internal_handle->aws_internal_flag == false)
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    {
        for (I = 0; ; I++) {
            if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_PLAYING) {
                break;
            }
            /* This is the case when AAC codec has started and encoutered an error,
               aacPlaybackHisr found this and set the state to STOP, then AAC codec
               set the state to IDLE. */
            if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_IDLE) {
                TASK_LOG_E("[A2DP]ERROR when AAC CODEC STARTS \n");
                break;
            }
            if (I > 80) {
                TASK_LOG_E("[A2DP][AAC] CODEC OPEN ERROR\n");
                return BT_CODEC_MEDIA_STATUS_ERROR;
            }
            hal_gpt_delay_ms(9);
        }
    }
#if defined(AWS_DEBUG_CODE)
    else {  /* Manually reset DSP for unit test */
        bt_codec_a2dp_aws_set_initial_sync(handle);
        for (I = 0; ; I++) {
            if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_PLAYING) {
                break;
            }
            /* This is the case when AAC codec has started and encoutered an error,
               aacPlaybackHisr found this and set the state to STOP, then AAC codec
               set the state to IDLE. */
            if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_IDLE) {
                TASK_LOG_E("[A2DP]ERROR when AAC CODEC STARTS \n");
                break;
            }
            if (I > 80) {
                TASK_LOG_E("[A2DP][AAC] CODEC OPEN ERROR\n");
                return BT_CODEC_MEDIA_STATUS_ERROR;
            }
            hal_gpt_delay_ms(9);
        }
    }
#endif  /* defined(AWS_DEBUG_CODE) */
    handle->state = BT_CODEC_STATE_PLAY;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_close_sink_aac_codec(bt_media_handle_t *handle)
{
    uint16_t I = 0;
    TASK_LOG_CTRL("[AAC]close_codec\r\n");
    for (I = 0; ; I++) {
        if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_IDLE) {
            break;
        }
        if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_PLAYING) {
            *DSP_AAC_DEC_FSM = DSP_AAC_STATE_STOP;
        }
        if (I > 80) {
            return BT_CODEC_MEDIA_STATUS_ERROR;
        }
        hal_gpt_delay_ms(9);
    }

    *DSP_AUDIO_CTRL2 &= ~(DSP_AAC_CTRL_ASP | DSP_PCM_R_DIS);
    *DSP_AUDIO_FLEXI_CTRL &= ~(FLEXI_VBI_ENABLE | FLEXI_SD_ENABLE);

    audio_playback_off();
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    {
        bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;
        if (internal_handle->aws_internal_flag) {
            bt_codec_a2dp_aws_stop_setting(internal_handle);
        }
    }
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
#if defined(AWS_DEBUG_CODE)
    TASK_LOG_I("[AWS] clear AWS flag\r\n");
    bt_codec_a2dp_aws_set_flag(handle, false);
#endif  /* defined(AWS_DEBUG_CODE) */
    audio_service_unhook_isr(DSP_D2C_AAC_DEC_INT);
    audio_service_clearflag(handle->audio_id);
    handle->state = BT_CODEC_STATE_STOP;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_sink_aac_play(bt_media_handle_t *handle)
{
    TASK_LOG_CTRL("[AAC]play\r\n");
    if (handle->state != BT_CODEC_STATE_READY && handle->state != BT_CODEC_STATE_STOP) {
        TASK_LOG_E("[A2DP][AAC] CODEC PLAY ERROR \n");
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return bt_open_sink_aac_codec(handle);
}

static bt_codec_media_status_t bt_a2dp_sink_aac_stop(bt_media_handle_t *handle)
{
    TASK_LOG_CTRL("[AAC]stop--state: %d\r\n", handle->state);
    if (handle->state == BT_CODEC_STATE_READY) {
        handle->state = BT_CODEC_STATE_STOP;
        return BT_CODEC_MEDIA_STATUS_OK;
    }

    if (handle->state != BT_CODEC_STATE_PLAY && handle->state != BT_CODEC_STATE_ERROR) {
        TASK_LOG_E("[A2DP][AAC] CODEC STOP ERROR \n");
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return bt_close_sink_aac_codec(handle);
}

static bt_codec_media_status_t bt_a2dp_sink_aac_process(bt_media_handle_t *handle, bt_codec_media_event_t event)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    if (internal_handle == NULL) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_sink_parse_aac_info(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    uint8_t channel_mode;
    uint16_t sample_rate;
    bt_codec_a2dp_audio_t *pParam = (bt_codec_a2dp_audio_t *)&internal_handle->codec_info;

    channel_mode = pParam->codec_cap.codec.aac.channels;
    sample_rate  = pParam->codec_cap.codec.aac.sample_rate;
    TASK_LOG_I("[A2DP][AAC] sample rate=%x, channel=%x \n", sample_rate, channel_mode);

    switch (channel_mode) {
        case 0x2:
            internal_handle->channel_number = 1;
            break;
        case 0x1:
            internal_handle->channel_number = 2;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    switch (sample_rate) {
        case 0x800:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_8KHZ;
            break;
        case 0x400:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_11_025KHZ;
            break;
        case 0x200:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_12KHZ;
            break;
        case 0x100:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_16KHZ;
            break;
        case 0x80:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_22_05KHZ;
            break;
        case 0x40:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_24KHZ;
            break;
        case 0x20:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_32KHZ;
            break;
        case 0x10:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_44_1KHZ;
            break;
        case 0x8:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_48KHZ;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
}
#endif /*MTK_BT_A2DP_AAC_ENABLE*/
bt_media_handle_t *bt_codec_a2dp_open(bt_codec_a2dp_callback_t bt_a2dp_callback , const bt_codec_a2dp_audio_t *param)
{
    bt_media_handle_t *handle;
    bt_a2dp_audio_internal_handle_t *internal_handle; /*internal handler*/
    uint16_t audio_id = audio_get_id();
    TASK_LOG_I("[A2DP]Open codec\n");
    if (audio_id > MAX_AUDIO_FUNCTIONS) {
        return 0;
    }
#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_I("[A2DP]Open codec--role: %d, type: %d, 1: %d, 2: %d, 3: %d, 4: %d, 5: %d, 6: %d, 7: %d\n",
        param->role, param->codec_cap.type,
        param->codec_cap.codec.sbc.alloc_method,
        param->codec_cap.codec.sbc.block_length,
        param->codec_cap.codec.sbc.channel_mode,
        param->codec_cap.codec.sbc.max_bit_pool,
        param->codec_cap.codec.sbc.min_bit_pool,
        param->codec_cap.codec.sbc.sample_rate,
        param->codec_cap.codec.sbc.subband_num);
#endif
    /* alloc internal handle space */
    internal_handle = (bt_a2dp_audio_internal_handle_t *)pvPortMalloc(sizeof(bt_a2dp_audio_internal_handle_t));
    memset(internal_handle, 0, sizeof(bt_a2dp_audio_internal_handle_t));
    handle = &internal_handle->handle;
    internal_handle->codec_info = *(bt_codec_a2dp_audio_t *)param;
    handle->audio_id = audio_id;
    handle->handler = bt_a2dp_callback;
    handle->directly_access_dsp_function = NULL;
    handle->get_data_count_function = NULL;
    bt_codec_buffer_function_init(handle);
    if (internal_handle->codec_info.role == BT_A2DP_SINK) {
        if (internal_handle->codec_info.codec_cap.type == BT_A2DP_CODEC_SBC) {
            bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
            handle->play    = bt_a2dp_sink_sbc_play;
            handle->stop    = bt_a2dp_sink_sbc_stop;
            handle->process = bt_a2dp_sink_sbc_process;
            result = bt_a2dp_sink_parse_sbc_info(internal_handle);
            if (BT_CODEC_MEDIA_STATUS_OK != result) {
                return 0;
            }
            #ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
            TASK_LOG_I("[A2DP][SBC]Codec open");
            #endif
        }
#ifdef MTK_BT_A2DP_AAC_ENABLE
        else if (internal_handle->codec_info.codec_cap.type == BT_A2DP_CODEC_AAC) {
            bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
            handle->play    = bt_a2dp_sink_aac_play;
            handle->stop    = bt_a2dp_sink_aac_stop;
            handle->process = bt_a2dp_sink_aac_process;
            result = bt_a2dp_sink_parse_aac_info(internal_handle);
            if (BT_CODEC_MEDIA_STATUS_OK != result) {
                return 0;
            }
            #ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
            TASK_LOG_I("[A2DP[AAC]Codec open");
            #endif
        }
#endif /*MTK_BT_A2DP_AAC_ENABLE*/
        else {
            return 0;
        }
    } else {
        /* A2DP source role */
    }
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    {
        int32_t result = bt_codec_a2dp_aws_open_setting(internal_handle);
        if (result < 0) {
            TASK_LOG_I("[A2DP][AWS]alloc fail, result = %d\r\n", (int)result);
            return 0;
        }
    }
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    handle->state = BT_CODEC_STATE_READY;
    return handle;
}

bt_codec_media_status_t bt_codec_a2dp_close(bt_media_handle_t *handle)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    TASK_LOG_I("[A2DP]Close codec\n");
    if (handle->state != BT_CODEC_STATE_STOP && handle->state != BT_CODEC_STATE_READY) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    handle->state = BT_CODEC_STATE_IDLE;
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_codec_a2dp_aws_close_setting(internal_handle);
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    audio_free_id(handle->audio_id);
    vPortFree(internal_handle);
    return BT_CODEC_MEDIA_STATUS_OK;
}

bt_codec_media_status_t bt_codec_a2dp_aws_set_flag(bt_media_handle_t *handle, bool flag)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    p_info->aws_flag = flag;
    return BT_CODEC_MEDIA_STATUS_OK;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_set_initial_sync(bt_media_handle_t *handle)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    if (handle != NULL && p_info->aws_internal_flag) {
        audio_service_aws_set_initial_sync();
    } else {
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return result;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_get_consumed_data_count(bt_media_handle_t *handle, bt_codec_a2dp_data_count_t *information)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    information->sample_count  = p_info->accumulated_sample_count;
    information->sampling_rate = bt_codec_a2dp_aws_convert_sampling_rate_from_index_to_value(p_info->sample_rate);
    return BT_CODEC_MEDIA_STATUS_OK;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_get_silence_frame_information(bt_media_handle_t *handle, bt_codec_a2dp_bitstream_t *information)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    bt_a2dp_codec_type_t codec_type = p_info->codec_info.codec_cap.type;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        information->sample_count = 128;
        information->byte_count = 10;
    } else if (codec_type == BT_A2DP_CODEC_AAC) {
#ifdef MTK_BT_A2DP_AAC_ENABLE
        information->sample_count = 1024;
        information->byte_count = 128;
#else   /* MTK_BT_A2DP_AAC_ENABLE */
        information->sample_count = 0;
        information->byte_count = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* MTK_BT_A2DP_AAC_ENABLE */
    } else {
        information->sample_count = 0;
        information->byte_count = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return result;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
static const uint8_t sbc_silence_pattern[10] = {0x53, 0x34, 0x06, 0x00, 0x01, 0x9C, 0x33, 0x00, 0x77, 0x80};
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */

bt_codec_media_status_t bt_codec_a2dp_aws_fill_silence_frame(bt_media_handle_t *handle, bt_codec_a2dp_buffer_t *data, uint32_t target_frm_cnt)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    bt_a2dp_codec_type_t codec_type = p_info->codec_info.codec_cap.type;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        uint32_t byte_count = data->byte_count;
        uint32_t available_frm_cnt = byte_count / 10;
        if (available_frm_cnt < target_frm_cnt) {
            result = BT_CODEC_MEDIA_STATUS_ERROR;
        } else {
            uint32_t frm_idx;
            uint8_t *p_src = (uint8_t *)sbc_silence_pattern;
            uint8_t *p_dst = data->buffer;
            for (frm_idx = 0; frm_idx < target_frm_cnt; frm_idx++) {
                memcpy(p_dst, p_src, 10);
                p_dst += 10;
            }
        }
    } else if (codec_type == BT_A2DP_CODEC_AAC) {
#ifdef MTK_BT_A2DP_AAC_ENABLE
        uint32_t byte_count = data->byte_count;
        uint32_t available_frm_cnt = byte_count / 128;
        if (available_frm_cnt < target_frm_cnt) {
            result = BT_CODEC_MEDIA_STATUS_ERROR;
        } else {
            uint32_t frm_idx;
            uint8_t *p_src = (uint8_t *)p_info->aac_silence_pattern;
            uint8_t *p_dst = data->buffer;
            aac_get_silence_pattern(p_info);
            for (frm_idx = 0; frm_idx < target_frm_cnt; frm_idx++) {
                memset(p_dst, 0, 128);
                memcpy(p_dst, p_src, 16);
                p_dst += 128;
            }
        }
#else   /* MTK_BT_A2DP_AAC_ENABLE */
        result = BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* MTK_BT_A2DP_AAC_ENABLE */
    } else {
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return result;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
static int32_t bt_codec_a2dp_aws_sbc_get_media_payload_sample_count (uint8_t *buffer, uint32_t *p_smpl_cnt, uint32_t *p_byte_cnt)
{
    int32_t result = -1;
    uint32_t smpl_cnt = 0;
    uint32_t available_byte_cnt = *p_byte_cnt;
    uint32_t consumed_byte_cnt = 0;
    if (available_byte_cnt > 0) { /* Skip 1 byte packet header */
        buffer++;
        consumed_byte_cnt++;
        available_byte_cnt--;
    }
    while (available_byte_cnt >= 5) {
        uint8_t header[5];
        memcpy(header, buffer, 5);
        result = 0;
        if (header[0] != 0x9C) {    /* Invalid sync byte */
            result = -2;
            break;
        } else {
            uint32_t setting = (uint32_t)header[1];
            uint32_t bitpool = (uint32_t)header[2];
            uint32_t consumed_len = 0;
            if (bitpool == 0 && setting == 0x33 && header[3] == 0x77) { /* Silence frame, 5 bytes */
                if (header[4] > 128) {  /* Invalid silence sample count */
                    result = -3;
                    break;
                } else {
                    consumed_len = 5;
                    smpl_cnt += header[4];
                }
            } else if (bitpool >= 2 && bitpool <= 250) {
                uint32_t equation_type;
                uint32_t nrof_channels;
                uint32_t join;
                uint32_t nrof_subbands;
                uint32_t nrof_blocks;
                {   /* Channel mode : equation_type, nrof_channels, join */
                    uint32_t channel_mode = (setting >> 2) & 0x03;
                    if (channel_mode == 0x00) { /* MONO */
                        equation_type = 0;
                        nrof_channels = 1;
                        join = 0;
                    } else if (channel_mode == 0x01) { /* DUAL_CHANNEL */
                        equation_type = 0;
                        nrof_channels = 2;
                        join = 0;
                    } else if (channel_mode == 0x02) { /* STEREO */
                        equation_type = 1;
                        nrof_channels = 2;
                        join = 0;
                    } else { /* JOINT_STEREO */
                        equation_type = 1;
                        nrof_channels = 2;
                        join = 1;
                    }
                }
                nrof_subbands = (setting & 0x01) == 0 ? 4 : 8;
                {   /* Blocks : nrof_blocks */
                    uint32_t blocks = (setting >> 4) & 0x03;
                    if (blocks == 0) {
                        nrof_blocks = 4;
                    } else if (blocks == 1) {
                        nrof_blocks = 8;
                    } else if (blocks == 2) {
                        nrof_blocks = 12;
                    } else {
                        nrof_blocks = 16;
                    }
                }
                {   /* Calculate frame length & sample count */
                    uint32_t tmp;
                    if (equation_type == 0) {
                        /* channel_mode == MONO || channel_mode == DUAL_CHANNEL */
                        /* frame_length = 4 + (4 * nrof_subbands * nrof_channels) / 8 + ceiling(nrof_blocks * nrof_channels * bitpool / 8) */
                        tmp = nrof_blocks * nrof_channels * bitpool;
                    } else {
                        /* channel_mode == STEREO || channel_mode == JOINT_STEREO */
                        /* frame_length = 4 + (4 * nrof_subbands * nrof_channels) / 8 + ceiling((join * nrof_subbands + nrof_blocks * bitpool) / 8) */
                        tmp = join * nrof_subbands + nrof_blocks * bitpool;
                    }
                    consumed_len = tmp >> 3;
                    if ((consumed_len << 3) < tmp) {
                        consumed_len++;
                    }
                    consumed_len += 4 + (nrof_subbands * nrof_channels >> 1);
                    if (consumed_len <= available_byte_cnt) {   /* Check if input buffer size is enough or not */
                        smpl_cnt += nrof_blocks * nrof_subbands;
                    }
                }
            } else {    /* Invalid bitpool value */
                result = -4;
                break;
            }
            consumed_byte_cnt += consumed_len;
            available_byte_cnt -= consumed_len;
        }
    }
    *p_smpl_cnt = smpl_cnt;
    *p_byte_cnt = consumed_byte_cnt;
    return result;
}
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */

bt_codec_media_status_t bt_codec_a2dp_aws_parse_data_information(bt_media_handle_t *handle, bt_codec_a2dp_buffer_t *data, bt_codec_a2dp_bitstream_t *information)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    bt_a2dp_codec_type_t codec_type = p_info->codec_info.codec_cap.type;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        uint8_t *buffer = data->buffer;
        uint32_t sample_count = 0;
        uint32_t byte_count = data->byte_count;
        int32_t parsed = bt_codec_a2dp_aws_sbc_get_media_payload_sample_count(buffer, &sample_count, &byte_count);
        if (parsed >= 0) {
            information->sample_count = sample_count;
            information->byte_count = byte_count;
        } else {
            information->sample_count = 0;
            information->byte_count = 0;
            result = BT_CODEC_MEDIA_STATUS_ERROR;
        }
    } else if (codec_type == BT_A2DP_CODEC_AAC) {
#ifdef MTK_BT_A2DP_AAC_ENABLE
        uint32_t byte_count = data->byte_count;
        information->sample_count = 1024;
        information->byte_count = byte_count;
#else   /* MTK_BT_A2DP_AAC_ENABLE */
        information->sample_count = 0;
        information->byte_count = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* MTK_BT_A2DP_AAC_ENABLE */
    } else {
        information->sample_count = 0;
        information->byte_count = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return result;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_get_clock_skew_status(bt_media_handle_t *handle, bt_codec_aws_clock_skew_status_t *status)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    *status = p_info->clock_skew_status;
    return BT_CODEC_MEDIA_STATUS_OK;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_set_clock_skew_compensation_value(bt_media_handle_t *handle, int32_t sample_count)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    if (p_info->clock_skew_status == BT_CODEC_AWS_CLOCK_SKEW_STATUS_IDLE) {
        if (sample_count != 0) {
            p_info->remained_sample_count = sample_count;
            p_info->clock_skew_status = BT_CODEC_AWS_CLOCK_SKEW_STATUS_BUSY;
            /* Compensate clock skew */
        }
    } else {
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return result;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_get_audio_latency(bt_media_handle_t *handle, uint32_t sampling_rate, uint32_t *p_latency_us)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    bt_a2dp_codec_type_t codec_type = p_info->codec_info.codec_cap.type;
    uint32_t latency_us;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        switch (sampling_rate) {
            case 48000: latency_us = 4805;  break;
            case 44100: latency_us = 4805;  break;
            case 32000: latency_us = 4805;  break;
            case 16000: latency_us = 4805;  break;
            default:
                latency_us = 0;
                result = BT_CODEC_MEDIA_STATUS_ERROR;
                break;
        }
    } else if (codec_type == BT_A2DP_CODEC_AAC) {
#ifdef MTK_BT_A2DP_AAC_ENABLE
        switch (sampling_rate) {
            case 48000: latency_us = 8604;  break;
            case 44100: latency_us = 8604;  break;
            case 32000: latency_us = 8604;  break;
            case 24000: latency_us = 8604;  break;
            case 22050: latency_us = 8604;  break;
            case 16000: latency_us = 8604;  break;
            case 12000: latency_us = 8604;  break;
            case 11025: latency_us = 8604;  break;
            case  8000: latency_us = 8604;  break;
            default:
                latency_us = 0;
                result = BT_CODEC_MEDIA_STATUS_ERROR;
                break;
        }
#else   /* MTK_BT_A2DP_AAC_ENABLE */
        latency_us = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* MTK_BT_A2DP_AAC_ENABLE */
    } else {
        latency_us = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    *p_latency_us = latency_us;
    return result;
#else   /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif  /* defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}
