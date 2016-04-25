/**
 ****************************************************************************************
 *
 * @file app_gatt_task.c
 *
 * @brief Application GATT task implementation
 *
 * Copyright (C) Quintic 2012-2014
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

#include "usr_design.h"
#include "lib.h"
/**
 ****************************************************************************************
 * @addtogroup APP_GATT_TASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_env.h"
extern nodDevice_t nodConfigArray[];
static bool TransmitEnableFlag = 0;
extern bool accelModeEnabled;
#if QN_SVC_DISC_USED

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/*
 ****************************************************************************************
 * @brief Handles the Discovery all services complete event. *//**
 *
 * @param[in] msgid     GATT_DISC_SVC_ALL_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_svc_all_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched services from
 *  the remote server. The searched service item include the UUID, start handle and end handle.
 *
 * @note  
 *
 *  GATT service list structure refer to struct gatt_svc_list
 ****************************************************************************************
 */
int app_gatt_disc_svc_all_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_svc_all_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->status == ATT_ERR_NO_ERROR)
    {
        uint8_t idx = KE_IDX_GET(src_id);
        uint8_t *nb_svc = &app_env.dev_rec[idx].nb_svc;
        for (uint8_t i = 0; i < param->nb_resp && *nb_svc < QN_GATT_MAX_HDL_NB; i++)
        {
            // Profile service should between ATT_SVC_IMMEDIATE_ALERT and ATT_SVC_SCAN_PARAMETERS
            if (param->list[i].attr_hdl >= ATT_SVC_IMMEDIATE_ALERT && param->list[i].attr_hdl <= ATT_SVC_SCAN_PARAMETERS)         
            {
                app_env.dev_rec[idx].svc_rec[*nb_svc].attr_hdl = param->list[i].attr_hdl;
                app_env.dev_rec[idx].nb_svc += 1;
            }
            QPRINTF("UUID: 0x%04X ", param->list[i].attr_hdl);
            QPRINTF("START: 0x%04X ", param->list[i].start_hdl);
            QPRINTF("END: 0x%04X\r\n", param->list[i].end_hdl);
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the Discovery specific services by UUID complete event. *//**
 *
 * @param[in] msgid     GATT_DISC_SVC_BY_UUID_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_svc_by_uuid_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched services from
 *  the remote server. The searched service item include start handle and end handle.
 *
 * @note  
 *
 *  GATT service list structure refer to struct gatt_svc_range_list
 ****************************************************************************************
 */
int app_gatt_disc_svc_by_uuid_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_svc_by_uuid_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->nb_resp != 0)
    {
        for (uint8_t i = 0; i < param->nb_resp; i++)
        {
            QPRINTF("Service start handle %04X, end handle %04X\r\n", param->list[i].start_hdl, param->list[i].end_hdl);
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the Discovery included services by UUID complete event. *//**
 *
 * @param[in] msgid     GATT_DISC_SVC_INCL_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_svc_incl_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched services from
 *  the remote server. The searched service item include included service UUID, start handle and end handle.
 *
 ****************************************************************************************
 */
int app_gatt_disc_svc_incl_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_svc_incl_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->nb_entry != 0)
    {
        if (param->entry_len == GATT_UUID_2_OCTET_LEN)
        {
            for (uint8_t i = 0; i < param->nb_entry; i++)
            {
                QPRINTF("Attribute handle 0x%04X, service UUID 0x%04X, start handle 0x%04X, end handle 0x%04X\r\n", param->incl_list.list[i].attr_hdl,
                param->incl_list.list[i].uuid, param->incl_list.list[i].start_hdl, param->incl_list.list[i].end_hdl);
            }
        }
        else // 16 bytes UUID
        {
            // Just include one item if remote device uses 128 bits UUID
            QPRINTF("Attribute handle 0x%04X, service UUID 0x", param->incl_list.incl.attr_hdl);
            for (int8_t i = GATT_UUID_16_OCTET_LEN - 1; i >= 0; i--)
                QPRINTF("%02x", param->incl_list.incl.uuid[i]);
            QPRINTF(", start handle 0x%04X, end handle 0x%04X\r\n", param->incl_list.incl.start_hdl, param->incl_list.incl.end_hdl);
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the Discovery services with 128 bits UUID complete event. *//**
 *
 * @param[in] msgid     GATT_DISC_SVC_ALL_128_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_svc_all_128_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched services from
 *  the remote server. The searched service item include included service UUID, start handle and end handle.
 *
 ****************************************************************************************
 */
int app_gatt_disc_svc_all_128_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_svc_all_128_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->status == ATT_ERR_NO_ERROR)
    {
        QPRINTF("Service UUID 0x");
        for (int8_t i = GATT_UUID_16_OCTET_LEN - 1; i >= 0; i--)
            QPRINTF("%02x", param->list.attr_hdl[i]);
        QPRINTF(", start handle 0x%04X, end handle 0x%04X\r\n", param->list.start_hdl, param->list.end_hdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the discover all characteristics complete event. *//**
 *
 * @param[in] msgid     GATT_DISC_CHAR_ALL_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_char_all_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched characteristics from
 *  the remote server. The searched characteristics item include properties, pointer handle to
 *  UUID and characteristic UUID.
 *
 ****************************************************************************************
 */
int app_gatt_disc_char_all_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_char_all_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->nb_entry != 0)
    {
        for (uint8_t i = 0; i < param->nb_entry; i++)
        {
            QPRINTF("Attribute handle 0x%04X, properties 0x%02X, value handle 0x%04X, char UUID 0x%04X\r\n", param->list[i].attr_hdl,
                param->list[i].prop, param->list[i].pointer_hdl, param->list[i].uuid);
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the discover characteristics by UUID complete event. *//**
 *
 * @param[in] msgid     GATT_DISC_CHAR_BY_UUID_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_char_by_uuid_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched characteristics from
 *  the remote server. The searched characteristics item include the UUID, properties, 
 *  pointer handle to UUID and characteristic UUID.
 *
 ****************************************************************************************
 */
int app_gatt_disc_char_by_uuid_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_char_by_uuid_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->nb_entry != 0)
    {
        for (uint8_t i = 0; i < param->nb_entry; i++)
        {
            QPRINTF("Attribute handle 0x%04X, properties 0x%02X, value handle 0x%04X, char UUID 0x%04X\r\n", param->list[i].attr_hdl,
                param->list[i].prop, param->list[i].pointer_hdl, param->list[i].uuid);
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the complete event for discover all 128 bits UUID characteristics. *//**
 *
 * @param[in] msgid     GATT_DISC_CHAR_ALL_128_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_char_all_128_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched characteristics from
 *  the remote server. The searched characteristics item include properties, pointer handle to
 *  UUID and characteristic UUID.
 *
 ****************************************************************************************
 */
int app_gatt_disc_char_all_128_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_char_all_128_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->status == ATT_ERR_NO_ERROR)
    {
        QPRINTF("Attribute handle 0x%04X, properties 0x%02X, value handle 0x%04X, ", param->list.attr_hdl,
                param->list.prop, param->list.pointer_hdl);
        QPRINTF("char UUID 0x");
        for (int8_t i = GATT_UUID_16_OCTET_LEN - 1; i >= 0; i--)
            QPRINTF("%02x", param->list.uuid[i]);
        QPRINTF("\r\n");
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the complete event for discover specific 128 bits UUID characteristics. *//**
 *
 * @param[in] msgid     GATT_DISC_CHAR_BY_UUID_128_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_char_by_uuid_128_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched characteristics from
 *  the remote server. The searched characteristics item include properties, pointer handle to
 *  UUID and characteristic UUID.
 *
 ****************************************************************************************
 */
int app_gatt_disc_char_by_uuid_128_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_char_by_uuid_128_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->status == ATT_ERR_NO_ERROR)
    {
        QPRINTF("Attribute handle 0x%04X, properties 0x%02X, value handle 0x%04X, ", param->list.attr_hdl,
                param->list.prop, param->list.pointer_hdl);
        QPRINTF("char UUID 0x");
        for (int8_t i = GATT_UUID_16_OCTET_LEN - 1; i >= 0; i--)
            QPRINTF("%02x", param->list.uuid[i]);
        QPRINTF("\r\n");
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the discover characteristics descriptors complete event. *//**
 *
 * @param[in] msgid     GATT_DISC_CHAR_DESC_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_char_desc_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched characteristics 
 *  descriptors from the remote server. The searched characteristics descriptors item include
 *  the database element handle and descriptor UUID.
 *
 ****************************************************************************************
 */
int app_gatt_disc_char_desc_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_char_desc_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->nb_entry != 0)
    {
        for (uint8_t i = 0; i < param->nb_entry; i++)
        {
            QPRINTF("Attribute handle 0x%04X descriptor UUID 0x%04X.\r\n", param->list[i].attr_hdl,
                 param->list[i].desc_hdl);
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the complete event for discover all 128 bits UUID characteristics. *//**
 *
 * @param[in] msgid     GATT_DISC_CHAR_ALL_128_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_char_desc_128_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the searched characteristics from
 *  the remote server. The searched characteristics item include properties, pointer handle to
 *  UUID and characteristic UUID.
 *
 ****************************************************************************************
 */
int app_gatt_disc_char_desc_128_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_char_desc_128_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    QPRINTF("Attribute handle 0x%04X, ", param->list_128.attr_hdl);
    QPRINTF("descriptor UUID 0x");
    for (int8_t i = GATT_UUID_16_OCTET_LEN - 1; i >= 0; i--)
        QPRINTF("%02x", param->list_128.uuid[i]);
    QPRINTF("\r\n");

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the read characteristic complete event. *//**
 *
 * @param[in] msgid     GATT_READ_CHAR_RESP
 * @param[in] param     Pointer to struct gatt_read_char_resp
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the value of the attribute
 *  handle element from the remote server. The element item include the data length and data.
 *
 ****************************************************************************************
 */
int app_gatt_read_char_resp_handler(ke_msg_id_t const msgid, struct gatt_read_char_resp const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->status == ATT_ERR_NO_ERROR)
    {
        QPRINTF("Gatt read characteristic sucess, ");
        for (int8_t i = 0; i < param->data.len; i++)
            QPRINTF("%02x", param->data.data[i]);
        QPRINTF("\r\n");
    }
    else
    {
        QPRINTF("Gatt read characteristic failed, status is %d\r\n", param->status);
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the Read multiple characteristic complete event. *//**
 *
 * @param[in] msgid     GATT_READ_CHAR_MULTI_RESP
 * @param[in] param     Pointer to struct gatt_read_char_mult_resp
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to receive the multiple value of the 
 *  attribute handle element from the remote server. The element item include
 *  the data length and data.
 *
 ****************************************************************************************
 */
int app_gatt_read_char_mult_resp_handler(ke_msg_id_t const msgid, struct gatt_read_char_mult_resp const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    //App should save the set of handles to be read and expected value size when sending read request
    if (param->status == ATT_ERR_NO_ERROR)
    {
			
			 // HEDDOKO val_len
        QPRINTF("Gatt read multiple characteristic sucess\r\n");
    }
    else
    {
        QPRINTF("Gatt read characteristic failed, status is %d\r\n", param->status);
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the write characteristic complete event. *//**
 *
 * @param[in] msgid     GATT_WRITE_CHAR_RESP
 * @param[in] param     Pointer to struct gatt_write_char_resp
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to handle the result of write characteristics 
 *  value to the remote server.
 *
 ****************************************************************************************
 */
int app_gatt_write_char_resp_handler(ke_msg_id_t const msgid, struct gatt_write_char_resp const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->status == ATT_ERR_NO_ERROR)
    {
      #ifdef DEBUG_MODE  
				QPRINTF("Gatt write sucess\r\n");
			#endif	
    }
    else
    {
			#ifdef DEBUG_MODE
        QPRINTF("Gatt write failed, status is %d\r\n", param->status);
			#endif
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the write reliable characteristic complete event. *//**
 *
 * @param[in] msgid     GATT_WRITE_CHAR_RELIABLE_RESP
 * @param[in] param     Pointer to struct gatt_write_reliable_resp
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to handle the result of write long 
 *  characteristics value to the remote server.
 *
 ****************************************************************************************
 */
int app_gatt_write_char_reliable_resp_handler(ke_msg_id_t const msgid, struct gatt_write_reliable_resp const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (param->status == ATT_ERR_NO_ERROR)
    {
        QPRINTF("Gatt reliable write sucess\r\n");
    }
    else
    {
        QPRINTF("Gatt reliable write failed, status is %d\r\n", param->status);
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the cancel write characteristic complete event. *//**
 *
 * @param[in] msgid     GATT_CANCEL_WRITE_CHAR_RESP
 * @param[in] param     None
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to handle the result of cancel write
 *  characteristics request.
 *
 ****************************************************************************************
 */
int app_gatt_cancel_write_char_resp_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    QPRINTF("Gatt cancel write complete\r\n");

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the notify complete event. *//**
 *
 * @param[in] msgid     GATT_NOTIFY_CMP_EVT
 * @param[in] param     Pointer to struct gatt_notify_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to indicate the failed reason of notify
 *  request.
 *
 ****************************************************************************************
 */
int app_gatt_notify_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_notify_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    QPRINTF("Gatt send notify handle 0x%04x, status %d\r\n", param->handle, param->status);

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the value notification. *//**
 *
 * @param[in] msgid     GATT_HANDLE_VALUE_NOTIF
 * @param[in] param     Pointer to struct gatt_handle_value_notif
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to handle the value notification
 *
 ****************************************************************************************
 */
int app_gatt_handle_value_notif_handler(ke_msg_id_t const msgid, struct gatt_handle_value_notif const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    //QPRINTF("Gatt received notify from remote, handle 0x%04x, ", param->charhdl); // Heddoko
   // for (uint8_t i = 0; i < param->size; i++)
    //    QPRINTF("%02x", param->value[i]);
   // QPRINTF("\r\n");
		//uint8_t addr_cpy=0;
		uint8_t x=0;
		struct bd_addr  barrdr;
		extern struct QN qn;
		//char input_d[2] = {0x00,0x00};
		app_get_bd_addr_by_conhdl(param->conhdl, &barrdr);
	
//		QPRINTF(" %02X%02X%02X%02X%02X%02X\r\n", 
//                    barrdr.addr[5],
//                    barrdr.addr[4],
//                    barrdr.addr[3],
//                    barrdr.addr[2],
//                    barrdr.addr[1],
//                    barrdr.addr[0]);
//	
//		//timer0_callback();	//funtion to print cpu time for reference	
//	
//		for (uint8_t i = 6; i < param->size; i++)
//       QPRINTF("%02x", param->value[i]);
//		QPRINTF("\r\n");
		
/*	Heddoko: Buffers to store the data of each NOD. Each NOD has an individual buffer	*/

		if(TransmitEnableFlag == 0)
		{
			TransmitEnableFlag = qn.id[0].number;
			for(int z=1; z<=QnConNum; z++)
				TransmitEnableFlag &= (qn.id[z].number);	//Transmit data only when first frame from all IMU is received
		}
		
		for(int z=0;z<=QnConNum;z++)
		{			
			if((barrdr.addr[1]==nod[z][1])&&(barrdr.addr[0]==nod[z][0]))
			{
				qn.id[z].number = 1;
				if((TransmitEnableFlag == 1) && (StartReqFlag == 1))		//(qn.id[z].buf_head == 10) & 
				{
//					QPRINTF("%d%d", z,z);
//					for(int i=0; i<10; i++)
//					{
//						for (int y = 0; y < 6; y++)
//							QPRINTF("%02X",qn.id[z].data[i][y]);
//					}
//					
//					QPRINTF("\r\n");
//					qn.id[z].buf_head = 0;
				//}
				//param->charhdl	
					
				uint16_t val[3];
				uint8_t dataPacket[6]; 
				int i =0;	
				int y = 0;	
				//Debug Print the received data

				if(accelModeEnabled == 0)
				{					
					if(param->charhdl == 0x0042)
					{					
						for(y=6;y<param->size;y+=2,i++)
						{
								 //val[i] = *((uint16_t*)&(param->value[y]));
							val[i] = (uint16_t)(param->value[y+1]) + (uint16_t)(param->value[y]<<8);	 
							//dataPacket[i]	= param->value[y]; 
							//(uint16_t*)&(param->value[y])
						}
						#ifdef DEBUG_MODE
							//timer0_callback();
							 QPRINTF("hdl%04x,%d,",param->charhdl, ke_time());
						#endif							
						QPRINTF("&%d%04X%04X%04X\r\n",z,val[0],val[1],val[2]);
					}
				}
				else
				{
					if(param->charhdl == 0x004e)
					{
						for(y=0;y<6;y+=2,i++)
						{
								 //val[i] = *((uint16_t*)&(param->value[y]));
							val[i] = (uint16_t)(param->value[y+1]) + (uint16_t)(param->value[y]<<8);	 
							//dataPacket[i]	= param->value[y]; 
							//(uint16_t*)&(param->value[y])
						}
						#ifdef DEBUG_MODE
							//timer0_callback();
							 QPRINTF("hdl%04x,%d,",param->charhdl, ke_time());
						#endif							
						QPRINTF("@%d%04X%04X%04X\r\n",z,val[0],val[1],val[2]);
					}
				}


				
					//for(int y=0;y<3;y++)
					//		QPRINTF("%02X",qn.id[z].data[qn.id[z].buf_head][y]);
					

				
				//QPRINTF("\r\n");
//				qn.id[z].buf_head++;
//				if(qn.id[z].buf_head>=BUF_MAX_SIZE)
//					qn.id[z].buf_head=0;
				}
			}
		}

/*	Heddoko: Print the values from all three buffers	*/
//		if((qn.id[0].nb>=100)&&(qn.id[1].nb>=100)&&(qn.id[2].nb>=100)&&(flg==0)){
			
//			QPRINTF("Writing to stop notif\r\n");
//			for (uint16_t i=0; i<app_env.cn_count; i++){
//				app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x0043,2,(uint8_t *)input_d);
//			}
//			for(int z=0;z<QN_MAX_CONN;z++){
//				
//				QPRINTF("NOD id: %02x", z);
//				QPRINTF(" %02X%02X%02X%02X%02X%02X\r\n", 
//											nod[z][5],
//											nod[z][4],
//											nod[z][3],
//											nod[z][2],
//											nod[z][1],
//											nod[z][0]);
//				
//				for(int x=0;x<=qn.id[z].nb;x++){
//					for(int y=6;y<param->size;y++)
//						QPRINTF("%02X",qn.id[z].data[x][y]);
//					QPRINTF("\r\n");
//				}
//			}
//			QPRINTF("End\r\n");
//			flg=1;
//		}
			
			
		/*
		*********************************** Avegraging function ******************************** //Heddoko
		
		//NO LONGER USED AS EVERY BIT OF DATA IS REQUIRED
		
		****************************************************************************************
		*/
		/*
		struct bd_addr dev_detect;
		//uint8_t data_dev[BLE_CONNECTION_MAX], value_dev[ATTM_MAX_VALUE];
		uint16_t dev_data[BLE_CONNECTION_MAX][ATTM_MAX_VALUE];
		
		for (uint8_t idx = 0; idx < app_env.cn_count; idx++) 			//searches the device sending data in device database //usually idx<BLE_CONNECTION_MAX
		{
			if (app_get_bd_addr_by_idx(idx, &dev_detect))
				if(co_bt_bdaddr_compare(&barrdr, &dev_detect)==true)	//checks device address with the one in the database
				{
					QPRINTF("Device matched with index: %d \r\n",idx);
					for (uint8_t i = 0; i < param->size; i++)						//averages the value receieved from the device.
					{
						//QPRINTF("%02x", param->value[i]);
						if(dev_data[idx][i]==0)														//checks if its first time. dont average if yes.
						{
							dev_data[idx][i]=dev_data[idx][i]+param->value[i];
						}
						else
						{
							dev_data[idx][i]=dev_data[idx][i]+param->value[i];
							dev_data[idx][i]/=2;
						}
							
					}
					
					QPRINTF("Data average for device id: %d is \r\n",idx);	//display the average value of data
					for (uint8_t i = 0; i < param->size; i++)
					{
						QPRINTF("%02x", dev_data[idx][i]);
					}
					QPRINTF("\r\n");
				}
		}
	***************************** Averaging function ends ********************************/
    return (KE_MSG_CONSUMED);
	
}

/*
 ****************************************************************************************
 * @brief Handles reception of of a peer device indication. *//**
 *
 * @param[in] msgid     GATT_HANDLE_VALUE_IND
 * @param[in] param     Pointer to struct gatt_handle_value_ind
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to handle reception of a peer device
 *  indication.
 *
 ****************************************************************************************
 */
int app_gatt_handle_value_ind_handler(ke_msg_id_t const msgid, struct gatt_handle_value_ind const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    QPRINTF("Gatt received inication from remote, handle 0x%04x, ", param->charhdl);
    for (uint8_t i = 0; i < param->size; i++)
        QPRINTF("%02x", param->value[i]);
    QPRINTF("\r\n");

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles reception of a peer device confirmation of a previously sent indication. *//**
 *
 * @param[in] msgid     GATT_HANDLE_VALUE_CFM
 * @param[in] param     Pointer to struct gatt_handle_value_cfm
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the Client role of GATT to Handles reception of a peer device
 *  confirmation of a previously sent indication.
 *
 ****************************************************************************************
 */
int app_gatt_handle_value_cfm_handler(ke_msg_id_t const msgid, struct gatt_handle_value_cfm const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    QPRINTF("Gatt the handle 0x%04x value indication is sent, status %d\r\n", param->handle, param->status);

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the Discovery complete event. *//**
 *
 * @param[in] msgid     GATT_DISC_CMP_EVT
 * @param[in] param     Pointer to struct gatt_disc_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used by the GATT Discovery all services complete event.
 *
 ****************************************************************************************
 */
int app_gatt_disc_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_disc_cmp_evt const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    QPRINTF("Discovery Services finished.\r\n");

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the GATT operation complete event. *//**
 *
 * @param[in] msgid     GATT_CMP_EVT
 * @param[in] param     Pointer to struct gatt_cmp_evt
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_GATT
 * @return If the message was consumed or not.
 * @description
 *
 *  This handler is used to tell application the complete of GATT operations.
 *
 ****************************************************************************************
 */
int app_gatt_cmp_evt_handler(ke_msg_id_t const msgid, struct gatt_cmp_evt const *param,
                          ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
		#ifdef DEBUG_MODE
    QPRINTF("Gatt command ");
    if (param->status == ATT_ERR_NO_ERROR)
        QPRINTF("success.\r\n");
    else
        QPRINTF("failed, status is %d.\r\n", param->status);
		#endif
    return (KE_MSG_CONSUMED);
}

#endif

/// @} APP_GATT_TASK
