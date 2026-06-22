/*
 * Copyright 2021-2023 Morse Micro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mmwlan.h"
#include "mmipal.h"
#include "mmosal.h"
#include "mm_app_loadconfig.h"
#include "mmwlan_regdb.def"
#include "camera_build_config.h"

#define COUNTRY_CODE HALOW_COUNTRY
#ifndef COUNTRY_CODE
#error COUNTRY_CODE must be defined to the appropriate 2 character country code.
#endif

/*
 * HaLow credentials (HT-HC33 defaults). Override at build:
 *   idf.py -DHALOW_SSID=haven -DHALOW_PASSPHRASE=havenmesh build
 *   idf.py -DOPENMANET_MESH=1 build   # preset: haven / havenmesh
 *
 * OPENMANET: mesh ID "haven" is not always the joinable SSID in scan — use the
 * name your gate shows (often grey-AP on Heltec/HAVEN setups).
 */
#ifdef APP_HALOW_SSID
#define DEFAULT_HALOW_SSID APP_HALOW_SSID
#else
#define DEFAULT_HALOW_SSID HALOW_SSID
#endif

#ifdef APP_HALOW_PASSPHRASE
#define DEFAULT_HALOW_PASSPHRASE APP_HALOW_PASSPHRASE
#else
#define DEFAULT_HALOW_PASSPHRASE HALOW_PASSPHRASE
#endif

#ifndef SECURITY_TYPE
#define SECURITY_TYPE MMWLAN_SAE
#endif

void load_mmipal_init_args(struct mmipal_init_args *args)
{
    (void)mmosal_safer_strcpy(args->ip_addr, STATIC_IP, sizeof(args->ip_addr));
    (void)mmosal_safer_strcpy(args->netmask, STATIC_NETMASK, sizeof(args->netmask));
    (void)mmosal_safer_strcpy(args->gateway_addr, STATIC_GATEWAY, sizeof(args->gateway_addr));

#if USE_STATIC_IP
    args->mode = MMIPAL_STATIC;
    printf("IPv4: static %s gw %s\n", STATIC_IP, STATIC_GATEWAY);
#else
    args->mode = MMIPAL_DHCP;
    printf("IPv4: DHCP\n");
#endif

    args->ip6_mode = MMIPAL_IP6_AUTOCONFIG;
    args->ip6_addr[0] = '\0';
}

const struct mmwlan_s1g_channel_list *load_channel_list(void)
{
    char strval[16];
    const struct mmwlan_s1g_channel_list *channel_list;

    (void)mmosal_safer_strcpy(strval, COUNTRY_CODE, sizeof(strval));
    channel_list = mmwlan_lookup_regulatory_domain(get_regulatory_db(), strval);
    if (channel_list == NULL) {
        printf("Unknown country code: %s\n", strval);
        MMOSAL_ASSERT(false);
    }
    return channel_list;
}

void load_mmwlan_sta_args(struct mmwlan_sta_args *sta_config)
{
    (void)mmosal_safer_strcpy((char *)sta_config->ssid, DEFAULT_HALOW_SSID, sizeof(sta_config->ssid));
    sta_config->ssid_len = strlen((char *)sta_config->ssid);

    (void)mmosal_safer_strcpy(sta_config->passphrase, DEFAULT_HALOW_PASSPHRASE,
                              sizeof(sta_config->passphrase));
    sta_config->passphrase_len = strlen(sta_config->passphrase);
    sta_config->security_type = SECURITY_TYPE;
}

void load_mmwlan_settings(void)
{
    mmwlan_set_power_save_mode(MMWLAN_PS_DISABLED);
}

const char *load_halow_ssid_default(void)
{
    return DEFAULT_HALOW_SSID;
}
