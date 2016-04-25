/**
 ****************************************************************************************
 *
 * @file app_task.c
 *
 * @brief APP Task implementation
 *
 * Copyright (C) Quintic 2012-2014
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_env.h"

#if QN_DEMO_MENU
#include "app_menu.h"
#endif
#if (QN_WORK_MODE == WORK_MODE_NP)
#include "led.h"
#endif
#if QN_EACI
#include "app_eaci.h"
#endif

#if (QN_32K_RCO)
#include "syscon.h"
#endif

#if (QN_DEEP_SLEEP_EN || QN_32K_RCO)
#include "lib.h"
#include "sleep.h"
#endif

#if (QN_EACI&&(EACI_MSG_BUFFER_EN==TRUE))
#include "app_eaci_trans.h"
#endif

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if (QN_DEMO_MENU || QN_EACI)
/**
 ****************************************************************************************
 * @brief Handles UART data indication.
 *
 * @param[in] msgid      APP_SYS_UART_DATA_IND
 * @param[in] param      Pointer to struct app_uart_data_ind
 * @param[in] dest_id    TASK_APP
 * @param[in] src_id     TASK_APP
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_uart_data_ind_handler(ke_msg_id_t const msgid, struct app_uart_data_ind const *ind,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch (ke_state_get(dest_id))
    {
    case APP_INIT:
        break;
    case APP_IDLE:
    default:
#if QN_EACI
        app_eaci_msg_hdl((src_id >> 8) & 0xFF,
                                src_id & 0xFF,
                                ind->len,
                                ind->data);
#elif QN_DEMO_MENU
        if (ind->len != 0)
        {
            memcpy(app_env.input, ind->data, ind->len);
            app_menu_hdl();
        }
#endif
        break;
    }

#if (QN_EACI&&(EACI_MSG_BUFFER_EN==TRUE))
    if (eaci_msg_buf_used == EACI_BUF_DEEP) {
        eaci_trans_init();
    }
    if (eaci_msg_buf_used) {
        eaci_msg_buf_used--;
    }
    return (KE_MSG_NO_FREE);
#else
    return (KE_MSG_CONSUMED);
#endif
}
#endif

/**
 ****************************************************************************************
 * @brief Handles 32k rco periodic calibration.
 *
 * @param[in] msgid      APP_SYS_RCO_CAL_TIMER
 * @param[in] param      None
 * @param[in] dest_id    TASK_APP
 * @param[in] src_id     TASK_APP
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
#if (QN_32K_RCO)
static int app_rco_cal_timer_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    clock_32k_correction_enable(clock_32k_correction_cb);

    dev_prevent_sleep(PM_MASK_TIMER1_ACTIVE_BIT);

    ke_timer_set(APP_SYS_RCO_CAL_TIMER, TASK_APP, 100);

    return (KE_MSG_CONSUMED);
}
#endif

/**
 ****************************************************************************************
 * @brief Handles 32k XTAL wakeup precedure.
 *
 * @param[in] msgid      APP_SYS_32K_XTAL_WAKEUP_TIMER
 * @param[in] param      None
 * @param[in] dest_id    TASK_APP
 * @param[in] src_id     TASK_APP
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
#if (QN_DEEP_SLEEP_EN && !QN_32K_RCO)
static int app_32k_xtal_wakeup_timer(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // disable schmitt trigger in 32.768KHz buffer
    syscon_SetIvrefX32WithMask(QN_SYSCON, SYSCON_MASK_X32SMT_EN, MASK_DISABLE);
    syscon_SetIvrefX32WithMask(QN_SYSCON, SYSCON_MASK_X32ICTRL, 16);
    
    // switch 32k RCO to 32k XTAL
    syscon_SetCMDCRWithMask(QN_SYSCON, SYSCON_MASK_SEL_CLK_32K, MASK_DISABLE);
    syscon_SetPGCR1WithMask(QN_SYSCON, SYSCON_MASK_DIS_RCO, MASK_ENABLE);

    // allow ble sleep
    enable_ble_sleep(true);

    return (KE_MSG_CONSUMED);
}
#endif

/* default State handlers definition. */
const struct ke_msg_handler app_default_state[] =
{
#ifdef QN_BLE_TIME_SUPPORT
    {APP_SYS_TIME_CHECK_TIMER,              (ke_msg_func_t) app_sys_time_check_timer_handler},
#endif

#if (QN_EACI || QN_DEMO_MENU)
    {APP_SYS_UART_DATA_IND,                 (ke_msg_func_t) app_uart_data_ind_handler},
#endif

#if (QN_32K_RCO)
    {APP_SYS_RCO_CAL_TIMER,                 (ke_msg_func_t) app_rco_cal_timer_handler},
#endif

#if (QN_DEEP_SLEEP_EN && !QN_32K_RCO)
    {APP_SYS_32K_XTAL_WAKEUP_TIMER,         (ke_msg_func_t) app_32k_xtal_wakeup_timer},
#endif

#if (!QN_WORK_MODE && !QN_EAPI)
#if (BLE_PERIPHERAL || BLE_BROADCASTER || BLE_OBSERVER)
//{APP_SYS_LED_1_TIMER,                   (ke_msg_func_t) app_led_timer_handler},
#if (!QN_EACI)
  //  {APP_SYS_LED_2_TIMER,                   (ke_msg_func_t) app_led_timer_handler},
#if (BLE_PERIPHERAL)
//{APP_ADV_INTV_UPDATE_TIMER,             (ke_msg_func_t) app_gap_adv_intv_update_timer_handler},
#endif
  //  {APP_SYS_BUTTON_1_TIMER,                (ke_msg_func_t) app_button_timer_handler},
 //   {APP_SYS_BUTTON_2_TIMER,                (ke_msg_func_t) app_button_timer_handler},
#endif
#endif
#endif

    {GAP_RESET_REQ_CMP_EVT,                 (ke_msg_func_t) app_gap_reset_req_cmp_handler},
    {GAP_SET_SEC_REQ_CMP_EVT,               (ke_msg_func_t) app_gap_set_sec_req_cmp_evt_handler},
    {GAP_READ_VER_REQ_CMP_EVT,              (ke_msg_func_t) app_gap_read_ver_req_cmp_evt_handler},
    {GAP_READ_BDADDR_REQ_CMP_EVT,           (ke_msg_func_t) app_gap_read_bdaddr_req_cmp_evt_handler},
    {GAP_READY_EVT,                         (ke_msg_func_t) app_gap_ready_evt_handler},
    {GAP_SET_DEVNAME_REQ_CMP_EVT,           (ke_msg_func_t) app_gap_set_devname_req_cmp_evt_handler},
#if (BLE_PERIPHERAL)
    {GAP_ADV_REQ_CMP_EVT,                   (ke_msg_func_t) app_gap_adv_req_cmp_evt_handler},
    {GAP_PARAM_UPDATE_RESP,                 (ke_msg_func_t) app_gap_param_update_resp_handler},
    {GAP_SET_PRIVACY_REQ_CMP_EVT,           (ke_msg_func_t) app_gap_set_privacy_req_cmp_handler},
#endif
#if (BLE_CENTRAL || BLE_OBSERVER)
    {GAP_DEV_INQ_REQ_CMP_EVT,               (ke_msg_func_t) app_gap_dev_inq_cmp_handler},
#endif
#if (BLE_OBSERVER)
    {GAP_ADV_REPORT_EVT,                    (ke_msg_func_t) app_gap_dev_scan_result_handler},
#endif
#if (BLE_CENTRAL)
    {GAP_DEV_INQ_RESULT_EVT,                (ke_msg_func_t) app_gap_dev_inq_result_handler},
    {GAP_SCAN_REQ_CMP_EVT,                  (ke_msg_func_t) app_gap_scan_req_cmp_evt_handler},
    {GAP_CANCEL_CONN_REQ_CMP_EVT,           (ke_msg_func_t) app_gap_cancel_conn_req_cmp_evt_handler},
    {GAP_NAME_REQ_CMP_EVT,                  (ke_msg_func_t) app_gap_name_req_cmp_handler},
    {GAP_PARAM_UPDATE_REQ_IND,              (ke_msg_func_t) app_gap_param_update_req_ind_handler},
    {GAP_SET_RECON_ADDR_REQ_CMP_EVT,        (ke_msg_func_t) app_gap_set_recon_addr_req_cmp_handler},
    {GAP_SET_PH_PRIVACY_REQ_CMP_EVT,        (ke_msg_func_t) app_gap_set_ph_privacy_req_cmp_handler},
#endif
    {GAP_CHANGE_PARAM_REQ_CMP_EVT,          (ke_msg_func_t) app_gap_change_param_req_cmp_handler},
    {GAP_SET_MODE_REQ_CMP_EVT,              (ke_msg_func_t) app_gap_set_mode_req_cmp_evt_handler},
#if (!BLE_BROADCASTER && !BLE_OBSERVER)
    {GAP_LE_CREATE_CONN_REQ_CMP_EVT,        (ke_msg_func_t) app_gap_le_create_conn_req_cmp_evt_handler},
    {GAP_DISCON_CMP_EVT,                    (ke_msg_func_t) app_gap_discon_cmp_evt_handler},
#endif
#if (!BLE_BROADCASTER && !BLE_OBSERVER)
    {GAP_BOND_REQ_IND,                      (ke_msg_func_t) app_gap_bond_req_ind_handler},
#endif
#if (QN_SECURITY_ON)
    {GAP_BOND_REQ_CMP_EVT,                  (ke_msg_func_t) app_gap_bond_req_cmp_ind_handler},
#endif
#if (QN_WL_SUPPORT)
    {GAP_LE_RD_WLST_SIZE_CMD_CMP_EVT,       (ke_msg_func_t) app_gap_le_rd_wlst_size_cmd_cmp_handler},
    {GAP_LE_ADD_DEV_TO_WLST_REQ_CMP_EVT,    (ke_msg_func_t) app_gap_le_add_dev_to_wlst_req_cmp_handler},
    {GAP_LE_RMV_DEV_FRM_WLST_REQ_CMP_EVT,   (ke_msg_func_t) app_gap_le_rmv_dev_from_wlst_req_cmp_handler},
#endif
#if (QN_LINK_INFO)
    {GAP_LE_RD_REMOTE_FEAT_REQ_CMP_EVT,     (ke_msg_func_t) app_gap_le_rd_remote_feat_req_cmp_handler},
    {GAP_RD_REM_VER_INFO_CMP_EVT,           (ke_msg_func_t) app_gap_rd_remote_ver_info_cmp_handler},
    {GAP_CHANNEL_MAP_CMP_EVT,               (ke_msg_func_t) app_gap_channel_map_cmp_handler},
    {GAP_READ_RSSI_REQ_CMP_EVT,             (ke_msg_func_t) app_gap_read_rssi_req_cmp_handler},
#endif
    {GAP_SET_RANDOM_ADDR_REQ_CMP_EVT,       (ke_msg_func_t) app_gap_set_random_add_req_cmp_handler},

    {GATT_RESOURCE_ACCESS_REQ,              (ke_msg_func_t) app_gatt_resource_access_req_handler},
#if QN_SVC_DISC_USED
    {GATT_DISC_SVC_ALL_CMP_EVT,             (ke_msg_func_t) app_gatt_disc_svc_all_cmp_evt_handler},
    {GATT_DISC_SVC_BY_UUID_CMP_EVT,         (ke_msg_func_t) app_gatt_disc_svc_by_uuid_cmp_evt_handler},
    {GATT_DISC_SVC_INCL_CMP_EVT,            (ke_msg_func_t) app_gatt_disc_svc_incl_cmp_evt_handler},
    {GATT_DISC_SVC_ALL_128_CMP_EVT,         (ke_msg_func_t) app_gatt_disc_svc_all_128_cmp_evt_handler},
    {GATT_DISC_CHAR_ALL_CMP_EVT,            (ke_msg_func_t) app_gatt_disc_char_all_cmp_evt_handler},
    {GATT_DISC_CHAR_BY_UUID_CMP_EVT,        (ke_msg_func_t) app_gatt_disc_char_by_uuid_cmp_evt_handler},
    {GATT_DISC_CHAR_ALL_128_CMP_EVT,        (ke_msg_func_t) app_gatt_disc_char_all_128_cmp_evt_handler},
    {GATT_DISC_CHAR_BY_UUID_128_CMP_EVT,    (ke_msg_func_t) app_gatt_disc_char_by_uuid_128_cmp_evt_handler},
    {GATT_DISC_CHAR_DESC_CMP_EVT,           (ke_msg_func_t) app_gatt_disc_char_desc_cmp_evt_handler},
    {GATT_DISC_CHAR_DESC_128_CMP_EVT,       (ke_msg_func_t) app_gatt_disc_char_desc_128_cmp_evt_handler},
    {GATT_READ_CHAR_RESP,                   (ke_msg_func_t) app_gatt_read_char_resp_handler},
    {GATT_READ_CHAR_MULT_RESP,              (ke_msg_func_t) app_gatt_read_char_mult_resp_handler},
    {GATT_WRITE_CHAR_RESP,                  (ke_msg_func_t) app_gatt_write_char_resp_handler},
    {GATT_WRITE_CHAR_RELIABLE_RESP,         (ke_msg_func_t) app_gatt_write_char_reliable_resp_handler},
    {GATT_CANCEL_WRITE_CHAR_RESP,           (ke_msg_func_t) app_gatt_cancel_write_char_resp_handler},
    {GATT_NOTIFY_CMP_EVT,                   (ke_msg_func_t) app_gatt_notify_cmp_evt_handler},
    {GATT_HANDLE_VALUE_NOTIF,               (ke_msg_func_t) app_gatt_handle_value_notif_handler},
    {GATT_HANDLE_VALUE_IND,                 (ke_msg_func_t) app_gatt_handle_value_ind_handler},
    {GATT_HANDLE_VALUE_CFM,                 (ke_msg_func_t) app_gatt_handle_value_cfm_handler},
    {GATT_DISC_CMP_EVT,                     (ke_msg_func_t) app_gatt_disc_cmp_evt_handler},
    {GATT_CMP_EVT,                          (ke_msg_func_t) app_gatt_cmp_evt_handler},
#endif

#if (!BLE_BROADCASTER && !BLE_OBSERVER)
    {SMPM_SET_KEY_CFM,                      (ke_msg_func_t) app_smpm_set_key_cfm_handler},
#endif
#if (QN_SECURITY_ON)
    {SMPC_KEY_IND,                          (ke_msg_func_t) app_smpc_key_ind_handler},
    {SMPC_TK_REQ_IND,                       (ke_msg_func_t) app_smpc_tk_req_ind_handler},    
    {SMPC_LTK_REQ_IND,                      (ke_msg_func_t) app_smpc_ltk_req_ind_handler},
    {SMPC_IRK_REQ_IND,                      (ke_msg_func_t) app_smpc_irk_req_ind_handler},
    {SMPC_CHK_BD_ADDR_REQ_IND,              (ke_msg_func_t) app_smpc_chk_bd_addr_req_ind_handler},
    {SMPC_SEC_STARTED_IND,                  (ke_msg_func_t) app_smpc_sec_started_ind_handler},
    {SMPC_TIMEOUT_EVT,                      (ke_msg_func_t) app_smpc_timeout_evt_handler},
#endif
#if (QN_CSRK_SUPPORT)
    {SMPC_CSRK_REQ_IND,                     (ke_msg_func_t) app_smpc_csrk_req_ind_handler},
#endif

#if BLE_DIS_CLIENT
    {DISC_ENABLE_CFM,                   	(ke_msg_func_t) app_disc_enable_cfm_handler},
    {DISC_RD_CHAR_RSP,                  	(ke_msg_func_t) app_disc_rd_char_rsp_handler},
    {DISC_DISABLE_IND,                      (ke_msg_func_t) app_disc_disable_ind_handler},
#endif

#if BLE_DIS_SERVER
    {DISS_CREATE_DB_CFM,                    (ke_msg_func_t) app_diss_create_db_cfm_handler}, 
    {DISS_ERROR_IND,                        (ke_msg_func_t) app_diss_error_ind_handler},
#endif

};

/* Specifies the app message handler structure for every input state.*/
const struct ke_state_handler app_state_handler[APP_STATE_MAX] =
{
    [APP_INIT] = KE_STATE_HANDLER_NONE,
    [APP_IDLE] = KE_STATE_HANDLER_NONE,
    [APP_ADV ] = KE_STATE_HANDLER_NONE,
    [APP_SCAN] = KE_STATE_HANDLER_NONE,
};

/* Specifies the app message handlers that are common to all states. */
const struct ke_state_handler app_default_handler = KE_STATE_HANDLER(app_default_state);

/* Defines the place holder for the states of all the app task instances. */
ke_state_t app_state[APP_IDX_MAX];

#if !QN_WORK_MODE
// Register app task into kernel
void task_app_desc_register(void)
{
    struct ke_task_desc task_app_desc;
    
    task_app_desc.state_handler = app_state_handler;
    task_app_desc.default_handler = &app_default_handler;
    task_app_desc.state = app_state;
    task_app_desc.state_max = APP_STATE_MAX;
    task_app_desc.idx_max = APP_IDX_MAX;
    
    task_desc_register(TASK_APP, task_app_desc);
}
#endif

