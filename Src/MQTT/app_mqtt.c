#include <string.h>
#include "flash_config_data.h"
#include "app_mqtt.h"

char    g_HEARTBEAT_TOPIC[ sizeof(HEARTBEAT_TOPIC_FMT) + 2 ];
uint8_t g_HEARTBEAT_TOPIC_LEN;

char    g_DATA_TOPIC[ sizeof(DATA_TOPIC_FMT) + 2 ];
uint8_t g_DATA_TOPIC_LEN;

char    g_CONN_REQ_TOPIC[ sizeof(CONN_REQ_TOPIC_FMT) + 2 ];
uint8_t g_CONN_REQ_TOPIC_LEN;

char    g_CONN_RESP_TOPIC[ sizeof(CONN_RESP_TOPIC_FMT) + 2 ];
uint8_t g_CONN_RESP_TOPIC_LEN;

char    g_NI_CONFIG_TOPIC[ sizeof(NI_CONFIG_TOPIC_FMT) + 2 ];
uint8_t g_NI_CONFIG_TOPIC_LEN;

char    g_BASE_SELF_CONFIG_TOPIC[ sizeof(BASE_SELF_CONFIG_TOPIC_FMT) + 2 ];
uint8_t g_BASE_SELF_CONFIG_TOPIC_LEN;

char    g_DFU_TOPIC[ sizeof(DFU_TOPIC_FMT) + 2 ];
uint8_t g_DFU_TOPIC_LEN;


void AppMqtt_Init(void)
{
    g_HEARTBEAT_TOPIC_LEN = snprintf( g_HEARTBEAT_TOPIC,
                                      sizeof(g_HEARTBEAT_TOPIC),
                                      HEARTBEAT_TOPIC_FMT,
                                      gp_persistent_conf->anchor_id );

    g_DATA_TOPIC_LEN = snprintf( g_DATA_TOPIC,
                                 sizeof(g_DATA_TOPIC),
                                 DATA_TOPIC_FMT,
                                 gp_persistent_conf->anchor_id );

    g_CONN_REQ_TOPIC_LEN = snprintf( g_CONN_REQ_TOPIC,
                                     sizeof(g_CONN_REQ_TOPIC),
                                     CONN_REQ_TOPIC_FMT,
                                     gp_persistent_conf->anchor_id );

    g_CONN_RESP_TOPIC_LEN = snprintf( g_CONN_RESP_TOPIC,
                                      sizeof(g_CONN_RESP_TOPIC),
                                      CONN_RESP_TOPIC_FMT,
                                      gp_persistent_conf->anchor_id );

    g_NI_CONFIG_TOPIC_LEN = snprintf( g_NI_CONFIG_TOPIC,
                                      sizeof(g_NI_CONFIG_TOPIC),
                                      NI_CONFIG_TOPIC_FMT,
                                      gp_persistent_conf->anchor_id );

    g_BASE_SELF_CONFIG_TOPIC_LEN = snprintf( g_BASE_SELF_CONFIG_TOPIC,
                                             sizeof(g_BASE_SELF_CONFIG_TOPIC),
                                             BASE_SELF_CONFIG_TOPIC_FMT,
                                             gp_persistent_conf->anchor_id );

    g_DFU_TOPIC_LEN = snprintf( g_DFU_TOPIC,
                                sizeof(g_DFU_TOPIC),
                                DFU_TOPIC_FMT,
                                gp_persistent_conf->anchor_id );
}
