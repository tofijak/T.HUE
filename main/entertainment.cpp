#include <Arduino.h>
#include "entertainment.h"
#include <vector>
#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/timing.h"
#include "hue.h"
#include "eink.h"
#include "secret.h"

std::vector<uint8_t> entertainment_msg; //!< buffer containing the entertainment mode packet data

constexpr uint8_t HUE_ENTERTAINMENT_HEADER_SIZE = 16;
constexpr uint8_t HUE_ENTERTAINMENT_LIGHT_SIZE = 9;

uint8_t entertainment_num_lights; //!< number of lights in entertainment mode group

bool startBehaviorNow = false;
bool fest = false;

struct TLSContext
{
    mbedtls_ssl_context ssl;
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_timing_delay_context timer;
};

TLSContext *tls_context;

std::vector<char> hexToBytes(const std::string &hex)
{
    std::vector<char> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

struct _hr_time
{
    struct timeval start;
};
unsigned long my_mbedtls_timing_get_timer(struct mbedtls_timing_hr_time *val, int reset)
{
    struct _hr_time *t = (struct _hr_time *)val;

    if (reset)
    {
        gettimeofday(&t->start, NULL);
        return (0);
    }
    else
    {
        unsigned long delta;
        struct timeval now;
        gettimeofday(&now, NULL);
        delta = (now.tv_sec - t->start.tv_sec) * 1000ul + (now.tv_usec - t->start.tv_usec) / 1000;
        return (delta);
    }
}

void my_mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
    mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *)data;

    ctx->int_ms = int_ms;
    ctx->fin_ms = fin_ms;

    if (fin_ms != 0)
        (void)my_mbedtls_timing_get_timer(&ctx->timer, 1);
}

int _handle_error(int err, const char *file, int line)
{
    if (err == -30848)
    {
        return err;
    }
#ifdef MBEDTLS_ERROR_C
    char error_buf[100];
    mbedtls_strerror(err, error_buf, 100);
    log_e("[%s():%d]: (%#02X) %s", file, line, err, error_buf);
#else
    log_e("[%s():%d]: code %#02X", file, line, err);
#endif
    return err;
}

int my_mbedtls_timing_get_delay(void *data)
{
    mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *)data;
    unsigned long elapsed_ms;

    if (ctx->fin_ms == 0)
        return (-1);

    elapsed_ms = my_mbedtls_timing_get_timer(&ctx->timer, 0);

    if (elapsed_ms >= ctx->fin_ms)
        return (2);

    if (elapsed_ms >= ctx->int_ms)
        return (1);

    return (0);
}

void initEntertainmentMode(Hue &hue, uint8_t roomId, uint8_t lightCount)
{
    tls_context = new TLSContext;

    /*-------------------------------------------------*\
    | Signal the bridge to start streaming              |
    \*-------------------------------------------------*/
    hue.startStreaming(roomId);
    //bridge->startStreaming(std::to_string(group->getId()));

    entertainment_num_lights = lightCount;

    /*-------------------------------------------------*\
    | Resize Entertainment Mode message buffer          |
    \*-------------------------------------------------*/
    entertainment_msg.resize(HUE_ENTERTAINMENT_HEADER_SIZE + (lightCount * HUE_ENTERTAINMENT_LIGHT_SIZE));

    /*-------------------------------------------------*\
    | Fill in Entertainment Mode message header         |
    \*-------------------------------------------------*/
    memcpy(&entertainment_msg[0], "HueStream", 9);
    entertainment_msg[9] = 0x01;  // Version Major (1)
    entertainment_msg[10] = 0x00; // Version Minor (0)
    entertainment_msg[11] = 0x00; // Sequence ID
    entertainment_msg[12] = 0x00; // Reserved
    entertainment_msg[13] = 0x00; // Reserved
    entertainment_msg[14] = 0x00; // Color Space (RGB)
    entertainment_msg[15] = 0x00; // Reserved

    /*-------------------------------------------------*\
    | Initialize mbedtls contexts                       |
    \*-------------------------------------------------*/
    mbedtls_net_init(&tls_context->server_fd);
    mbedtls_ssl_init(&tls_context->ssl);
    mbedtls_ssl_config_init(&tls_context->conf);
    mbedtls_x509_crt_init(&tls_context->cacert);
    mbedtls_ctr_drbg_init(&tls_context->ctr_drbg);
    mbedtls_entropy_init(&tls_context->entropy);

    /*-------------------------------------------------*\
    | Seed the Deterministic Random Bit Generator (RNG) |
    \*-------------------------------------------------*/
    int ret = mbedtls_ctr_drbg_seed(&tls_context->ctr_drbg, mbedtls_entropy_func, &tls_context->entropy, NULL, 0);

    /*-------------------------------------------------*\
    | Parse certificate                                 |
    \*-------------------------------------------------*/
    ret = mbedtls_x509_crt_parse(
        &tls_context->cacert, (const unsigned char *)mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
}

void free()
{
    mbedtls_entropy_free(&tls_context->entropy);
    mbedtls_ctr_drbg_free(&tls_context->ctr_drbg);
    mbedtls_x509_crt_free(&tls_context->cacert);
    mbedtls_ssl_config_free(&tls_context->conf);
    mbedtls_ssl_free(&tls_context->ssl);
    mbedtls_net_free(&tls_context->server_fd);
}

bool connect(Hue &hue, uint8_t roomId)
{
    /*-------------------------------------------------*\
    | Signal the bridge to start streaming              |
    | If successful, connect to the UDP port            |
    \*-------------------------------------------------*/
    if (hue.startStreaming(roomId))
    {
        /*-------------------------------------------------*\
        | Connect to the Hue bridge UDP server              |
        \*-------------------------------------------------*/
        int ret = mbedtls_net_connect(
            &tls_context->server_fd, hue._hueBridgeBareIP.c_str(), "2100", MBEDTLS_NET_PROTO_UDP);

        /*-------------------------------------------------*\
        | If connecting failed, close and return false      |
        \*-------------------------------------------------*/
        if (ret != 0)
        {
            mbedtls_ssl_close_notify(&tls_context->ssl);
            Serial.println("Connect failed");
            return false;
        }

        /*-------------------------------------------------*\
        | Configure defaults                                |
        \*-------------------------------------------------*/
        ret = mbedtls_ssl_config_defaults(
            &tls_context->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);

        /*-------------------------------------------------*\
        | If configuring failed, close and return false     |
        \*-------------------------------------------------*/
        if (ret != 0)
        {
            mbedtls_ssl_close_notify(&tls_context->ssl);
            Serial.println("Config failed");
            return false;
        }

        mbedtls_ssl_conf_authmode(&tls_context->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_ssl_conf_ca_chain(&tls_context->conf, &tls_context->cacert, NULL);
        mbedtls_ssl_conf_rng(&tls_context->conf, mbedtls_ctr_drbg_random, &tls_context->ctr_drbg);

        /*-------------------------------------------------*\
        | Convert client key to binary array                |
        \*-------------------------------------------------*/
        Serial.println(hue._clientKey.c_str());
        std::vector<char> psk_binary = hexToBytes(hue._clientKey.c_str());

        /*-------------------------------------------------*\
        | Set up the SSL                                    |
        \*-------------------------------------------------*/
        ret = mbedtls_ssl_setup(&tls_context->ssl, &tls_context->conf);

        /*-------------------------------------------------*\
        | If setup failed, close and return false           |
        \*-------------------------------------------------*/
        if (ret != 0)
        {
            mbedtls_ssl_close_notify(&tls_context->ssl);
            Serial.println("SSL setup failed");
            return false;
        }

        ret = mbedtls_ssl_set_hostname(&tls_context->ssl, "localhost");

        /*-------------------------------------------------*\
        | If set hostname failed, close and return false    |
        \*-------------------------------------------------*/
        if (ret != 0)
        {
            mbedtls_ssl_close_notify(&tls_context->ssl);
            Serial.println("Hostname failed");
            return false;
        }

        mbedtls_ssl_set_bio(
            &tls_context->ssl, &tls_context->server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
        mbedtls_ssl_set_timer_cb(
            &tls_context->ssl, &tls_context->timer, my_mbedtls_timing_set_delay, my_mbedtls_timing_get_delay);
        mbedtls_net_set_nonblock(&tls_context->server_fd);

        /*-------------------------------------------------*\
        | Configure SSL pre-shared key and identity         |
        | PSK - binary array from client key                |
        | Identity - username (ASCII)                       |
        \*-------------------------------------------------*/
        ret = mbedtls_ssl_conf_psk(&tls_context->conf, (const unsigned char *)&psk_binary[0], psk_binary.size(),
                                   (const unsigned char *)hue._userId.c_str(), hue._userId.length());

        /*-------------------------------------------------*\
        | If configuring failed, close and return false     |
        \*-------------------------------------------------*/
        if (ret != 0)
        {
            Serial.println(&psk_binary[0]);
            Serial.println(hue._userId.c_str());
            mbedtls_ssl_close_notify(&tls_context->ssl);
            Serial.println("Config psk failed");
            handle_error_mbedtls(ret);
            return false;
        }
        /*-------------------------------------------------*\
        | Handshake                                         |
        \*-------------------------------------------------*/
        do
        {
            ret = mbedtls_ssl_handshake(&tls_context->ssl);
        } while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

        /*-------------------------------------------------*\
        | If set hostname failed, close and return false    |
        \*-------------------------------------------------*/
        if (ret != 0)
        {
            mbedtls_ssl_close_notify(&tls_context->ssl);
            Serial.println("Handshake failed");
            handle_error_mbedtls(ret);
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool disconnect()
{
    mbedtls_ssl_close_notify(&tls_context->ssl);
    free();
    return true;
}

bool setColorRGB(uint8_t light_index, uint8_t lightId, uint8_t red, uint8_t green, uint8_t blue)
{
    if (light_index < entertainment_num_lights)
    {
        unsigned int msg_idx = HUE_ENTERTAINMENT_HEADER_SIZE + (light_index * HUE_ENTERTAINMENT_LIGHT_SIZE);

        entertainment_msg[msg_idx + 0] = 0x00;           // Type (Light)
        entertainment_msg[msg_idx + 1] = lightId >> 8;   // ID MSB
        entertainment_msg[msg_idx + 2] = lightId & 0xFF; // ID LSB
        entertainment_msg[msg_idx + 3] = red;            // Red MSB
        entertainment_msg[msg_idx + 4] = red;            // Red LSB;
        entertainment_msg[msg_idx + 5] = green;          // Green MSB;
        entertainment_msg[msg_idx + 6] = green;          // Green LSB;
        entertainment_msg[msg_idx + 7] = blue;           // Blue MSB;
        entertainment_msg[msg_idx + 8] = blue;           // Blue LSB;

        return true;
    }
    else
    {
        return false;
    }
}

bool setColorRGBAndConvert(uint8_t light_index, uint8_t lightId, uint8_t red, uint8_t green, uint8_t blue)
{
    if (light_index < entertainment_num_lights)
    {
        unsigned int msg_idx = HUE_ENTERTAINMENT_HEADER_SIZE + (light_index * HUE_ENTERTAINMENT_LIGHT_SIZE);

        entertainment_msg[msg_idx + 0] = 0x00;           // Type (Light)
        entertainment_msg[msg_idx + 1] = lightId >> 8;   // ID MSB
        entertainment_msg[msg_idx + 2] = lightId & 0xFF; // ID LSB
        entertainment_msg[msg_idx + 3] = red >> 8;       // Red MSB
        entertainment_msg[msg_idx + 4] = red & 0xFF;     // Red LSB;
        entertainment_msg[msg_idx + 5] = green >> 8;     // Green MSB;
        entertainment_msg[msg_idx + 6] = green & 0xFF;   // Green LSB;
        entertainment_msg[msg_idx + 7] = blue >> 8;      // Blue MSB;
        entertainment_msg[msg_idx + 8] = blue & 0xFF;    // Blue LSB;

        return true;
    }
    else
    {
        return false;
    }
}

bool update()
{
    int ret;
    unsigned int total = 0;

    while (total < entertainment_msg.size())
    {
        ret = mbedtls_ssl_write(
            &tls_context->ssl, (const unsigned char *)&entertainment_msg[total], entertainment_msg.size());

        if (ret < 0)
        {
            // Return if mbedtls_ssl_write errors
            return false;
        }
        else
        {
            total += ret;
        }
    }
    return true;
}

void activateBehavior()
{
    startBehaviorNow = true;
    Serial.println("startBehaviorNow = " + String(startBehaviorNow));
}

void startBehavior(void *xStruct)
{
    for (;;)
    {
        while (startBehaviorNow)
        {
            Serial.println("startBehaviorNow = " + String(startBehaviorNow));
            initEntertainmentMode(*_hue, ENTERTAINMENT_ID, behaviors[lastBehaviorId].lightCounter);
            if (connect(*_hue, ENTERTAINMENT_ID))
            {
                if (behaviors[lastBehaviorId].behaviorId == repeatedBehavior)
                {
                    Serial.println("Starting training!");
                    for (int i = 0; i < behaviors[lastBehaviorId].entriesCount; i++)
                    {
                        int now = millis();
                        for (int l = 0; l < behaviors[lastBehaviorId].lightCounter; l++)
                        {
                            setColorRGB(l, behaviors[lastBehaviorId].lightIds[l], behaviors[lastBehaviorId].entries[i].command.r, behaviors[lastBehaviorId].entries[i].command.g, behaviors[lastBehaviorId].entries[i].command.b);
                        }
                        update();
                        while (millis() - now < behaviors[lastBehaviorId].delay * 10)
                        {
                            update();
                            vTaskDelay(50);
                        }
                    }
                }
                else if (behaviors[lastBehaviorId].behaviorId == timeBehavior)
                {
                    Serial.println("Starting alarm!");
                    int now = millis();
                    while (millis() - now < behaviors[lastBehaviorId].delay * 10)
                    {
                        update();
                        vTaskDelay(50);
                    }
                    for (int i = 0; i < behaviors[lastBehaviorId].entriesCount; i++)
                    {
                        for (int l = 0; l < behaviors[lastBehaviorId].lightCounter; l++)
                        {
                            setColorRGB(l, behaviors[lastBehaviorId].lightIds[l], behaviors[lastBehaviorId].entries[i].command.r, behaviors[lastBehaviorId].entries[i].command.g, behaviors[lastBehaviorId].entries[i].command.b);
                        }
                        update();
                        vTaskDelay(100);
                    }
                }
                else if (behaviors[lastBehaviorId].behaviorId == transitionBehavior)
                {
                    Serial.println("Starting party!");
                    fest = true;
                    while (fest)
                    {
                        for (int l = 0; l < behaviors[lastBehaviorId].lightCounter; l++)
                        {
                            setColorRGB(l, behaviors[lastBehaviorId].lightIds[l], random(0, 255), random(0, 255), random(0, 255));
                        }
                        update();
                        vTaskDelay(behaviors[lastBehaviorId].delay);
                    }
                }
                toggleBehavior(lastBehaviorId, 5); //Toggle behavior as if pressing on STOP;
                startBehaviorNow = false;
            }
            else
            {
                Serial.println("Connect failed");
            }
            toggleBehavior(lastBehaviorId, 5); //Toggle behavior as if pressing on STOP;
            startBehaviorNow = false;
        }
        vTaskDelay(100);
    }
}

void createTransitionBehavior(Hue &hue, Light lightIds[])
{
    Behavior transitionLights;
    transitionLights.behaviorId = transitionBehavior;
    transitionLights.enterainmentGroup = hue._entertainmentGroup;
    transitionLights.name = "FEST";
    transitionLights.activated = false;
    transitionLights.delay = 1000 / festDelay / 10;
    transitionLights.lightCounter = 0;
    transitionLights.entriesCount = 100;
    behaviors[2] = transitionLights;
}

void createTimebasedBehavior(Hue &hue, Light lightIds[])
{
    Behavior timebasedLights;
    timebasedLights.behaviorId = timeBehavior;
    timebasedLights.enterainmentGroup = hue._entertainmentGroup;
    timebasedLights.name = "ALARM";
    timebasedLights.activated = false;
    timebasedLights.delay = alarmDelay / 10;
    timebasedLights.lightCounter = 0;
    timebasedLights.entriesCount = alarmEntries;
    for (int i = 0; i < timebasedLights.entriesCount; i++)
    {
        if (i % 2 == 0)
        {
            timebasedLights.entries[i].command.r = 0xff;
            timebasedLights.entries[i].command.g = 0x00;
            timebasedLights.entries[i].command.b = 0x00;
        }
        else
        {
            timebasedLights.entries[i].command.r = 0xfc;
            timebasedLights.entries[i].command.g = 0xc5;
            timebasedLights.entries[i].command.b = 0x73;
        }
    }
    behaviors[1] = timebasedLights;
}

void createRepeatingBehavior(Hue &hue, Light lightIds[])
{
    //Create blinking lights
    Behavior blinkingLights;
    blinkingLights.behaviorId = repeatedBehavior;
    blinkingLights.enterainmentGroup = hue._entertainmentGroup;
    blinkingLights.name = "TRÆNING";
    blinkingLights.activated = false;
    blinkingLights.lightCounter = 0;
    blinkingLights.entriesCount = blinkingLightsEntries;
    blinkingLights.delay = blinkingLightsDelay / 10;
    for (int i = 0; i < blinkingLights.entriesCount; i++)
    {
        if (i % 2 == 0)
        {
            blinkingLights.entries[i].command.r = 0xff;
            blinkingLights.entries[i].command.g = 0x00;
            blinkingLights.entries[i].command.b = 0x00;
        }
        else
        {
            blinkingLights.entries[i].command.r = 0xfc;
            blinkingLights.entries[i].command.g = 0xc5;
            blinkingLights.entries[i].command.b = 0x73;
        }
    }
    behaviors[0] = blinkingLights;
}

void updateBlinkingLightsEntries()
{
    for (int i = 0; i < behaviors[0].entriesCount; i++)
    {
        if (i % 2 == 0)
        {
            behaviors[0].entries[i].command.r = 0xff;
            behaviors[0].entries[i].command.g = 0x00;
            behaviors[0].entries[i].command.b = 0x00;
        }
        else
        {
            behaviors[0].entries[i].command.r = 0xfc;
            behaviors[0].entries[i].command.g = 0xc5;
            behaviors[0].entries[i].command.b = 0x73;
        }
    }
}

void updateAlarmLightsEntries()
{
    for (int i = 0; i < behaviors[1].entriesCount; i++)
    {
        if (i % 2 == 0)
        {
            behaviors[1].entries[i].command.r = 0xff;
            behaviors[1].entries[i].command.g = 0x00;
            behaviors[1].entries[i].command.b = 0x00;
        }
        else
        {
            behaviors[1].entries[i].command.r = 0xfc;
            behaviors[1].entries[i].command.g = 0xc5;
            behaviors[1].entries[i].command.b = 0x73;
        }
    }
}

void initBehaviors(Hue &hue, Light lightIds[])
{
    createRepeatingBehavior(hue, lightIds);
    createTimebasedBehavior(hue, lightIds);
    createTransitionBehavior(hue, lightIds);
    behaviorCount = 3;
}
