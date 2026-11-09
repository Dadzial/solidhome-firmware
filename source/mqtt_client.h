/*
* mqtt_client.h
*
* Klient MQTT 3.1.1.
*/


#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include <stdint.h>
#include <stdbool.h>

#define MQTT_MAX_PACKET_SIZE 1024
#define MQTT_DEFAULT_KEEPALIVE_SEC 60
#define MQTT_PING_INTERVAL_MS 25000
#define MQTT_MAX_SUBSCRIPTIONS 8

/* Typ funkcji zwrotnej wywolywanej po odebraniu wiadomosci */
typedef void (*mqtt_message_callback_t)(const char *topic, const char *payload);

/* Pojedyncza subskrypcja: nazwa tematu oraz przypisana funkcja obslugi */
typedef struct {
    char topic[64];
    mqtt_message_callback_t callback;
} mqtt_subscription_t;

/* Glowna struktura klienta MQTT */
typedef struct {
    char host[64];
    uint16_t port;
    char client_id[32];
    int socket;
    bool connected;
    uint32_t last_activity_ms;
    uint32_t last_ping_ms;
    mqtt_subscription_t subscriptions[MQTT_MAX_SUBSCRIPTIONS];
    uint8_t sub_count;
} mqtt_client_t;

/**
* Inicjalizuje strukture klienta MQTT domyslnymi wartosciami.
*/
void mqtt_client_init(mqtt_client_t *client, const char *host, uint16_t port, const char *client_id);

/**
 * Connect to MQTT broker (TCP connect + MQTT CONNECT packet).
 * Returns 0 on success, negative on error.
 */
int mqtt_client_connect(mqtt_client_t *client);

/**
* Rozlacza z brokerem i zamyka gniazdo sieciowe TCP.
*/
void mqtt_client_disconnect(mqtt_client_t *client);

/**
* Subskrybuje wybrany temat z QoS 0 i rejestruje funkcje zwrotna dla tego tematu.
* Zwraca 0 w przypadku sukcesu, wartosc ujemna w przypadku bledu.
*/
int mqtt_client_subscribe(mqtt_client_t *client, const char *topic, mqtt_message_callback_t callback);

/**
* Publikuje wiadomosc na podany temat z QoS 0.
* Zwraca 0 w przypadku sukcesu, wartosc ujemna w przypadku bledu.
*/
int mqtt_client_publish(mqtt_client_t *client, const char *topic, const char *payload);

/**
* Odbiera przychodzace pakiety MQTT, obsluguje utrzymanie polaczenia (PINGREQ keep-alive),
* przekazuje wiadomosci do zarejestrowanych funkcji zwrotnych oraz wznawia polaczenie w razie zerwania.
* Powinna byc wywolywana cyklicznie w glownej petli programu.
* Zwraca 0 przy normalnej pracy, wartosc ujemna w przypadku bledu krytycznego.
*/
int mqtt_client_poll(mqtt_client_t *client, uint32_t current_time_ms);

#endif /* MQTT_CLIENT_H_ */
