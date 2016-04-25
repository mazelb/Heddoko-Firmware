/**
 ****************************************************************************************
 *
 * @file app_menu.h
 *
 * @brief Application DEMO menu
 *
 * Copyright (C) Quintic 2012-2013
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

#ifndef _APP_MENU_H_
#define _APP_MENU_H_

#include "ke_msg.h"
//uint8_t nod[9][6]={0};	//Heddoko nod addresses

/*
 * DEFINES
 ****************************************************************************************
 */
// menu id
typedef enum
{
    menu_start,
    menu_main,
    menu_gap,
    menu_gap_create_connection,
    menu_gap_disconnection,
    menu_gap_bond,
    menu_gap_unpair,
    menu_gatt,
    menu_gatt_disc_all_svc,
    menu_gatt_disp_all_svc,
	  menu_gatt_ext, // HEDDOKO
	  menu_gatt_ext_read, // HEDDOKO
		menu_gatt_ext_write_cmd, // HEDDOKO
		menu_gatt_ext_write_req, // HEDDOKO
	  menu_gatt_ext_evt_handle, //HEDDOKO
		menu_gatt_send_data,
    menu_smp,
    menu_smp_sec_lvl_set,
    menu_smp_io_cap_set,
    menu_smp_tk_input,
		menu_qppc,
		menu_qppc_enable,
		menu_disc_enable,
		menu_disc,
    menu_max
} MENU_ID;
 
/*
 ****************************************************************************************
 * @brief Show the BLE demo application.
 *
 ****************************************************************************************
 */
void app_menu_show(void);

/*
 ****************************************************************************************
 * @brief Handler function of the BLE demo application.
 *
 ****************************************************************************************
 */
void app_menu_hdl(void);

/*
 ****************************************************************************************
 * @brief Callback function of MENU.
 *
 ****************************************************************************************
 */
void app_menu_cb(ke_msg_id_t const msgid, void *param);

#endif // _APP_MENU_H_

