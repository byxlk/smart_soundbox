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

#ifndef __BT_AWS_H__
#define __BT_AWS_H__

#include "bt_a2dp.h"

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothAWS AWS
 * @{
 * The Advanced Wireless Stereo (AWS) is MediaTek proprietary profile that defines the minimum set of functions such that two devices can transfer the audio data and synchronize the audio status.
 * The AWS profile provides the following roles - Source (SRC) and Sink (SNK).
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                 |
 * |------------------------------|------------------------------------------------------------------------|
 * |\b AWS                        | Advanced Wireless Stereo. |
 * |\b SBC                        | The Low Complexity Sub-Band Coding (SBC) is an audio sub-band codec specified by the Bluetooth Special Interest Group (SIG) for the Advanced Audio Distribution Profile (A2DP). SBC is a digital audio encoder and decoder used to transfer data to Bluetooth audio output devices like headphones or loudspeakers. For more information, please refer to <a href="https://en.wikipedia.org/wiki/SBC_(codec)">Wikipedia</a>.|
 * |\b AAC                        | Advanced Audio Coding (AAC) is an audio coding standard for lossy digital audio compression. For more information, please refer to <a href="https://en.wikipedia.org/wiki/Advanced_Audio_Coding">Wikipedia</a>.|
 * |\b SNK                        | Sink. A device is the SNK when it acts as a sink of a digital audio stream delivered from the SRC on the same piconet. |
 * |\b SRC                        | Source. A device is the SRC when it acts as a source of a digital audio stream that is delivered to the SNK of the piconet. |
 *
 * @section bt_aws_api_usage How to use this module
 * This section presents the AWS connection handle and a method to stream and synchronize audio signals.
 *  - 1. Mandatory, implement #bt_app_event_callback() to handle the AWS events, such as connect, disconnect, indication and more.
 *   - Sample code:
 *    @code
 *       void bt_app_event_callback(bt_msg_type_t msg_type, bt_status_t status, void *data)
 *       {
 *          switch (msg_type)
 *          {
 *              case BT_AWS_CONNECT_IND:
 *              {
 *                  bt_aws_connect_ind_t *conn_ind =
 *                      (bt_aws_connect_ind_t *)data;
 *                  bt_aws_connect_response(conn_ind->handle,true);
 *                  // Respond to the connection request.
 *                  break;
 *              }
 *              case BT_AWS_ROLE_SET_IND:
 *              {
 *                  // The application can stream, notify, send commands and more, only after the role is set.
 *                  break;
 *              }
 *              case BT_AWS_START_STREAMING_IND:
 *              {
 *                  // The application should call the #bt_aws_start_streaming_response() API to respond to the remote device after receiving this indication.
 *                  break;
 *              }
 *              case BT_AWS_SUSPEND_STREAMING_IND:
 *              {
 *                  // The application should call the #bt_aws_suspend_streaming_response() API to respond to the remote device after receiving this indication.
 *                  break;
 *              }
 *              case BT_AWS_COMMAND_IND:
 *              {
 *                  // The application can get the command request when receiving this indication.
 *                  break;
 *              }
 *              case BT_AWS_NOTIFY_IND:
 *              {
 *                  break;
 *              }
 *              default:
 *              {
 *                  break;
 *              }
 *           }
 *       }
 *    @endcode
 *  - 2. Mandatory, connect to a remote device, as shown in figure titled as "AWS connection establishment message sequence" in the Bluetooth developer's guide under the [sdk_root]/doc folder.
 *   - Sample code:
 *      Connect to a remote device by passing the remote Bluetooth address and get the connection handle.
 *      The event #BT_AWS_CONNECT_CNF captured by #bt_app_event_callback() indicates the AWS is connected.
 *    @code
 *       bt_aws_connect(&handle, &addr);
 *    @endcode
 *  - 3. Mandatory, synchronize the start playing time with the Bluetooth clock,  and play the streaming data at same time in both devices, as shown in figure titled as "AWS synchronize the voice message sequence" in the Bluetooth developer's guide under the [sdk_root]/doc folder.
 *   - Sample code:
 *    @code
 *     // Device A
 *
 *     bt_aws_notify_t aws_notify;
 *     // Get the current Bluetooth clock.
 *     bt_aws_get_bt_local_time(&sink_loc_play_nclk, &sink_loc_play_nclk_intra);
 *     aws_notify.op_id = BT_AWS_NOTIFY_ID_READY_TO_PLAY;
 *     aws_notify.play_time.play_time = time_dur;// The value of time_dur should be greater than the command transfer time. The recommended value is 100ms.
 *     aws_notify.play_time.loc_play_nclk = sink_loc_play_nclk;
 *     aws_notify.play_time.loc_play_nclk_intra = sink_loc_play_nclk_intra;
 *
 *     // Notify the remote device is ready to play.
 *     err = bt_aws_notify(aws_handle, &aws_notify);
 *
 *     // Define a timer then play the audio once it's reached.
 *     hal_gpt_start_timer_us(HAL_GPT_1, time_dur, HAL_GPT_TIMER_TYPE_ONE_SHOT);
 *
 *     // Play the music when the time is up.
 *     //-------------------------------------------------------------------------------------
 *     // Device B
 *
 *     // Start a timer to wait before playing the music, after receiving the #BT_AWS_NOTIFY_ID_READY_TO_PLAY notification ID.
 *     time_dur = notify_ind->param->play_time.play_time;
 *     hal_gpt_register_callback(HAL_GPT_1,
 *                             bt_sink_srv_aws_gpt_cb, (void *)dev);
 *     gpt_ret = hal_gpt_start_timer_us(HAL_GPT_1, time_dur, HAL_GPT_TIMER_TYPE_ONE_SHOT);
 *
 *     // Play the music when the time is up.
 *    @endcode
 *  - 4. Mandatory, disconnect the remote device, as shown in figure titled as "AWS connection release message sequence" in the Bluetooth developer's guide under the [sdk_root]/doc folder.
 *   - Sample code:
 *        Disconnect the remote device by passing the connection handle.
 *        The event #BT_AWS_DISCONNECT_IND captured by #bt_app_event_callback() indicates the AWS channel has disconnected.
 *    @code
 *       ret = bt_aws_disconnect(handle);
 *    @endcode
 */



/**
 * @brief The event report to user
 */

#define BT_AWS_CONNECT_IND             (BT_MODULE_AWS | 0x0001)       /**< An attempt to connect from a remote device with the #bt_aws_connect_ind_t payload. Call #bt_aws_connect_response() to accept or reject the attempt. */
#define BT_AWS_CONNECT_CNF             (BT_MODULE_AWS | 0x0002)       /**< The result of a connection attempt initiated by a local or remote device is available with the #bt_aws_connect_cnf_t payload. */
#define BT_AWS_DISCONNECT_IND          (BT_MODULE_AWS | 0x0003)       /**< A disconnect attempt is initiated by a local or remote device or a link loss occurred with the #bt_aws_disconnect_ind_t payload. */
#define BT_AWS_DISCONNECT_CNF          (BT_MODULE_AWS | 0x0004)       /**< The result of a disconnect attempt initiated by a local device is available with the #bt_aws_disconnect_cnf_t payload.  */
#define BT_AWS_ROLE_SET_IND            (BT_MODULE_AWS | 0x0005)       /**< The role set indication is triggered when the AWS role is set with the #bt_aws_role_set_ind_t payload. */
#define BT_AWS_START_STREAMING_IND     (BT_MODULE_AWS | 0x0006)       /**< A start streaming request is initiated from a remote device with the #bt_aws_start_streaming_ind_t payload. Call #bt_aws_start_streaming_response() to accept or reject the request. */
#define BT_AWS_START_STREAMING_CNF     (BT_MODULE_AWS | 0x0007)       /**< The result of a start streaming attempt initiated by a local device is available with the #bt_aws_start_streaming_cnf_t payload. */
#define BT_AWS_SUSPEND_STREAMING_IND   (BT_MODULE_AWS | 0x0008)       /**< A suspend streaming request is initiated from a remote device with the #bt_aws_suspend_streaming_ind_t payload. Call #bt_aws_suspend_streaming_response() to accept or reject the request. */
#define BT_AWS_SUSPEND_STREAMING_CNF   (BT_MODULE_AWS | 0x0009)       /**< The result of a suspend streaming attempt initiated by a local device is available with the #bt_aws_suspend_streaming_cnf_t payload. */
#define BT_AWS_COMMAND_IND             (BT_MODULE_AWS | 0x000C)       /**< A command request is initiated from a remote device with the #bt_aws_command_ind_t payload. Call #bt_aws_command_response() to accept or reject the request. */
#define BT_AWS_NOTIFY_IND              (BT_MODULE_AWS | 0x000E)       /**< The notify indication is initiated from a remote device with the  #bt_aws_notification_ind_t payload. */   
#define BT_AWS_STREAMING_RECEIVED_IND  (BT_MODULE_AWS | 0x000F)       /**< The indication is triggered when streaming data is received with the #bt_aws_streaming_received_ind_t payload.*/
#define BT_AWS_RESYNC_IND              (BT_MODULE_AWS | 0x0011)       /**< A re-sync attempt is initiated from a remote device with the #bt_aws_resync_ind_t payload. The user has to call #bt_aws_resync_response() to accept or reject the request and clear the current streaming buffer. */
#define BT_AWS_RESYNC_CNF              (BT_MODULE_AWS | 0x0012)       /**< The result of a re-sync attempt initiated by a local device is available with the #bt_aws_resync_cnf_t payload.*/

BT_EXTERN_C_BEGIN
/**
 * @brief This structure defines the AWS role.
 */
typedef uint8_t bt_aws_role_t;
#define BT_AWS_ROLE_SOURCE      0x00                      /**< The AWS role of source. */
#define BT_AWS_ROLE_SINK        0x01                      /**< The AWS role of sink. */
#define BT_AWS_ROLE_NONE        0xFF                      /**< The AWS role of none. */


/**
 *  @brief Define for AWS codec type.
 */
typedef uint8_t bt_aws_codec_type_t;
#define BT_AWS_CODEC_TYPE_SBC         0x00           /**< SBC codec. */
#define BT_AWS_CODEC_TYPE_MP3         0x01           /**< MP3 codec. */
#define BT_AWS_CODEC_TYPE_AAC         0x02           /**< AAC codec. */
#define BT_AWS_CODEC_TYPE_ATRAC       0x04           /**< ATRAC codec. */
#define BT_AWS_CODEC_TYPE_RAW         0x80           /**< RAW data. */


/**
 * @brief The AWS command ID.
 */
typedef uint8_t bt_aws_command_id_t;
#define BT_AWS_COMMAND_ID_PLAY          0x01         /**< The command ID of start. */
#define BT_AWS_COMMAND_ID_PAUSE         0x02         /**< The command ID of pause. */
#define BT_AWS_COMMAND_ID_FORWARD       0x03         /**< The command ID of forward. */
#define BT_AWS_COMMAND_ID_BACKWARD      0x04         /**< The command ID of backward. */
#define BT_AWS_COMMAND_ID_CUSTOMIZE     0xE0         /**< The command ID of a user-defined command, the ID range is from 0xE0 to 0xFF. */


/**
 * @brief The AWS notification ID.
 */
typedef uint8_t bt_aws_notify_id_t;
#define BT_AWS_NOTIFY_ID_VOLUME_UP     0x01 /**< Volume up. */
#define BT_AWS_NOTIFY_ID_VOLUME_DOWN   0x02 /**< Volume down. */
#define BT_AWS_NOTIFY_ID_READY_TO_PLAY 0x03 /**< Ready to play. */

/**
 *  @brief This structure defines the AWS codec capability.
 */
typedef struct {
  bt_aws_codec_type_t type;      /**< Codec type. */
  uint8_t   length;              /**< The length of the following codec. */
  union {
        bt_a2dp_aac_codec_t aac; /**< AAC codec. */
        bt_a2dp_sbc_codec_t sbc; /**< SBC codec. */
  } codec;                       /**< Codec information. */
} bt_aws_codec_capability_t;


/**
 * @brief This structure defines the AWS play time with a notification ID #BT_AWS_NOTIFY_ID_READY_TO_PLAY.
 */

typedef struct
{
    uint32_t play_time;             /**< The delay time in microseconds. */
    uint32_t loc_play_nclk;         /**< The current native Bluetooth clock with 312.5us unit, the application can get the value by calling the API #bt_aws_get_bt_local_time().*/
    uint32_t loc_play_nclk_intra;   /**< The current native Bluetooth clock with 1us unit, the application can get the value by calling the API #bt_aws_get_bt_local_time().*/
} bt_aws_play_time_t;


/**
 *  @brief This structure defines the AWS notififcation settings.
 */
typedef struct {
    bt_aws_notify_id_t op_id;          /**< The notification ID. */
    bt_aws_play_time_t play_time;      /**< The time ready to play, the application should define it with #BT_AWS_NOTIFY_ID_READY_TO_PLAY. */
} bt_aws_notify_t;


/**
 *  @brief This structure defines the A2DP sink status settings.
 */
typedef struct {
    bool status;                         /**< If the status is connected, the value is set to 'TRUE', otherwise 'FALSE'. */
    uint32_t a2dp_handle;                /**< Set the value of the A2DP handle, if the A2DP Sink is connected. */
} bt_aws_a2dp_sink_status_t;


/**
 *  @brief This structure defines the result of the #BT_AWS_CONNECT_IND event.
 */
typedef struct {
    uint32_t handle;            /**< AWS handle. */
    bt_bd_addr_t *address;      /**< Bluetooth address of a remote device. */
} bt_aws_connect_ind_t;

/**
 *  @brief This structure defines the #BT_AWS_CONNECT_CNF result.
 */
typedef struct {
    uint32_t handle;           /**< AWS handle. */
    bt_status_t status;        /**< Connection status. */
} bt_aws_connect_cnf_t;

/**
 *  @brief This structure defines the #BT_AWS_DISCONNECT_IND result.
 */
typedef struct {
    uint32_t handle;           /**< AWS handle. */
} bt_aws_disconnect_ind_t;

/**
 *  @brief This structure defines the #BT_AWS_DISCONNECT_CNF result.
 */
typedef struct {
    uint32_t handle;           /**< AWS handle. */
    bt_status_t status;        /**< Disconnect status. */
} bt_aws_disconnect_cnf_t;


/**
 *  @brief This structure defines the #BT_AWS_ROLE_SET_IND result.
 */
typedef struct {
    uint32_t handle;           /**< AWS handle. */
    bt_aws_role_t role;        /**< AWS role. */
} bt_aws_role_set_ind_t;


/**
 *  @brief This structure defines the #BT_AWS_START_STREAMING_IND result.
 */
typedef struct {
    uint32_t handle;                             /**< AWS handle. */
    bt_aws_codec_capability_t *codec_cap;        /**< Streaming channel codec capability. */
} bt_aws_start_streaming_ind_t;


/**
 *  @brief This structure defines the #BT_AWS_START_STREAMING_CNF result.
 */
typedef struct {
    uint32_t handle;                            /**< AWS handle. */
    bt_status_t status;                         /**< Start streaming status. */
} bt_aws_start_streaming_cnf_t;


/**
 *  @brief This structure defines the #BT_AWS_SUSPEND_STREAMING_IND result.
 */
typedef struct {
    uint32_t handle;                           /**< AWS handle. */
} bt_aws_suspend_streaming_ind_t;

/**
 *  @brief This structure defines the #BT_AWS_SUSPEND_STREAMING_CNF result.
 */
typedef struct {
    uint32_t handle;                            /**< AWS handle. */
    bt_status_t status;                         /**< Suspend streaming status. */
} bt_aws_suspend_streaming_cnf_t;

/**
 *  @brief This structure defines the #BT_AWS_RESYNC_IND result.
 */
typedef struct {
    uint32_t handle;                           /**< AWS handle. */
} bt_aws_resync_ind_t;


/**
 *  @brief This structure defines the #BT_AWS_RESYNC_CNF result.
 */
typedef struct {
    uint32_t handle;                           /**< AWS handle. */
    bt_status_t status;                        /**< Resync status. */
} bt_aws_resync_cnf_t;


/**
 *  @brief This structure defines the #BT_AWS_COMMAND_IND result.
 */
typedef struct {
    uint32_t handle;                                /**< AWS handle. */
    bt_aws_command_id_t command_id;                 /**< Current command's command ID. */
    uint8_t *data;                                  /**< The custom data to send to a remote device. */
    uint16_t data_length;                           /**< The custom data length. */
} bt_aws_command_ind_t;


/**
 *  @brief This structure defines the BT_AWS_COMMAND_CNF result.
 */
typedef struct {
    uint32_t handle;                            /**< AWS handle. */
    bt_status_t status;                         /**< The command status. */
} bt_aws_command_cnf_t;


/**
 *  @brief This structure defines the BT_AWS_NOTIFY_IND result.
 */
typedef struct {
    uint32_t handle;                            /**< AWS handle. */
    bt_aws_notify_t *param;                     /**< The notification settings. */
} bt_aws_notification_ind_t;


/**
 *  @brief This structure defines the BT_AWS_STREAMING_RECEIVED_IND result.
 */
typedef struct {
    uint32_t handle;                             /**< AWS handle. */
    uint8_t *data_node;                          /**< Media data pointer. */
    uint16_t total_length;                       /**< The total length of a media packet. */
    uint16_t media_offset;                       /**< The offset of media payload from the whole packet. */
} bt_aws_streaming_received_ind_t;


/**
 * @brief                                         This function sends an AWS connection request to the remote device.
 *                                                The event #BT_AWS_CONNECT_CNF returns when the connection request is sent.
 * @param[out] handle                             is the channel handle.
 * @param[in] address                             is the Bluetooth address of a remote device.
 * @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                                #BT_STATUS_FAIL, the operation has failed.
 *                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
 */
bt_status_t bt_aws_connect(uint32_t *handle, bt_bd_addr_t *address);


/**
 * @brief                                         This function is a response to the remote device's incoming connection request.
 *                                                Application needs to call this API when getting the event #BT_AWS_CONNECT_IND to accept or reject the connection.
 * @param[in] handle                              is the handle of the current connection.
 * @param[in] accept                              is the operation of accepting or rejecting the current connection request.
 * @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                                #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t bt_aws_connect_response(uint32_t handle, bool accept);


/**
 * @brief                                         This function sends an AWS disconnect request to the remote device.
 *                                                The event #BT_AWS_DISCONNECT_CNF returns the current request's result.
 * @param[in] handle                              is the handle of the current connection.
 * @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                                #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t bt_aws_disconnect(uint32_t handle);


/**
* @brief                                         This function sends an AWS command to the remote device.
* @param[in] handle                              is the handle of the current connection.
* @param[in] command_id                          is the command ID to send.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
*/
bt_status_t bt_aws_send_command(uint32_t handle, bt_aws_command_id_t command_id, const uint8_t *data, uint16_t data_length);



/**
* @brief                                         This function sends an AWS start streaming request to the remote device.
* @param[in] handle                              is the handle of the current connection.
* @param[in] capability                          is the codec capability of the current streaming.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
*/
bt_status_t bt_aws_start_streaming_request(uint32_t handle, bt_aws_codec_capability_t *capability);


/**
* @brief                                         This function sends an AWS start streaming response to the remote device.
* @param[in] handle                              is the handle of the current connection.
* @param[in] accept                              is the parameter to accept or reject the remote request.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
*/
bt_status_t bt_aws_start_streaming_response(uint32_t handle, bool accept);


/**
* @brief                                         This function sends an AWS suspend streaming request to the remote device.
* @param[in] handle                              is the handle of current connection.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
*/
bt_status_t bt_aws_suspend_streaming_request(uint32_t handle);


/**
* @brief                                         This function sends an AWS suspend streaming response to the remote device.
* @param[in] handle                              is the handle of the current connection.
* @param[in] accept                              is the parameter to accept or reject the remote request.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
*/
bt_status_t bt_aws_suspend_streaming_response(uint32_t handle, bool accept);


/**
* @brief                                         This function sends an AWS resync request to the remote device, the AWS profile will drop the streaming data till get the resync response.
* @param[in] handle                              is the handle of the current connection.
* @param[in] accept                              is the parameter to accept or reject the remote request.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
*/
bt_status_t bt_aws_resync_request(uint32_t handle);


/**
* @brief                                         This function sends an AWS resync response to the remote device.
* @param[in] handle                              is the handle of the current connection.
* @param[in] accept                              is the parameter to accept or reject the remote request.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
*/
bt_status_t bt_aws_resync_response(uint32_t handle);


/**
* @brief                                         This function sends an AWS notification to the remote device.
* @param[in] handle                              is the handle of the current connection.
* @param[in] notificaton                         is the notification to notify the remote device.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*                                                #BT_STATUS_OUT_OF_MEMORY, out of memory.
*/
bt_status_t bt_aws_notify(uint32_t handle, bt_aws_notify_t *notification);


/**
* @brief                                         Get the current native Bluetooth clock.
* @param[out] nclk                               is the Bluetooth clock with unit 312.5us.
* @param[out] nclk_intra                         is the Bluetooth clock with unit 1us.
* @return                                        #BT_STATUS_SUCCESS, the operation completed successfully.
*                                                #BT_STATUS_FAIL, the operation has failed.
*/
bt_status_t bt_aws_get_bt_local_time(uint32_t *nclk, uint32_t *nclk_intra);


/**
 * @brief                                         Get the A2DP SNK connection status. This API is invoked when AWS is connected. The application should implement this function.
 * @param[in] handle                              is the handle of the current connection.
 * @param[out] a2dp_status                        is the A2DP connection status.
 * @return                                        #BT_STATUS_SUCCESS, the operation completed successfully, otherwise it failed.
 */
bt_status_t bt_aws_get_a2dp_sink_status(uint32_t handle, bt_aws_a2dp_sink_status_t *a2dp_status);


BT_EXTERN_C_END


#endif /*__BT_AWS_H__*/
