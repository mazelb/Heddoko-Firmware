/**
 ****************************************************************************************
 *
 * @file app_util.c
 *
 * @brief Application Utility API
 *
 * Copyright (C) Quintic 2012-2014
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

#include "app_env.h"
#if !QN_WORK_MODE
#include "qnrf.h"
#endif

#if (defined(CFG_EACI))
extern struct device_name_set device_name;
#endif

/**
 ****************************************************************************************
 * @brief Convert u32 value to array with Hex format
 *
 ****************************************************************************************
 */
void app_i_to_array(uint32_t num, char *array)
{
    int key_len = KEY_LEN;
    uint8_t base = 16;
    
    while (num != 0)
    {
        array[--key_len] = (uint8_t)(num % base);
        num = num / base;
        
        array[key_len] |= ((uint8_t)(num % base) << 4) & 0xF0;
        num = num / base;
    }
}

/**
 ****************************************************************************************
 * @brief Set local device role
 *
 ****************************************************************************************
 */
void app_set_role(uint8_t role)
{
    // should be GAP_CENTRAL_MST, GAP_PERIPHERAL_SLV, GAP_BROADCASTER_ADV or GAP_OBSERVER_SCA
    app_env.role = role;
}

/**
 ****************************************************************************************
 * @brief Set local device role
 *
 ****************************************************************************************
 */
uint8_t app_get_role(void)
{
    return app_env.role;
}

/**
 ****************************************************************************************
 * @brief Get Device Record Index by connection handle
 *
 ****************************************************************************************
 */
#if (BLE_CENTRAL)
uint8_t app_get_rec_idx_by_conhdl(uint16_t conhdl)
{
    uint8_t idx;

    for (idx = 0; idx < BLE_CONNECTION_MAX; idx++)
    {
        if (app_env.dev_rec[idx].conhdl == conhdl)
            return idx;
    }

    idx = GAP_INVALID_CONIDX;
    return idx;
}
#endif

/**
 ****************************************************************************************
 * @brief Get Connection Handle by Index 
 *
 ****************************************************************************************
 */
uint16_t app_get_conhdl_by_idx(uint8_t idx)
{
    if (app_get_link_status_by_idx(idx))
        return app_env.dev_rec[idx].conhdl;
    else
        return 0xFFFF;
}

/**
 ****************************************************************************************
 * @brief Get Connection Handle and Index by BD address
 *
 ****************************************************************************************
 */
uint16_t app_get_conhdl_by_bdaddr(struct bd_addr const *addr, uint8_t *idx)
{
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (co_bt_bdaddr_compare(addr, &app_env.dev_rec[i].bonded_info.peer_addr) == true)
        {
            ASSERT_ERR(app_env.dev_rec[i].free == false);
            *idx = i;

            return app_env.dev_rec[i].conhdl;
        }
    }

    *idx = GAP_INVALID_CONIDX;
    return 0xFFFF;
}

/**
 ****************************************************************************************
 * @brief Get BD Addr by connection handle
 *
 ****************************************************************************************
 */
uint8_t app_get_bd_addr_by_conhdl(uint16_t conhdl, struct bd_addr *addr)
{
    uint8_t idx = GAP_INVALID_CONIDX;

    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (app_env.dev_rec[i].conhdl == conhdl)
        {
            if (addr)
                *addr = app_env.dev_rec[i].bonded_info.peer_addr;
            idx = i;
            break;
        }
    }

    return idx;
}        

/**
 ****************************************************************************************
 * @brief Get BD Addr by index
 *
 ****************************************************************************************
 */
#if (BLE_CENTRAL)
bool app_get_bd_addr_by_idx(uint8_t idx, struct bd_addr *addr)
{
    bool r = false;
    
    if (addr && app_get_link_status_by_idx(idx))
    {
        *addr = app_env.dev_rec[idx].bonded_info.peer_addr;
        r = true;
    }
    
    return r;
}        
#endif

/**
 ****************************************************************************************
 * @brief Get link security status by device address
 *
 ****************************************************************************************
 */
#if (QN_SECURITY_ON)
uint8_t app_get_lk_sec_status(struct bd_addr const *addr)
{
    uint8_t idx;

    if ((idx = app_find_bonded_dev(addr)) != GAP_INVALID_CONIDX)
    {
        if (app_env.bonded_info[idx].sec_prop == SMP_KSEC_UNAUTH_NO_MITM)
            return GAP_LK_UNAUTHENTICATED;
        else if (app_env.bonded_info[idx].sec_prop == SMP_KSEC_AUTH_MITM)
            return GAP_LK_AUTHENTICATED;
    }

    return GAP_LK_SEC_NONE;
}
#endif

/**
 ****************************************************************************************
 * @brief Get Connection status by Idx
 *
 ****************************************************************************************
 */
bool app_get_link_status_by_idx(uint8_t idx)
{
    return (bool)(app_env.dev_rec[idx].free == false);
}

/**
 ****************************************************************************************
 * @brief Set Connection status by Connection handle
 *
 ****************************************************************************************
 */
void app_set_link_status_by_conhdl(uint16_t conhdl, struct gap_link_info const *conn_info, bool conn)
{
    if (conn == true)
    {
#if (BLE_CENTRAL)
        uint8_t idx = app_get_rec_idx_by_conhdl(0xFFFF);
#else
        uint8_t idx = 0;
#endif
        if (idx != GAP_INVALID_CONIDX)
        {
            app_env.dev_rec[idx].free = false;
            app_env.dev_rec[idx].conhdl = conhdl;
            app_env.dev_rec[idx].bonded_info.addr_type = conn_info->peer_addr_type;
            app_env.dev_rec[idx].bonded_info.peer_addr = conn_info->peer_addr;
            app_env.cn_count++;
        }
    }
    else
    {
        uint8_t idx = app_get_bd_addr_by_conhdl(conhdl, NULL);
        if (idx != GAP_INVALID_CONIDX)
        {
            app_env.dev_rec[idx].free = true;
            app_env.dev_rec[idx].conhdl = 0xFFFF;
            memset(&app_env.dev_rec[idx].bonded_info, 0, sizeof(struct app_bonded_info));
            app_env.cn_count--;
        }
    }
}

/**
 ****************************************************************************************
 * @brief Get the connection number of the device record
 *
 ****************************************************************************************
 */
#if (BLE_CENTRAL)
uint8_t app_get_link_nb(void)
{
    return app_env.cn_count;
}
#endif

/**
 ****************************************************************************************
 * @brief Get the bonded number of the device record
 *
 ****************************************************************************************
 */
#if (QN_SECURITY_ON)
uint8_t app_get_bond_nb(void)
{
    return app_env.bonded_count;
}
#endif

/**
 ****************************************************************************************
 * @brief Get the bonded staus of the specific device record
 *
 ****************************************************************************************
 */
#if (QN_SECURITY_ON)
bool app_get_bond_status(uint8_t idx, uint8_t key_flag)
{
    return app_env.bonded_info[idx].sec_prop && (app_env.bonded_info[idx].peer_distribute_keys & key_flag);
}
#endif

/**
 ****************************************************************************************
 * @brief Add the bonded device
 *
 ****************************************************************************************
 */
#if (QN_SECURITY_ON)
bool app_add_bonded_dev(void *bonded_dev)
{
    if (app_env.bonded_count < APP_MAX_BONDED_DEVICE_NUMBER)
    {
        app_env.bonded_info[app_env.bonded_count++] = *(struct app_bonded_info *)bonded_dev;
#if (QN_NVDS_WRITE)
        if (NVDS_OK == nvds_put(APP_NVDS_DB_START_TAG + app_env.bonded_count - 1, sizeof(struct app_bonded_info), (uint8_t *)bonded_dev))
        {
            uint8_t num = app_env.bonded_count;
            if (NVDS_OK == nvds_put(APP_NVDS_DB_COUNT_TAG, sizeof(num), &num))
            {
                return true;
            }
        }
        app_env.bonded_count--;
        return false;
#endif
    }
    else
    {
#if (QN_NVDS_WRITE)
        // Replace one item
        nvds_tag_len_t length = sizeof(struct app_bonded_info);

        for (uint8_t i = 1; i < (app_env.bonded_count - 1); i++)
        {
            if (NVDS_OK != nvds_put(APP_NVDS_DB_COUNT_TAG + i - 1, length, (uint8_t *)&app_env.bonded_info[i]))
            {
                return false;
            }
            app_env.bonded_info[i - 1] = app_env.bonded_info[i];
        }
        if (NVDS_OK != nvds_put(APP_NVDS_DB_START_TAG + app_env.bonded_count - 1, length, (uint8_t *)bonded_dev))
        {
            return false;
        }
        app_env.bonded_info[app_env.bonded_count - 1] = *(struct app_bonded_info *)bonded_dev;
#endif
    }
    return true;
}
#endif

/**
 ****************************************************************************************
 * @brief Find the bonded device by device address
 *
 ****************************************************************************************
 */
#if (QN_SECURITY_ON)
uint8_t app_find_bonded_dev(struct bd_addr const *addr)
{
    int i;

    for (i = 0; i < app_env.bonded_count; i++)
    {
        if (true == co_bt_bdaddr_compare(addr, &app_env.bonded_info[i].peer_addr))
        {
            return i;
        }
    }
    return GAP_INVALID_CONIDX;
}
#endif

/**
 ****************************************************************************************
 * @brief Check Service setup FLAG and Initiate SMP IRK and CSRK
 *
 ****************************************************************************************
 */
#if (QN_SECURITY_ON)
void app_init_local_smp_key(void)
{
    // Set key here
    // IRK firstly, then CSRK
    struct smp_key new_key;
    memcpy(new_key.key, app_env.irk, KEY_LEN);
    app_smpm_set_key_req(SMP_KDIST_IDKEY, &new_key);
}
#endif

/**
 ****************************************************************************************
 * @brief Get the local service setup inital value
 *
 ****************************************************************************************
 */
#if (BLE_PERIPHERAL)
uint16_t app_get_local_service_flag(void)
{
    uint16_t srv_flag = 0;
    
#if BLE_DIS_SERVER
    srv_flag |= BLE_DIS_SERVER_BIT;
#endif

    return srv_flag;
}
#endif

/**
 ****************************************************************************************
 * @brief Clear Service setup FLAG
 *
 ****************************************************************************************
 */
#if (BLE_PERIPHERAL)
void app_clear_local_service_flag(uint16_t srv_bit)
{
    app_env.srv_flag &= ~srv_bit;
}
#endif

/**
 ****************************************************************************************
 * @brief Get the remote service UUID enable status from application device record
 *
 ****************************************************************************************
 */
#if (BLE_CENTRAL)
bool app_get_remote_service_status(uint8_t idx, uint16_t uuid)
{
    bool r = false;
#if QN_SVC_DISC_USED
    for (uint8_t i = 0; i < app_env.dev_rec[idx].nb_svc; i++)
    {
        if (app_env.dev_rec[idx].svc_rec[i].attr_hdl == uuid)
        {
            r = true;
            break;
        }
    }    
#endif
    return r;
}
#endif

/**
 ****************************************************************************************
 * @brief Get the client index by connection handle and uuid.
 *
 ****************************************************************************************
 */
#if (BLE_CENTRAL)
uint8_t app_get_client_idx_by_conhdl(uint16_t conhdl, uint16_t uuid)
{
    uint8_t idx = 0, stop = false;

    for (; idx < BLE_CONNECTION_MAX; idx++)
    {
        switch (uuid)
        {
    

#if BLE_DIS_CLIENT
            case ATT_SVC_DEVICE_INFO:
                if (app_disc_env[idx].conhdl == conhdl)
                    stop = true;
                break;
#endif            
                
            default:
                QPRINTF("Unknown UUID\r\n");
                break;
        }

        if (stop == true)
            return idx;
    }

    return APP_INVALID_INDEX;
}
#endif

/**
 ****************************************************************************************
 * @brief Get the client service UUID enable status from application device record
 *
 ****************************************************************************************
 */
#if (BLE_CENTRAL)
uint8_t app_get_qpp_client_service_status(uint8_t idx)
{
    uint8_t enabled = 0;

    return enabled;
}

uint8_t app_get_client_service_status(uint8_t idx, uint16_t uuid)
{
    uint8_t enabled = 0;
    switch (uuid)
    {

#if BLE_DIS_CLIENT
        case ATT_SVC_DEVICE_INFO:
            enabled = app_disc_env[idx].enabled;
            break;
#endif
        default:
            break;
    }
   
    return enabled;
}
#endif

/**
 ****************************************************************************************
 * @brief Set the client service enable status from application device record
 *
 ****************************************************************************************
 */
#if (BLE_CENTRAL)
void app_set_qpp_client_service_status(uint16_t conhdl)
{
//    uint8_t idx;
//
//    for (idx = 0; idx < BLE_CONNECTION_MAX; idx++)
//  {
//
//    }
}

void app_set_client_service_status(uint16_t conhdl)
{
    uint8_t idx;

    for (idx = 0; idx < BLE_CONNECTION_MAX; idx++)
    {
#if BLE_DIS_CLIENT
        if (conhdl == app_disc_env[idx].conhdl)
        {
            app_disc_env[idx].enabled = false;
        }
#endif
    }
}
#endif

/**
 ****************************************************************************************
 * @brief Create server service database
 *
 ****************************************************************************************
 */
#if (BLE_PERIPHERAL)
void app_create_server_service_DB(void)
{
#if (BLE_DIS_SERVER)
    app_diss_create_db(app_diss_env->features);
#endif

}
#endif

/**
 ****************************************************************************************
 * @brief Enable server service
 *
 ****************************************************************************************
 */
#if (BLE_PERIPHERAL)
void app_enable_server_service(uint8_t enabled, uint16_t conhdl)
{
#if BLE_DIS_SERVER
    if (enabled == true)
    {
        app_diss_env->conhdl = conhdl;
        app_diss_enable_req(conhdl, PERM_RIGHT_ENABLE);
        app_diss_env->enabled = true;
    }
    else if (app_diss_env->enabled == true)
    {
        app_diss_env->enabled = false;
    }
#endif
}
#endif

/**
 ****************************************************************************************
 * @brief Set Advertising data
 *
 * @note
 * The format of Advertising data and Scan Response data contains a sequence of AD structures.
 * Each AD structure shall have a Length field of one octet, which contains the Length value, 
 * and a Data field of Length octets. The first octet of the Data field contains the AD type field.
 * The content of the remaining Length C 1 octet in the Data field depends on the value of 
 * the AD type field and is called the AD data.
 * More details please see Core spec 4.0 page 1735.
 ****************************************************************************************
 */
#if (BLE_PERIPHERAL)
uint8_t app_set_adv_data(uint16_t disc_mode)
{
    uint8_t len;
    /* "\x02\x01\x0?\x0C\x08Quintic BLE" */
    
    // Advertising data, BLE only, general discovery mode and short device name
    app_env.adv_data[0] = 0x02;
    app_env.adv_data[1] = GAP_AD_TYPE_FLAGS;
    if (GAP_GEN_DISCOVERABLE == (disc_mode & GAP_GEN_DISCOVERABLE))
    {
        /// Mode in general discoverable
        app_env.adv_data[2] = 0x06;
    }
    else if (GAP_LIM_DISCOVERABLE == (disc_mode & GAP_LIM_DISCOVERABLE))
    {
        /// Mode in limited discoverable
        app_env.adv_data[2] = 0x05;
    }
    else
    {
        /// Mode in non-discoverable
        app_env.adv_data[2] = 0x04;
    }
#if (defined(CFG_EACI))
    app_env.adv_data[3] = device_name.namelen + 1;
    app_env.adv_data[4] = 0x09;
    memcpy((char *)&app_env.adv_data[5], device_name.name, device_name.namelen);
    len = 5 + device_name.namelen;
#else
    nvds_tag_len_t name_length = 31 - 5; // The maximum length of Advertising data is 31 Octets

    if (NVDS_OK != nvds_get(NVDS_TAG_DEVICE_NAME, &name_length, &app_env.adv_data[5]))
    {
        // NVDS is empty, use default name
        name_length = strlen(QN_LOCAL_NAME);
        strcpy((char *)&app_env.adv_data[5], QN_LOCAL_NAME);
    }
    else
    {
        name_length--; // Discard '\0'
    }
    app_env.adv_data[3] = name_length + 1;
    app_env.adv_data[4] = GAP_AD_TYPE_SHORTENED_NAME;
    len = 5 + name_length;
#endif
    
    return len;
}
#endif

/**
 ****************************************************************************************
 * @brief Set Scan response data
 *
 * @note
 * The format of Scan Response data is the same as the same as Advertising data.
 ****************************************************************************************
 */
#if (BLE_PERIPHERAL)
uint8_t app_set_scan_rsp_data(uint16_t srv_flag)
{
    /* "\x05\x12\x06\x00\x80\x0C" */
    uint8_t len;

    if (srv_flag == 0)
    {
        // Slave connection interval range:0x12
        app_env.scanrsp_data[0] = 0x05;
        app_env.scanrsp_data[1] = 0x12;
        app_env.scanrsp_data[2] = 0x06;
        app_env.scanrsp_data[3] = 0x00;
        app_env.scanrsp_data[4] = 0x80;
        app_env.scanrsp_data[5] = 0x0C;

        len = 6;
    }
    else {
#if ( BLE_AN_SERVER || BLE_CSC_SENSOR || BLE_PAS_SERVER || BLE_HT_THERMOM || BLE_RSC_SENSOR\
      || BLE_BP_SENSOR || BLE_HR_SENSOR || BLE_GL_SENSOR || BLE_TIP_SERVER || BLE_SP_SERVER\
      || BLE_HID_DEVICE || BLE_PROX_REPORTER || BLE_FINDME_TARGET || BLE_DIS_SERVER || BLE_BATT_SERVER\
      || BLE_OTA_SERVER )
    uint8_t remain_len = 25-2;
#endif
    bool complete = true;
    len = 2;

#if BLE_DIS_SERVER
    if (len <= remain_len)
    {
        if (srv_flag & BLE_DIS_SERVER_BIT)
        {
            app_env.scanrsp_data[len+0] = (uint8_t)(ATT_SVC_DEVICE_INFO&0x00FF);
            app_env.scanrsp_data[len+1] = (uint8_t)(ATT_SVC_DEVICE_INFO>>8);
            len += 2;
        }
    }
    else
    {
        complete = false;
    }
#endif
 
// in case no profile server enable
    if (len == 2)
    {
        app_env.scanrsp_data[2] = (uint8_t)(ATT_SVC_GENERIC_ACCESS&0x00FF);
        app_env.scanrsp_data[3] = (uint8_t)(ATT_SVC_GENERIC_ACCESS>>8);
        app_env.scanrsp_data[4] = (uint8_t)(ATT_SVC_GENERIC_ATTRIBUTE&0x00FF);
        app_env.scanrsp_data[5] = (uint8_t)(ATT_SVC_GENERIC_ATTRIBUTE>>8);
        len += 4;
    }
    // UUID list length
    app_env.scanrsp_data[0] = len - 1;

    // Complete UUID list or not
    if (complete == true)
        app_env.scanrsp_data[1] = 0x03;
    else
        app_env.scanrsp_data[1] = 0x02;
    
    // Slave connection interval range:0x12
    app_env.scanrsp_data[len+0] = 0x05;
    app_env.scanrsp_data[len+1] = 0x12;
    app_env.scanrsp_data[len+2] = 0x06;
    app_env.scanrsp_data[len+3] = 0x00;
    app_env.scanrsp_data[len+4] = 0x80;
    app_env.scanrsp_data[len+5] = 0x0C;

    len += 6; }
    
    return len;
}
#endif

/**
 ****************************************************************************************
 * @brief Parser Advertising or Scan response data
 *
 ****************************************************************************************
 */
#if (BLE_CENTRAL || BLE_OBSERVER)
bool app_parser_adv_data(uint8_t *pdata, uint8_t total_len, struct app_adv_data *padv)
{
    uint8_t *pend = pdata + total_len;

    memset(padv, 0, sizeof(struct app_adv_data));
    while (pdata < pend)
    {
        uint8_t data_len = *pdata++; // Data length include advertising type

        if ((pdata + data_len) > pend)
            return false;

        switch (*pdata)
        {
            case GAP_AD_TYPE_SHORTENED_NAME:
            case GAP_AD_TYPE_COMPLETE_NAME:
                memcpy(padv->name, pdata + 1, data_len - 1);
                padv->name[data_len] = '\0';
                padv->flag |= AD_TYPE_NAME_BIT;
                break;
            case GAP_AD_TYPE_MORE_16_BIT_UUID:
            case GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID:
            {
                uint16_t *puuid = &(padv->uuid[0]);
                for (uint8_t i = 0; i < (data_len - 1); i += 2)
                {
                   *puuid++ = (uint16_t)(pdata[i + 1] | (pdata[i + 2] << 8));
                    padv->uuid_num++;
                }
                padv->flag |= AD_TYPE_16bitUUID_BIT;
                break;
            }
            default:
                break;
        }
        pdata += data_len;
    }

    return true;
}
#endif

/**
 ****************************************************************************************
 * @brief Check Updated Connection Parameters is acceptable or not
 *
 ****************************************************************************************
 */
bool app_check_update_conn_param(struct gap_conn_param_update const *conn_param)
{
    
	return false; 
	//return true;
}
