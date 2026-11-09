/*
 * mqtt_client.c
 *
 * Lightweight MQTT 3.1.1 client.
 */

#include "mqtt_client.h"
#include <string.h>
#include <stdio.h>
#include "serial_mwm.h"
#include "fsl_debug_console.h"

/* Kody pierwszego bajtu naglowka pakietow protokolu binarnego MQTT 3.1.1 */
#define MQTT_PKT_CONNECT     0x10
#define MQTT_PKT_CONNACK     0x20
#define MQTT_PKT_PUBLISH     0x30
#define MQTT_PKT_PUBACK      0x40
#define MQTT_PKT_SUBSCRIBE   0x82
#define MQTT_PKT_SUBACK      0x90
#define MQTT_PKT_PINGREQ     0xC0
#define MQTT_PKT_PINGRESP    0xD0
#define MQTT_PKT_DISCONNECT  0xE0

/** Bufor nadwaczy */
static uint8_t tx_buffer[MQTT_MAX_PACKET_SIZE];
/** Bufor odbiorczy */
static uint8_t rx_buffer[MQTT_MAX_PACKET_SIZE];

/**
 * Koduje dlugosc pozostalej czesci pakietu (Remaining Length) zgodnie ze standardem MQTT.
 * Uzywa kodowania o zmiennej dlugosci (od 1 do 4 bajtow), gdzie najstarszy bit (0x80)
 * informuje, czy liczba jest kontynuowana w kolejnym bajcie.
 */
static int encode_remaining_length(uint8_t *buf, uint32_t length) {
    int bytes = 0;
    do {
        uint8_t digit = length % 128;
        length /= 128;
        if (length > 0) {
            digit |= 0x80;
        }
        buf[bytes++] = digit;
    } while (length > 0);
    return bytes;
}
/**
 * Dekoduje pole dlugosci pakietu (Remaining Length) ze standardu MQTT na liczbe calkowita.
 * Odczytuje od 1 do 4 bajtow ze wskazanego bufora i przesuwa wskaznik offsetu do miejsca,
 * w ktorym zaczynaja sie wlasciwe dane pakietu.
 */
static int decode_remaining_length(const uint8_t *buf, int *offset, int total_len) {
    int multiplier = 1;
    int value = 0;
    uint8_t digit;
    do {
        if (*offset >= total_len) return -1;
        digit = buf[(*offset)++];
        value += (digit & 0x7F) * multiplier;
        multiplier *= 128;
        if (multiplier > 128 * 128 * 128) return -1;
    } while ((digit & 0x80) != 0);
    return value;
}

void mqtt_client_init(mqtt_client_t *client, const char *host, uint16_t port, const char *client_id) {
    if (!client) return;
    memset(client, 0, sizeof(mqtt_client_t));
    strncpy(client->host, host, sizeof(client->host) - 1);
    client->port = port;
    strncpy(client->client_id, client_id, sizeof(client->client_id) - 1);
    client->socket = -1;
    client->connected = false;
    client->sub_count = 0;
}

void mqtt_client_disconnect(mqtt_client_t *client) {
    if (!client) return;
    if (client->socket >= 0) {
        if (client->connected) {
            uint8_t disc_pkt[2] = { MQTT_PKT_DISCONNECT, 0x00 };
            mwm_send(client->socket, disc_pkt, 2);
        }
        mwm_close(client->socket);
        client->socket = -1;
    }
    client->connected = false;
    PRINTF("[MQTT] Disconnected\r\n");
}

int mqtt_client_connect(mqtt_client_t *client) {
    if (!client) return -1;
    if (client->connected) {
        mqtt_client_disconnect(client);
    }

    PRINTF("[MQTT] Connecting to %s:%d...\r\n", client->host, client->port);

    client->socket = mwm_socket(MWM_TCP);
    if (client->socket < 0) {
        PRINTF("[MQTT] Error opening TCP socket: %d\r\n", client->socket);
        return -1;
    }

    mwm_sockaddr_t srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    strncpy(srv_addr.host, client->host, sizeof(srv_addr.host) - 1);
    srv_addr.port = client->port;

    int ret = mwm_connect(client->socket, &srv_addr, sizeof(srv_addr));
    if (ret != 0) {
        PRINTF("[MQTT] TCP connect failed: %d\r\n", ret);
        mwm_close(client->socket);
        client->socket = -1;
        return -2;
    }

    uint16_t client_id_len = (uint16_t)strlen(client->client_id);
    uint32_t var_header_len = 10;
    uint32_t payload_len = 2 + client_id_len;
    uint32_t remaining_len = var_header_len + payload_len;

    int idx = 0;
    tx_buffer[idx++] = MQTT_PKT_CONNECT;
    idx += encode_remaining_length(&tx_buffer[idx], remaining_len);


    tx_buffer[idx++] = 0x00;
    tx_buffer[idx++] = 0x04;
    tx_buffer[idx++] = 'M';
    tx_buffer[idx++] = 'Q';
    tx_buffer[idx++] = 'T';
    tx_buffer[idx++] = 'T';
    tx_buffer[idx++] = 0x04;
    tx_buffer[idx++] = 0x02;
    tx_buffer[idx++] = (MQTT_DEFAULT_KEEPALIVE_SEC >> 8) & 0xFF;
    tx_buffer[idx++] = MQTT_DEFAULT_KEEPALIVE_SEC & 0xFF;


    tx_buffer[idx++] = (client_id_len >> 8) & 0xFF;
    tx_buffer[idx++] = client_id_len & 0xFF;
    memcpy(&tx_buffer[idx], client->client_id, client_id_len);
    idx += client_id_len;

    ret = mwm_send(client->socket, tx_buffer, idx);
    if (ret != idx) {
        PRINTF("[MQTT] Failed to send CONNECT packet (%d)\r\n", ret);
        mqtt_client_disconnect(client);
        return -3;
    }


    ret = mwm_recv_timeout(client->socket, rx_buffer, 4, 5000);
    if (ret < 4) {
        PRINTF("[MQTT] CONNACK timeout or error: %d\r\n", ret);
        mqtt_client_disconnect(client);
        return -4;
    }

    if (rx_buffer[0] != MQTT_PKT_CONNACK || rx_buffer[3] != 0x00) {
        PRINTF("[MQTT] Broker rejected connection: code 0x%02X\r\n", rx_buffer[3]);
        mqtt_client_disconnect(client);
        return -5;
    }

    client->connected = true;
    PRINTF("[MQTT] Connected successfully as '%s'!\r\n", client->client_id);
    return 0;
}

static int send_subscribe_packet(mqtt_client_t *client, const char *topic) {
    uint16_t topic_len = (uint16_t)strlen(topic);
    uint32_t remaining_len = 2 /* packet ID */ + 2 /* topic len */ + topic_len + 1 /* QoS 0 */;

    int idx = 0;
    tx_buffer[idx++] = MQTT_PKT_SUBSCRIBE;
    idx += encode_remaining_length(&tx_buffer[idx], remaining_len);

    tx_buffer[idx++] = 0x00;
    tx_buffer[idx++] = 0x01;

    tx_buffer[idx++] = (topic_len >> 8) & 0xFF;
    tx_buffer[idx++] = topic_len & 0xFF;
    memcpy(&tx_buffer[idx], topic, topic_len);
    idx += topic_len;
    tx_buffer[idx++] = 0x00;

    int ret = mwm_send(client->socket, tx_buffer, idx);
    if (ret != idx) {
        PRINTF("[MQTT] Failed to send SUBSCRIBE packet\r\n");
        return -2;
    }


    ret = mwm_recv_timeout(client->socket, rx_buffer, 5, 3000);
    if (ret < 5 || rx_buffer[0] != MQTT_PKT_SUBACK || rx_buffer[4] == 0x80) {
        PRINTF("[MQTT] SUBACK failed: ret=%d, code=0x%02X\r\n", ret, ret >= 5 ? rx_buffer[4] : 0xFF);
        return -3;
    }

    PRINTF("[MQTT] Subscribed to '%s' (QoS %d)\r\n", topic, rx_buffer[4]);
    return 0;
}

int mqtt_client_subscribe(mqtt_client_t *client, const char *topic, mqtt_message_callback_t callback) {
    if (!client || !topic) return -1;

    int existing_idx = -1;
    for (int i = 0; i < client->sub_count; i++) {
        if (strcmp(client->subscriptions[i].topic, topic) == 0) {
            existing_idx = i;
            break;
        }
    }

    if (existing_idx >= 0) {
        client->subscriptions[existing_idx].callback = callback;
    } else if (client->sub_count < MQTT_MAX_SUBSCRIPTIONS) {
        strncpy(client->subscriptions[client->sub_count].topic, topic, sizeof(client->subscriptions[client->sub_count].topic) - 1);
        client->subscriptions[client->sub_count].callback = callback;
        client->sub_count++;
    } else {
        PRINTF("[MQTT] Max subscriptions reached!\r\n");
        return -4;
    }

    if (client->connected) {
        return send_subscribe_packet(client, topic);
    }

    return 0;
}

int mqtt_client_publish(mqtt_client_t *client, const char *topic, const char *payload) {
    if (!client || !client->connected || !topic || !payload) return -1;

    uint16_t topic_len = (uint16_t)strlen(topic);
    uint32_t payload_len = (uint32_t)strlen(payload);
    uint32_t remaining_len = 2 + topic_len + payload_len;

    int idx = 0;
    tx_buffer[idx++] = MQTT_PKT_PUBLISH;
    idx += encode_remaining_length(&tx_buffer[idx], remaining_len);

    tx_buffer[idx++] = (topic_len >> 8) & 0xFF;
    tx_buffer[idx++] = topic_len & 0xFF;
    memcpy(&tx_buffer[idx], topic, topic_len);
    idx += topic_len;


    memcpy(&tx_buffer[idx], payload, payload_len);
    idx += payload_len;

    int ret = mwm_send(client->socket, tx_buffer, idx);
    if (ret != idx) {
        PRINTF("[MQTT] Publish failed: %d\r\n", ret);
        return -2;
    }

    return 0;
}

int mqtt_client_poll(mqtt_client_t *client, uint32_t current_time_ms) {
    if (!client) return -1;

    if (!client->connected) {
        PRINTF("[MQTT] Connection lost, retrying...\r\n");
        if (mqtt_client_connect(client) == 0) {
            for (int i = 0; i < client->sub_count; i++) {
                send_subscribe_packet(client, client->subscriptions[i].topic);
            }
            client->last_ping_ms = current_time_ms;
        } else {
            return -1;
        }
    }

    if (current_time_ms - client->last_ping_ms >= MQTT_PING_INTERVAL_MS) {
        uint8_t ping_pkt[2] = { MQTT_PKT_PINGREQ, 0x00 };
        int ret = mwm_send(client->socket, ping_pkt, 2);
        if (ret != 2) {
            PRINTF("[MQTT] Failed to send PINGREQ, resetting connection\r\n");
            mqtt_client_disconnect(client);
            return -1;
        }
        client->last_ping_ms = current_time_ms;
    }

    int ret = mwm_recv_timeout(client->socket, rx_buffer, sizeof(rx_buffer) - 1, 50);
    if (ret <= 0) {
        return 0;
    }

    rx_buffer[ret] = '\0';
    uint8_t pkt_type = rx_buffer[0] & 0xF0;

    if (pkt_type == MQTT_PKT_PINGRESP) {
        return 0;
    }

    if (pkt_type == MQTT_PKT_PUBLISH) {
        int offset = 1;
        int rem_len = decode_remaining_length(rx_buffer, &offset, ret);
        if (rem_len < 0 || offset >= ret) return 0;

        uint8_t qos = (rx_buffer[0] >> 1) & 0x03;

        uint16_t topic_len = (rx_buffer[offset] << 8) | rx_buffer[offset + 1];
        offset += 2;
        if (offset + topic_len > ret) return 0;

        char topic[64];
        int copy_len = (topic_len < sizeof(topic) - 1) ? topic_len : (sizeof(topic) - 1);
        memcpy(topic, &rx_buffer[offset], copy_len);
        topic[copy_len] = '\0';
        offset += topic_len;

        if (qos > 0) {
            uint16_t packet_id = (rx_buffer[offset] << 8) | rx_buffer[offset + 1];
            offset += 2;

            if (qos == 1) {
                uint8_t puback[4] = { MQTT_PKT_PUBACK, 0x02, (uint8_t)(packet_id >> 8), (uint8_t)(packet_id & 0xFF) };
                mwm_send(client->socket, puback, 4);
            }
        }

        int payload_len = ret - offset;
        if (payload_len >= 0) {
            char payload[MQTT_MAX_PACKET_SIZE];
            int p_copy = (payload_len < sizeof(payload) - 1) ? payload_len : (sizeof(payload) - 1);
            memcpy(payload, &rx_buffer[offset], p_copy);
            payload[p_copy] = '\0';

            PRINTF("[MQTT] Message on '%s': %s\r\n", topic, payload);

            for (int i = 0; i < client->sub_count; i++) {
                if (strcmp(client->subscriptions[i].topic, topic) == 0) {
                    if (client->subscriptions[i].callback) {
                        client->subscriptions[i].callback(topic, payload);
                    }
                    break;
                }
            }
        }
    }

    return 0;
}
