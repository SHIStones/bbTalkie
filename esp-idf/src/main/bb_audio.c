/**
 * @file bb_audio.c
 * @brief ESP-SR 语音检测、ADPCM 编解码、ESP-NOW 音频收发和 I2S 播放。
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_adpcm_dec.h"
#include "esp_adpcm_enc.h"
#include "esp_afe_sr_models.h"
#include "esp_audio_dec.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_enc.h"
#include "esp_audio_enc_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include "esp_board_init.h"
#include "esp_log.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_now.h"
#include "esp_wifi.h"

#include "include/agc.h"
#include "include/app_config.h"
#include "include/app_state.h"
#include "include/bb_audio.h"
#include "include/bb_display.h"
#include "include/bb_radio.h"
#include "include/sound.h"

static adpcm_encode_buffer_t encode_buffer;

float calculate_rms(int16_t *buffer, size_t samples)
{
    float sum = 0.0f;

    if ((buffer == NULL) || (samples == 0))
    {
        return 0.0f;
    }

    for (size_t i = 0; i < samples; i++)
    {
        float sample = (float)buffer[i];
        sum += sample * sample;
    }

    return sqrtf(sum / samples);
}

void apply_agc(int16_t *buffer, size_t samples, agc_t *agc)
{
    if ((buffer == NULL) || (samples == 0) || (agc == NULL))
    {
        return;
    }

    float current_rms = calculate_rms(buffer, samples);
    if (current_rms < 1.0f)
    {
        current_rms = 1.0f;
    }

    float desired_gain = agc->target_rms / current_rms;
    float rate = (desired_gain < agc->current_gain) ? agc->attack_rate : agc->release_rate;
    agc->current_gain += rate * (desired_gain - agc->current_gain);

    if (agc->current_gain < MIN_GAIN)
    {
        agc->current_gain = MIN_GAIN;
    }
    else if (agc->current_gain > MAX_GAIN)
    {
        agc->current_gain = MAX_GAIN;
    }

    for (size_t i = 0; i < samples; i++)
    {
        float sample = (float)buffer[i] * agc->current_gain;

        if (sample > 32767.0f)
        {
            sample = 32767.0f;
        }
        else if (sample < -32768.0f)
        {
            sample = -32768.0f;
        }

        buffer[i] = (int16_t)sample;
    }
}

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();

    printf("feed chunksize:%d, channel:%d\n", audio_chunksize, nch);
    assert(nch == feed_channel);

    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (1)
    {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        afe_handle->feed(afe_data, i2s_buff);
    }

    free(i2s_buff);
    vTaskDelete(NULL);
}

void encode_adpcm(const int16_t *pcm_data, size_t pcm_len_bytes, uint8_t *adpcm_output, size_t *adpcm_len)
{
    if ((pcm_data == NULL) || (adpcm_output == NULL) || (adpcm_len == NULL))
    {
        return;
    }

    esp_audio_enc_register_default();

    esp_adpcm_enc_config_t adpcm_cfg = {
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BIT_DEPTH,
        .channel = 1,
    };

    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_ADPCM,
        .cfg = &adpcm_cfg,
        .cfg_sz = sizeof(adpcm_cfg),
    };

    esp_audio_enc_handle_t encoder = NULL;
    esp_audio_enc_open(&enc_cfg, &encoder);

    esp_audio_enc_in_frame_t in_frame = {
        .buffer = (uint8_t *)pcm_data,
        .len = pcm_len_bytes,
    };

    esp_audio_enc_out_frame_t out_frame = {
        .buffer = adpcm_output,
        .len = (pcm_len_bytes / 4) + 7,
    };

    esp_audio_enc_process(encoder, &in_frame, &out_frame);
    *adpcm_len = out_frame.encoded_bytes;
}

void decode_adpcm(const uint8_t *adpcm_data, size_t adpcm_len, uint8_t *pcm_output, size_t *pcm_len)
{
    if ((adpcm_data == NULL) || (pcm_output == NULL) || (pcm_len == NULL))
    {
        return;
    }

    esp_audio_dec_register_default();

    esp_adpcm_dec_cfg_t adpcm_cfg = {
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BIT_DEPTH / 4,
        .channel = 1,
    };

    esp_audio_dec_cfg_t dec_cfg = {
        .type = ESP_AUDIO_TYPE_ADPCM,
        .cfg = &adpcm_cfg,
        .cfg_sz = sizeof(adpcm_cfg),
    };

    esp_audio_dec_handle_t decoder = NULL;
    esp_audio_dec_open(&dec_cfg, &decoder);

    esp_audio_dec_in_raw_t raw = {
        .buffer = (uint8_t *)adpcm_data,
        .len = adpcm_len,
    };

    esp_audio_dec_out_frame_t out_frame = {
        .buffer = (uint8_t *)pcm_output,
        .len = adpcm_len * 4,
    };

    esp_audio_dec_process(decoder, &raw, &out_frame);
    *pcm_len = ADPCM_FRAME_SIZE * 2;
}

void decode_Task(void *arg)
{
    (void)arg;

    uint8_t *pcm_buffer = malloc(ENCODED_BUF_SIZE);
    size_t pcm_len = 0;
    TickType_t last_recv_time = xTaskGetTickCount();

    if (pcm_buffer == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate PCM buffer");
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        esp_now_recv_data_t recv_data;

        if (get_esp_now_data(&recv_data) == true)
        {
            is_receiving = true;
            last_recv_time = xTaskGetTickCount();

            decode_adpcm(recv_data.data, recv_data.data_len, pcm_buffer, &pcm_len);
            (void)xStreamBufferSend(play_stream_buf, pcm_buffer, pcm_len, 0);
        }
        else if ((xTaskGetTickCount() - last_recv_time) > pdMS_TO_TICKS(128))
        {
            is_receiving = false;

            ESP_ERROR_CHECK(esp_now_set_wake_window(25));
            ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(100));

            const int silence_samples = 512;
            int16_t *silence_buffer = calloc(silence_samples * 2, sizeof(int16_t));

            if (silence_buffer != NULL)
            {
                esp_err_t ret = esp_audio_play(silence_buffer, silence_samples, portMAX_DELAY);
                if (ret != ESP_OK)
                {
                    printf("Failed to end audio: %s", esp_err_to_name(ret));
                }
                free(silence_buffer);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

/**
 * @brief 清空 ADPCM 编码累积缓冲。
 *
 * @param enc_buf 编码缓冲。
 * @retval None
 */
static void init_encode_buffer(adpcm_encode_buffer_t *enc_buf)
{
    if (enc_buf != NULL)
    {
        enc_buf->current_samples = 0;
    }
}

/**
 * @brief 累积 PCM 样本，满一帧后编码并广播。
 *
 * @param enc_buf 编码缓冲。
 * @param pcm_data PCM 样本。
 * @param pcm_samples 样本数。
 * @param adpcm_output ADPCM 临时输出缓冲。
 * @retval None
 */
static void encode_and_send(adpcm_encode_buffer_t *enc_buf,
                            const int16_t *pcm_data,
                            size_t pcm_samples,
                            uint8_t *adpcm_output)
{
    size_t samples_processed = 0;

    if ((enc_buf == NULL) || (pcm_data == NULL) || (adpcm_output == NULL))
    {
        return;
    }

    while (samples_processed < pcm_samples)
    {
        size_t samples_to_copy = ADPCM_FRAME_SIZE - enc_buf->current_samples;
        size_t samples_available = pcm_samples - samples_processed;

        if (samples_to_copy > samples_available)
        {
            samples_to_copy = samples_available;
        }

        memcpy(&enc_buf->buffer[enc_buf->current_samples],
               &pcm_data[samples_processed],
               samples_to_copy * sizeof(int16_t));

        enc_buf->current_samples += samples_to_copy;
        samples_processed += samples_to_copy;

        if (enc_buf->current_samples == ADPCM_FRAME_SIZE)
        {
            size_t adpcm_len = 0;
            encode_adpcm(enc_buf->buffer, ADPCM_FRAME_BYTES, adpcm_output, &adpcm_len);

            if (adpcm_len > 0)
            {
                send_data_esp_now(adpcm_output, adpcm_len);
            }

            enc_buf->current_samples = 0;
        }
    }
}

/**
 * @brief 语音结束时用静音补齐最后一帧并发送。
 *
 * @param enc_buf 编码缓冲。
 * @param adpcm_output ADPCM 临时输出缓冲。
 * @retval None
 */
static void flush_encode_buffer(adpcm_encode_buffer_t *enc_buf, uint8_t *adpcm_output)
{
    if ((enc_buf == NULL) || (adpcm_output == NULL) || (enc_buf->current_samples == 0))
    {
        return;
    }

    memset(&enc_buf->buffer[enc_buf->current_samples],
           0,
           (ADPCM_FRAME_SIZE - enc_buf->current_samples) * sizeof(int16_t));

    size_t adpcm_len = 0;
    encode_adpcm(enc_buf->buffer, ADPCM_FRAME_BYTES, adpcm_output, &adpcm_len);

    if (adpcm_len > 0)
    {
        send_data_esp_now(adpcm_output, adpcm_len);
    }

    enc_buf->current_samples = 0;
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE);

    if (mn_name == NULL)
    {
        ESP_LOGE(TAG, "Chinese MultiNet model not found. Enable CONFIG_SR_MN_CN_MULTINET7_QUANT and flash the model partition.");
        vTaskDelete(NULL);
        return;
    }

    printf("multinet:%s\n", mn_name);

    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    if (multinet == NULL)
    {
        ESP_LOGE(TAG, "Failed to get MultiNet handle for model: %s", mn_name);
        vTaskDelete(NULL);
        return;
    }

    model_iface_data_t *model_data = multinet->create(mn_name, 1488);
    if (model_data == NULL)
    {
        ESP_LOGE(TAG, "Failed to create MultiNet model data for model: %s", mn_name);
        vTaskDelete(NULL);
        return;
    }

    int mu_chunksize = multinet->get_samp_chunksize(model_data);

    printf("mu chunksize:%d, afe chunksize:%d\n", mu_chunksize, afe_chunksize);
    assert(mu_chunksize == afe_chunksize);
    multinet->print_active_speech_commands(model_data);
    printf("------------detect start------------\n");
    printf("------------vad start------------\n");

    uint8_t *adpcm_output = malloc(ENCODED_BUF_SIZE);
    if (adpcm_output == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate ADPCM buffer");
        vTaskDelete(NULL);
        return;
    }

    printf("detect_Task init_encode_buffer\n");
    init_encode_buffer(&encode_buffer);

    esp_mn_state_t mn_state = ESP_MN_STATE_DETECTING;
    printf("detect_Task enter loop\n");

    while (1)
    {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if ((res == NULL) || (res->ret_value == ESP_FAIL))
        {
            printf("fetch error!\n");
            break;
        }

        if ((res->vad_state != VAD_SILENCE) && (is_receiving == false) && (isMicOff == false))
        {
            is_speaking = true;

            if (res->vad_cache_size > 0)
            {
                size_t cache_samples = res->vad_cache_size / sizeof(int16_t);
                encode_and_send(&encode_buffer, res->vad_cache, cache_samples, adpcm_output);

                size_t num_chunks = cache_samples / (size_t)mu_chunksize;
                for (size_t i = 0; i < num_chunks; i++)
                {
                    int16_t *chunk = res->vad_cache + (i * mu_chunksize);
                    mn_state = multinet->detect(model_data, chunk);
                }
            }

            if (res->vad_state == VAD_SPEECH)
            {
                size_t data_samples = res->data_size / sizeof(int16_t);
                encode_and_send(&encode_buffer, res->data, data_samples, adpcm_output);
                mn_state = multinet->detect(model_data, res->data);
            }

            if (mn_state == ESP_MN_STATE_DETECTING)
            {
                continue;
            }

            if (mn_state == ESP_MN_STATE_DETECTED)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);

                for (int i = 0; i < mn_result->num; i++)
                {
                    printf("TOP %d, command_id: %d, phrase_id: %d, string:%s prob: %f\n",
                           i + 1,
                           mn_result->command_id[i],
                           mn_result->phrase_id[i],
                           mn_result->string,
                           mn_result->prob[i]);
                }

                printf("Playing animation for command_id: %d\n", mn_result->command_id[0]);
                bb_display_show_command_animation(mn_result->command_id[0]);

                char cmd_buffer[32];
                int cmd_len = snprintf(cmd_buffer, sizeof(cmd_buffer), "CMD:%d", mn_result->command_id[0]);
                if ((cmd_len > 0) && (cmd_len < (int)sizeof(cmd_buffer)))
                {
                    send_data_esp_now((const uint8_t *)cmd_buffer, (size_t)cmd_len);
                    ESP_LOGI(TAG, "Sent CMD via ESP-NOW: %d", mn_result->command_id[0]);
                }
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);

                printf("timeout, string:%s\n", mn_result->string);
                bb_display_show_message_bubble(mn_result->string);

                char msg_buffer[ESP_NOW_MAX_DATA_LEN_V2];
                int msg_len = snprintf(msg_buffer, sizeof(msg_buffer), "MSG:%s", mn_result->string);
                if ((msg_len > 0) && (msg_len < (int)sizeof(msg_buffer)))
                {
                    send_data_esp_now((const uint8_t *)msg_buffer, (size_t)msg_len);
                    ESP_LOGI(TAG, "Sent timeout MSG via ESP-NOW: %s", mn_result->string);
                }
                continue;
            }
        }
        else
        {
            if (is_speaking == true)
            {
                flush_encode_buffer(&encode_buffer, adpcm_output);
                printf("clean\n");
                multinet->clean(model_data);
            }
            is_speaking = false;
        }
    }

    free(adpcm_output);
    vTaskDelete(NULL);
}

void boot_sound(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(650));
    esp_err_t ret = esp_audio_play((const int16_t *)boot, sizeof(boot) / 2, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        printf("Failed to play boot sound: %s", esp_err_to_name(ret));
    }

    vTaskDelete(NULL);
}

void byebye_sound(void *pvParameters)
{
    (void)pvParameters;

    esp_err_t ret = esp_audio_play((const int16_t *)byebye, sizeof(byebye) / 2, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        printf("Failed to play byebye sound: %s", esp_err_to_name(ret));
    }

    vTaskDelete(NULL);
}

void i2s_writer_task(void *arg)
{
    (void)arg;

    uint8_t i2s_buf[PLAY_CHUNK_SIZE];

    while (1)
    {
        size_t received = xStreamBufferReceive(play_stream_buf, i2s_buf, PLAY_CHUNK_SIZE, portMAX_DELAY);

        if ((received > 0) && (isMute == false))
        {
            apply_agc((int16_t *)i2s_buf, received / 2, &agc_custom);

            esp_err_t ret = esp_audio_play((const int16_t *)i2s_buf, received / 2, portMAX_DELAY);
            if (ret != ESP_OK)
            {
                printf("Failed to play audio: %s", esp_err_to_name(ret));
            }
        }
    }
}

void init_audio_stream_buffer(void)
{
    play_stream_buf = xStreamBufferCreate(PLAY_RING_BUFFER_SIZE, 1);
    assert(play_stream_buf);
}
