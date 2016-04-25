/**
 ****************************************************************************************
 *
 * @file app_menu.c
 *
 * @brief Application DEMO menu
 *
 * Copyright (C) Quintic 2012-2014
 *
 * @note make sure to add items to the enum type in app_menu.h
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_env.h"
#include "app_gap.h"
#include <math.h>

#if QN_DEMO_MENU
#include "app_menu.h"
#include "led.h"

//#define QN_MAX_CONN 3		// Maximum number of devices to connect to
#define MAX_NUM_CONNECTIONS 8
uint8_t ack[]="ack";
uint8_t begin[]="begin";	//Heddoko
uint8_t end[]="end";
uint8_t connect[]="connect";
uint8_t scan[]="scan";
uint8_t start[]="start";
uint8_t stop[]="stop";
uint8_t check[]="check";
uint8_t rssi[]="rssi";
uint8_t send1[]="send1";
uint8_t send2[]="send2";
uint8_t send3[]="send3";
uint8_t n[]="A0E";

uint8_t nod[9][6]={0};
uint8_t buf[9][12]={0};
uint8_t chmapBuf[10] = {0};

uint8_t vSetMapCount = 0;

struct le_chnl_map chmapArray[8] = {0}; 
struct le_chnl_map chmapArray_default[8] = {0}; 

char con_st, con_st_nb;
uint8_t menu_lvl =0, z=0;
char QnConNum;
bool j=0;
bool StartReqFlag;
bool accelModeEnabled = 0;

//extern variables
extern char ConnResp;
extern char expectedNodMask;

//structures
struct QN qn;


volatile nodDevice_t nodConfigArray[MAX_NUM_CONNECTIONS];

static void clearNodConfigArray()
{
	int j = 0;
	for(j=0;j<MAX_NUM_CONNECTIONS;j++)
	{
		nodConfigArray[j].isFound = false; 
		nodConfigArray[j].isConnected = false; 
		nodConfigArray[j].mapAssigned = false; 
		nodConfigArray[j].accelNotifyEn = false; 
		nodConfigArray[j].eulerNotifyEn = false;
		memset((void*)(nodConfigArray[j].bdaAddress), 0, 6);
	}	
}
static void app_menu_show_line(void)
{
	//QPRINTF("*------------------------\r\n");
}

static void app_menu_show_start(void)
{
	app_menu_show_line();
	//QPRINTF("* QN BLE Master Application\r\n");
}

/*	Heddoko main routine for Quintic	*/

static void app_menu_show_main(void)
{
	led_set(3, LED_ON);
	led_set(2, LED_ON);
	
	for(int i=0; i<3; i++)
		qn.id[i].number = 0;
	
	#ifdef DEBUG_MODE
	QPRINTF("Main app started\r\n");
	#endif
	QPRINTF("AppStart\r\n");
	clearNodConfigArray();
	for(int x = 0; x < 8; x++)
	{
				chmapArray[x].map[0] = 0xFF;
				chmapArray[x].map[1] = 0xFF;
				chmapArray[x].map[2] = 0xFF;
				chmapArray[x].map[3] = 0xFF;
				chmapArray[x].map[4] = 0x1F;
	}
	
	//	for(int x = 0; x < 8; x++)
//	{
//			if(x%2 == 0)
//			{
//				chmapArray_default[x].map[0] = 0xFF;
//				chmapArray_default[x].map[1] = 0xFF;
//				chmapArray_default[x].map[2] = 0xFF;
//				chmapArray_default[x].map[3] = 0xFF;
//				chmapArray_default[x].map[4] = 0x1F;
//			}
//			else
//			{
//				chmapArray_default[x].map[0] = 0xFF;
//				chmapArray_default[x].map[1] = 0xFF;
//				chmapArray_default[x].map[2] = 0xFF;
//				chmapArray_default[x].map[3] = 0xFF;
//				chmapArray_default[x].map[4] = 0x1F;
//			}
//	}
	
//	while(j==0){
//	
//		QPRINTF("QnAck\r\n");
//		QPRINTF("%d \r\n",j);
//		delay(300);
//		if(memcmp(app_env.input, ack, 3)==0)
//			j=1;
//	}
	
//	do{
//	
//		QPRINTF("QnAck\r\n");
//	}while(memcmp(app_env.input, ack, 3)==0);
	
	//spi_printf(QN_SPI0, )
}

//static void app_menu_show_main(void)
//{
//	app_menu_show_line();
//	QPRINTF("* QN BLE Main Menu\r\n");
//    QPRINTF("* 1. GAP   Menu\r\n");
//#if QN_SVC_DISC_USED
//    QPRINTF("* 2. GATT  Menu\r\n");
//#endif
//#if QN_SECURITY_ON
//	QPRINTF("* 3. SMP   Menu\r\n");
//#endif
//#if BLE_QPP_CLIENT
//    QPRINTF("* h. QPPC  Menu\r\n");
//#endif
//    QPRINTF("* r. Upper Menu\r\n");
//    QPRINTF("* s. Show  Menu\r\n");
//	app_menu_show_line();
//}

static void app_menu_handler_main(void)
{	
	//initialize the debug number variable
	for(int z = 0; z < QN_MAX_CONN; z++)
		qn.id[z].number = 0;
	
	//QPRINTF("main app handler");
	if(memcmp(app_env.input, ack, 3)==0)
		j=1;
	else if(memcmp(app_env.input, begin, 5)==0)
		menu_lvl=0;
	else if(memcmp(app_env.input, "chmap ", 6)==0)
		menu_lvl=2;
	else if(memcmp(app_env.input, scan, 4)==0)
		menu_lvl=2;
	else if(memcmp(app_env.input, connect, 7)==0)
		menu_lvl=3;
	else if(memcmp(app_env.input, start, 5)==0)
		menu_lvl=4;	
	else if(memcmp(app_env.input, stop, 4)==0)
		menu_lvl=4;
	uint16_t conhdl = 0;
	
	if(strncmp(app_env.input, "testCmd",7) == 0)
	{
			for (uint16_t i=0; i<app_env.cn_count; i++)
			{				
						#ifdef DEBUG_MODE
						QPRINTF("%d. %c %02X%02X%02X%02X%02X%02X \r\n", 
							i,
							app_env.addr_type[i] ? 'R' : 'P', 
							app_env.inq_addr[i].addr[5],
							app_env.inq_addr[i].addr[4],
							app_env.inq_addr[i].addr[3],
							app_env.inq_addr[i].addr[2],
							app_env.inq_addr[i].addr[1],
							app_env.inq_addr[i].addr[0]);
							//make request for all the channel maps
						#endif
							conhdl = app_get_conhdl_by_idx(i);
							app_gap_channel_map_req(true, conhdl, &chmapArray[0]); 				
			}
			//app_gap_dev_inq_req(GAP_KNOWN_DEV_INQ_TYPE, QN_ADDR_TYPE);
			//QPRINTF("QnAck\r\n");
	}
	if(strncmp(app_env.input, "getAccel",8) == 0)
	{
			char input_d[2] = {0x00,0x00};
			input_d[0] = 0x00;
			input_d[1] = 0x00;
			//StartReqFlag = 1; //just set the flag, it should already be registered for the characteristic 
			// GATTTOOL char-write-cmd 0x0043 0100
			//turn off euler notification and register for the acceleration characteristic. 
			//0x004f			
//			for (uint16_t i=0; i<app_env.cn_count; i++)
//			{
//				app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x0043,2,(uint8_t *)input_d);
//			}
			//enable the acceleration data
			if(app_env.input[8] == '1')
			{
				input_d[0] = 0x01;
				input_d[1] = 0x00;
				accelModeEnabled = 1;
			}
			else
			{
				input_d[0] = 0x00;
				input_d[1] = 0x00;
				accelModeEnabled = 0;
			}
			/*
			
			struct gatt_reliable_write
		{
    /// characteristic handle
    uint16_t elmt_hdl;
    /// value size
    uint16_t size;
    /// value holder
    uint8_t value[ATTM_RELIABLE_WRITE];
		};
			app_gatt_write_reliable_req(uint16_t conhdl, uint8_t nb_writes, uint8_t auto_execute, struct gatt_reliable_write *data_write)
			*/
			struct gatt_reliable_write gatt_write;
			gatt_write.elmt_hdl = 0x004f; 
			gatt_write.size = 2; 
			gatt_write.value[0] = input_d[0];
			gatt_write.value[1] = input_d[1];
			
			//app_gatt_write_reliable_req(uint16_t conhdl, 1, uint8_t auto_execute, struct gatt_reliable_write *data_write)
			for (uint16_t i=0; i<app_env.cn_count; i++)
			{
				QPRINTF("connHandle%d = %04x\r\n",i, app_env.dev_rec[i].conhdl); 
				
				app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x004f,2,(uint8_t *)input_d);
				//app_gatt_write_reliable_req(i, 1, 1, &gatt_write); 
			}
			QPRINTF("QnAck\r\n");	//Send ACK to brain_mcu
			
	}
	if(strncmp(app_env.input, "setMap",6) == 0)
	{
		vSetMapCount = 0;
		for (uint16_t i=0; i<app_env.cn_count; i++)
		{
			conhdl = app_get_conhdl_by_idx(i);
			app_gap_channel_map_req(true, conhdl, &chmapArray[i]);
			//QPRINTF("CMD-ACK\r\n");
//			vSetMapCount++;
//			if(vSetMapCount >= app_env.cn_count)
//				vSetMapCount = 0;
		}	
		//QPRINTF("QnAck\r\n");
	}
	
	if(strncmp(app_env.input, "checkMap",8) == 0)
	{	//TODO: when called this function returns ConnResp. why?
			vSetMapCount = app_env.cn_count;
			for (uint16_t i=0; i<app_env.cn_count; i++)
			{				
						#ifdef DEBUG_MODE
						QPRINTF("%d. %c %02X%02X%02X%02X%02X%02X \r\n", 
							i,
							app_env.addr_type[i] ? 'R' : 'P', 
							app_env.inq_addr[i].addr[5],
							app_env.inq_addr[i].addr[4],
							app_env.inq_addr[i].addr[3],
							app_env.inq_addr[i].addr[2],
							app_env.inq_addr[i].addr[1],
							app_env.inq_addr[i].addr[0]);
							//make request for all the channel maps
						#endif
				
							conhdl = app_get_conhdl_by_idx(i);
							app_gap_channel_map_req(false, conhdl, &chmapArray_default[0]); 				
			}
	}
	if(strncmp(app_env.input, "getEuler",8) == 0)
	{
			//app_gap_dev_inq_req(GAP_KNOWN_DEV_INQ_TYPE, QN_ADDR_TYPE);
			char input_d[2] = {0x00,0x00};
			input_d[0] = 0x00;
			input_d[1] = 0x00;
			StartReqFlag = 1; //just set the flag, it should already be registered for the characteristic 
			// GATTTOOL char-write-cmd 0x0043 0100
			//turn off acceleration notification and register for the acceleration characteristic. 
			//0x0043			
//			for (uint16_t i=0; i<app_env.cn_count; i++)
//			{
//				app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x004f,2,(uint8_t *)input_d);
//			}					
			input_d[0] = 0x01;
			input_d[1] = 0x00;
			for (uint16_t i=0; i<app_env.cn_count; i++)
			{
				app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x0043,2,(uint8_t *)input_d);
			}
			QPRINTF("QnAck\r\n");	//Send ACK to brain_mcu
			
	}	
	if(strncmp(app_env.input, "cancelCon",9) == 0)
	{
//			for (uint16_t i=0; i<app_env.cn_count; i++)
//			{				
//						QPRINTF("%d. %c %02X%02X%02X%02X%02X%02X \r\n", 
//							i,
//							app_env.addr_type[i] ? 'R' : 'P', 
//							app_env.inq_addr[i].addr[5],
//							app_env.inq_addr[i].addr[4],
//							app_env.inq_addr[i].addr[3],
//							app_env.inq_addr[i].addr[2],
//							app_env.inq_addr[i].addr[1],
//							app_env.inq_addr[i].addr[0]);
//							//make request for all the channel maps
//							
//							conhdl = app_get_conhdl_by_idx(i);
//							app_gap_channel_map_req(true, conhdl, &chmapArray[i]); 				
//			}
			app_gap_dev_inq_req(GAP_KNOWN_DEV_INQ_TYPE, QN_ADDR_TYPE);
	}
	
	if(menu_lvl==0)
	{
		//QPRINTF("menu level 0");		
		if(memcmp(app_env.input, begin, 5)==0)
		{
			char input_d[2] = {0x00,0x00};
			for(int z=0; z<BLE_CONNECTION_MAX; z++)
			{
				app_gatt_write_char_req(GATT_WRITE_CHAR,z,0x0043,2,(uint8_t *)input_d);
				app_gap_discon_req(z);
			}
			
			for(int z=0; z<9; z++)
			{
				for(int i=0; i<6; i++)
					nod[z][i]=0;
				qn.id[z].number = 0;
			}
			StartReqFlag = 0;
			ConnResp=0;
			ScanResp=0;
			expectedNodMask=0;
			clearNodConfigArray(); 
			z=0;
			#ifdef DEBUG_MODE
			QPRINTF("Send the NOD addresses to connect\r\n");
			#endif
			QPRINTF("QnAck\r\n");	//Send ACK to brain_mcu
			menu_lvl++;
		}
	}
		
	if(menu_lvl==1)
	{	//receive address and store in nod[][]
		//QPRINTF("menu level 1");
		//QPRINTF("Input address");
		
		if(memcmp(app_env.input, n, 3)==0)
		{			
			uint8_t j=0;
			for(uint8_t i=0;i<12;i=i+2)
			{	// fetch the address in ASCII and convert to HEX
				buf[z][i]=app_env.input[i];
				buf[z][i+1]=app_env.input[i+1];
				if(buf[z][i+1]>64)
					buf[z][i+1]=(buf[z][i+1]-55)&(0x0f);
				else
					buf[z][i+1]=(buf[z][i+1]-48)&(0x0f);
				if(buf[z][i]>64)
					buf[z][i]=((buf[z][i]-55)<<4)&(0xf0);
				else
					buf[z][i]=((buf[z][i]-48)<<4)&(0xf0);
				buf[z][j]=buf[z][i]|buf[z][i+1];
				j++;
			}
			for(uint8_t i=0; i<6; i++)
			{
				nod[z][5-i]=buf[z][i];
			}
			//copy the address into the nod config array. 
			memcpy((void*)(nodConfigArray[z].bdaAddress), nod[z], 6);
			#ifdef DEBUG_MODE
			QPRINTF("add received\r\n");
			QPRINTF("%02X%02X%02X%02X%02X%02X\r\n",nod[z][5],nod[z][4],nod[z][3],nod[z][2],nod[z][1],nod[z][0]);
			#endif
			QPRINTF("QnAck\r\n");	//Send ACK to brain_mcu
			expectedNodMask |= (1<<z);
			//increment the loaded nod number/index (which is called 'z' for no fucking reason... sean angry!)
			z++;
//			if(z>=QN_MAX_CONN)
//				menu_lvl++;
		}		
		if(memcmp(app_env.input, end, 3)==0)
		{
			QPRINTF("QnAck\r\n");
			menu_lvl++;
		}
	}
	
	if(menu_lvl>=2)
	{
		if(memcmp(app_env.input, "chmap", 5)==0)
		{
			//Received command to set channel map.
			uint8_t j=0;
			for(uint8_t i=0; i<10; i=i+2)
			{	// convert ASCII string to HEX
				chmapBuf[i] = app_env.input[i+6];
				chmapBuf[i+1] = app_env.input[i+1+6];
				if(chmapBuf[i+1] > 64)
					chmapBuf[i+1] = (chmapBuf[i+1]-55)&(0x0f);
				else
					chmapBuf[i+1] = (chmapBuf[i+1]-48)&(0x0f);
				if(chmapBuf[i] > 64)
					chmapBuf[i] = ((chmapBuf[i]-55)<<4)&(0xf0);
				else
					chmapBuf[i] = ((chmapBuf[i]-48)<<4)&(0xf0);
				if(j == 0)
				{	//The first half-byte should never be greater than 1 (adv channels)
					chmapBuf[j] = (chmapBuf[i]&0x10)|chmapBuf[i+1];
				}
				else
				{
					chmapBuf[j] = chmapBuf[i]|chmapBuf[i+1];
				}
				j++;
			}
			for(uint8_t i=0; i<5; i++)
			{
				chmapArray[0].map[4-i] = chmapBuf[i];
			}
			QPRINTF("QnAck\r\n");
			#ifdef DEBUG_MODE
			QPRINTF("New channel map received\r\n");
			QPRINTF("%02X%02X%02X%02X%02X\r\n",chmapArray[0].map[4],chmapArray[0].map[3],
							chmapArray[0].map[2],chmapArray[0].map[1],chmapArray[0].map[0]);
			#endif
		}
		
		//QPRINTF("menu level 2");
		if(memcmp(app_env.input, scan, 4)==0)
		{
			con_st_nb=0;
			con_st=0;
			#ifdef DEBUG_MODE
			QPRINTF("Scanning for NODs\r\n");
			#endif
			QnConNum = z-1;
			//QPRINTF("QnAck\r\n");	//Send ACK to brain_mcu - @ app_gap_task line 490
			app_gap_dev_inq_req(GAP_GEN_INQ_TYPE, QN_ADDR_TYPE);
			menu_lvl++;	
		}
//	}
//	
//	if(menu_lvl==3)
//	{
		if(memcmp(app_env.input, connect, 7)==0)
		{	
			con_st_nb=0;
			con_st=0;
			app_gap_le_cancel_conn_req(); //cancel any previous connection attempts.  
			#ifdef DEBUG_MODE
			QPRINTF("Connecting to NODs\r\n");
			#endif
			for(uint8_t i=0;i<=QnConNum;i++)
			{
				if(app_env.inq_idx>0)
				{				
					if (app_get_link_nb() != BLE_CONNECTION_MAX)
					{
						
						app_gap_le_create_conn_req(&app_env.inq_addr[i], app_env.addr_type[app_env.select_idx], QN_ADDR_TYPE, 
																				GAP_INIT_CONN_MIN_INTV, GAP_INIT_CONN_MAX_INTV, GAP_CONN_SUPERV_TIMEOUT);
						//QPRINTF("QnConAck\r\n");		//@ app_gap_task line 675
					}
				}
			}
			//menu_lvl++;
		}
//	}
//	
//	if(menu_lvl==4)
//	{	
		if(memcmp(app_env.input, start, 5)==0)
		{		
			char input_d[2] = {0x00,0x00};
			#ifdef DEBUG_MODE
			QPRINTF("Writing to receive notif");
			#endif
			input_d[0] = 0x01;
			input_d[1] = 0x00;
			StartReqFlag = 1; //just set the flag, it should already be registered for the characteristic 
			// GATTTOOL char-write-cmd 0x0043 0100
//			for (uint16_t i=0; i<app_env.cn_count; i++)
//			{
//				app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x0043,2,(uint8_t *)input_d);
//			}			
		}
		
		if(memcmp(app_env.input, stop, 4)==0)
		{		
			char input_d[2] = {0x00,0x00};
			#ifdef DEBUG_MODE
			QPRINTF("Writing to stop notif");
			#endif
			input_d[0] = 0x00;
			input_d[1] = 0x00;
			StartReqFlag = 0;
			// GATTTOOL char-write-cmd 0x0043 0100
//			for (uint16_t i=0; i<app_env.cn_count; i++)
//			{
//				app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x0043,2,(uint8_t *)input_d);
//			}
			
//			//Print Buffers
//			//if(flg==1){
//			
//				for(int z=0;z<QN_MAX_CONN;z++){
//				
//					QPRINTF("NOD id: %02x", z);
//					QPRINTF(" %02X%02X%02X%02X%02X%02X\r\n", 
//												nod[z][5],
//												nod[z][4],
//												nod[z][3],
//												nod[z][2],
//												nod[z][1],
//												nod[z][0]);
//					
//					for(int x=0;x<=qn.id[z].nb;x++){
//						for(int y=6;y<12;y++)
//							QPRINTF("%02X",qn.id[z].data[x][y]);
//						QPRINTF("\r\n");
//					}
//				}
//			QPRINTF("End\r\n");
//			//}
		}
		
		if(memcmp(app_env.input, check, 5) == 0)
		{
			//Send the current connection Status
			QPRINTF("ConnResp%d%d%d%d%d%d%d%d\r\n",   ((ConnResp>>0)&0x01), ((ConnResp>>1)&0x01),
																								((ConnResp>>2)&0x01), ((ConnResp>>3)&0x01),
																								((ConnResp>>4)&0x01), ((ConnResp>>5)&0x01),
																								((ConnResp>>6)&0x01), ((ConnResp>>7)&0x01));
		}
		
		if(memcmp(app_env.input, rssi, 4) == 0)
		{
			for (uint16_t i=0; i<=QnConNum; i++)
			{
				#ifdef DEBUG_MODE
				QPRINTF("Request to read RSSI\r\n");
				#endif
				app_gap_read_rssi_req(i);
			}
		}
	}
}


//static void app_menu_handler_main(void)
//{
//    switch (app_env.input[0])
//    {
//    case '1':
//        app_env.menu_id = menu_gap;
//        break;
//#if QN_SVC_DISC_USED
//    case '2':
//        app_env.menu_id = menu_gatt;
//        break;
//#endif
//#if (QN_SECURITY_ON)
//	case '3':
//        app_env.menu_id = menu_smp;
//        break;
//#endif
//#if BLE_DIS_CLIENT
//    case 'd':
//        app_env.menu_id = menu_disc;
//        break;
//#endif
//#if BLE_QPP_CLIENT
//    case 'h':
//        app_env.menu_id = menu_qppc;
//        break;
//#endif
//    case 'r':
//    case 's':
//    default:
//        break;
//    }
//    app_menu_show();
//}

static void app_menu_show_gap(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE GAP Menu\r\n");
    QPRINTF("* 1. Inquiry Start\r\n");
    QPRINTF("* 2. Inquiry Stop\r\n");
#if (BLE_PERIPHERAL)
    QPRINTF("* 3. Advertisement Start\r\n");
    QPRINTF("* 4. Advertisement Stop\r\n");
#endif
    QPRINTF("* 5. Create Connection\r\n");
    QPRINTF("* 6. Disconnection\r\n");
#if (QN_SECURITY_ON)
    QPRINTF("* 7. Pair Request\r\n");
#endif
    QPRINTF("* 8. UnPair\r\n");
		QPRINTF(" 'r' Go back\r\n");
		QPRINTF(" 's' Show menu\r\n");
	app_menu_show_line();
}


static void app_menu_handler_gap(void)
{
    switch (app_env.input[0])
    {
    case '1':
			app_gap_dev_inq_req(GAP_GEN_INQ_TYPE, QN_ADDR_TYPE);	// try GAP_KNOWN_DEV_INQ_TYPE
        break;
    case '2':
        app_gap_dev_inq_cancel_req();
        break;
#if (BLE_PERIPHERAL)
    case '3':
        app_gap_adv_start_req(GAP_GEN_DISCOVERABLE|GAP_UND_CONNECTABLE, 
            app_env.adv_data, app_set_adv_data(GAP_GEN_DISCOVERABLE), 
            app_env.scanrsp_data, app_set_scan_rsp_data(app_get_local_service_flag()),
            GAP_ADV_FAST_INTV1, GAP_ADV_FAST_INTV2);
        break;
    case '4':
        app_gap_adv_stop_req();
        break;
#endif
    case '5':
        app_env.menu_id = menu_gap_create_connection;
        app_menu_show();
        break;
    case '6':
        app_env.menu_id = menu_gap_disconnection;
        app_menu_show();
        break;
#if (QN_SECURITY_ON)
    case '7':
        app_env.menu_id = menu_gap_bond;
        app_menu_show();
        break;
#endif
    case '8':
        app_env.menu_id = menu_gap_unpair;
        app_menu_show();
        break;
    case 'r':
        app_env.menu_id = menu_main;
    case 's':
    default:
        app_menu_show();
        break;
    }
}

static void app_menu_show_gap_inq_record(void)
{
    if (app_env.inq_idx >= 0)
    {
        QPRINTF("\r\n");
        QPRINTF("Inquiry device record:\r\n");
        for (uint8_t i = 0; i < app_env.inq_idx; i++)
        {
            QPRINTF("%d. %02X%02X%02X%02X%02X%02X\r\n", i, 
                app_env.inq_addr[i].addr[5],
                app_env.inq_addr[i].addr[4],
                app_env.inq_addr[i].addr[3],
                app_env.inq_addr[i].addr[2],
                app_env.inq_addr[i].addr[1],
                app_env.inq_addr[i].addr[0]);
        }
        QPRINTF("Please input the device index:\r\n");
    }
    else
    {
        QPRINTF("Inquiry device record is NULL.\r\n");
        app_env.menu_id = menu_gap;
    }
}

static void app_menu_show_gap_dev_record(void)
{
    if (app_get_link_nb())
    {
        QPRINTF("\r\n");
        QPRINTF("Connected device records (%d):\r\n", app_get_link_nb());
        for (uint8_t idx = 0; idx < BLE_CONNECTION_MAX; idx++)
        {
            struct bd_addr peer_addr;
            if (app_get_bd_addr_by_idx(idx, &peer_addr))
            {
                QPRINTF("%d. %02X%02X%02X%02X%02X%02X\r\n", idx, 
                    peer_addr.addr[5],
                    peer_addr.addr[4],
                    peer_addr.addr[3],
                    peer_addr.addr[2],
                    peer_addr.addr[1],
                    peer_addr.addr[0]);
            }
        }
        if (app_env.menu_id != menu_gatt_disc_all_svc
         && app_env.menu_id != menu_gap_disconnection
         && app_env.menu_id != menu_gap_bond
         && app_env.menu_id != menu_disc_enable
         )
            QPRINTF("a. All device\r\n");
        QPRINTF("Please input the device index:\r\n");
    }
    else
    {
        QPRINTF("Connected device record is NULL.\r\n");
        if (app_env.menu_id == menu_gatt_disc_all_svc)
            app_env.menu_id = menu_gatt;
        else if (app_env.menu_id == menu_disc_enable)
            app_env.menu_id = menu_disc;
        else
            app_env.menu_id = menu_gap;
    }
}

static void app_menu_show_gap_bond_record(void)
{
#if (QN_SECURITY_ON)
    uint8_t nb = app_get_bond_nb();
#else
    uint8_t nb = 0;
#endif
    if (nb > 0)
    {
        QPRINTF("\r\n");
        QPRINTF("Bonded device records (%d):\r\n", nb);
        for (uint8_t idx = 0; idx < APP_MAX_BONDED_DEVICE_NUMBER && idx < BLE_CONNECTION_MAX; idx++)
        {
            if (app_get_bond_status(idx, SMP_KDIST_ENCKEY|SMP_KDIST_SIGNKEY|SMP_KDIST_SIGNKEY))
            {
                QPRINTF("%d. %02X%02X%02X%02X%02X%02X\r\n", idx, 
                    app_env.dev_rec[idx].bonded_info.peer_addr.addr[5],
                    app_env.dev_rec[idx].bonded_info.peer_addr.addr[4],
                    app_env.dev_rec[idx].bonded_info.peer_addr.addr[3],
                    app_env.dev_rec[idx].bonded_info.peer_addr.addr[2],
                    app_env.dev_rec[idx].bonded_info.peer_addr.addr[1],
                    app_env.dev_rec[idx].bonded_info.peer_addr.addr[0]);
            }
        }
        if (app_env.menu_id != menu_gatt_disc_all_svc
            &&
            app_env.menu_id != menu_gap_unpair)
            QPRINTF("a. All device\r\n");
        QPRINTF("Please input the device index:\r\n");
    }
    else
    {
        QPRINTF("Bonded device record is NULL.\r\n");
        app_env.menu_id = menu_gap;
    }
}

static void app_menu_handler_gap_dev_select(void)
{
    if (app_env.input[0] - '0' < BLE_CONNECTION_MAX)
    {
        app_env.select_idx = app_env.input[0] - '0';

        switch (app_env.menu_id)
        {
#if (QN_SECURITY_ON)
        case menu_gap_bond:
            {
                struct bd_addr addr;
                uint8_t auth;
                if (app_get_bd_addr_by_idx(app_env.select_idx, &addr))
                {
                    /// Authentication Requirements
                    switch (app_env.sec_lvl_set)
                    {
                    case GAP_SEC1_AUTH_PAIR_ENC:
                        auth = SMP_AUTH_REQ_MITM_BOND;
                        break;
                    case GAP_SEC1_NOAUTH_PAIR_ENC:
                        auth = SMP_AUTH_REQ_NO_MITM_BOND;
                        break;
                    case GAP_NO_SEC:
                    default:
                        auth = SMP_AUTH_REQ_NO_MITM_BOND;
                        break;
                    }
                    app_gap_bond_req(&addr, SMP_OOB_AUTH_DATA_NOT_PRESENT, auth, app_env.iocap_set);
                }
                else
                    QPRINTF("The selected device is not in connected status.\r\n");
            }
            break;
        case menu_gap_unpair:
            {
                struct bd_addr addr;
                if (app_get_bd_addr_by_idx(app_env.select_idx, &addr))
                {
                    app_gap_unpair_req(&addr, app_get_bond_nb());
                    // TODO: Deleted paired information form NVDS
                    QPRINTF("Updated link bonded info success.\r\n");
                }
                else
                    QPRINTF("The selected device is not in bonded status.\r\n");
            }
            break;
#endif
        case menu_gap_create_connection:
            if (app_get_link_nb() != BLE_CONNECTION_MAX)
            {
                app_gap_le_create_conn_req(&app_env.inq_addr[app_env.select_idx], app_env.addr_type[app_env.select_idx], QN_ADDR_TYPE, 
                                           GAP_INIT_CONN_MIN_INTV, GAP_INIT_CONN_MAX_INTV, GAP_CONN_SUPERV_TIMEOUT);
            }
            else
            {
                QPRINTF("Connection Limit Exceeded\r\n");
            }
            break;
        case menu_gap_disconnection:
            if (app_env.input[0] == 'a')
            {
                for (uint8_t idx = 0; idx < BLE_CONNECTION_MAX; idx++)
                {
                    uint16_t conhdl = app_get_conhdl_by_idx(idx);
                    if (conhdl != 0xFFFF)
                    {
                        app_gap_discon_req(conhdl);
                    }
                }
            }
            else
            {
                uint16_t conhdl = app_get_conhdl_by_idx(app_env.select_idx);
                if (conhdl != 0xFFFF)
                    app_gap_discon_req(conhdl);
                else
                    QPRINTF("The selected device is not in connected status.\r\n");
            }
            break;
        case menu_gatt_disc_all_svc:
            {
                uint16_t conhdl = app_get_conhdl_by_idx(app_env.select_idx);
                if (conhdl != 0xFFFF)
                {
#if QN_SVC_DISC_USED
                    app_env.dev_rec[app_env.select_idx].nb_svc = 0;
                    app_gatt_disc_svc_req(GATT_DISC_ALL_SVC, conhdl);
#endif
                }
                else
                    QPRINTF("The selected device is not in connected status.\r\n");
            }
            break;
						case menu_qppc_enable:
							QPRINTF("Select success.\r\n");
							break;
        default:
            break;
        }
    }
    switch (app_env.input[0])
    {
    case 'r':
    default:
        if (app_env.menu_id == menu_disc_enable)
            app_env.menu_id = menu_disc;
        else if (app_env.menu_id == menu_gatt_disc_all_svc)
            app_env.menu_id = menu_gatt;
				else if (app_env.menu_id == menu_qppc_enable)
            app_env.menu_id = menu_qppc;
        else
            app_env.menu_id = menu_gap;
        break;
    case 's':
        app_menu_show();
        break;
    }
}


#if QN_SVC_DISC_USED
static void app_menu_show_gatt(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE GATT Menu\r\n");
    QPRINTF("* 1. Discovery All Services\r\n");
    QPRINTF("* 2. Display All Services\r\n");
		QPRINTF("* 3. Extended Features\r\n"); // HEDDOKO - added entry here
		QPRINTF(" 'r' Go back\r\n");
		QPRINTF(" 's' Show menu\r\n");
	app_menu_show_line();
}

/**
 * app_menu_show_gatt_ext() //HEDDOKO
 * @brief Add extended menu to be reached through GATT on the main
 * menu from terminal
 */
static void app_menu_show_gatt_ext(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE GATT EXT Menu\r\n");
    QPRINTF("* 1. Write CMD - not used \r\n");
    QPRINTF("* 2. Write Req\r\n");
		QPRINTF("* 3. Read - not used so far\r\n"); 
	  QPRINTF("* 4. Setup Notif Handle and Recv\r\n");
		QPRINTF("* 5. Send data to paired smart device\r\n");
		QPRINTF(" 'r' Go back\r\n");
		QPRINTF(" 's' Show	menu\r\n");
	app_menu_show_line();
}

/**
 * app_menu_show_gatt_ext_evt_notif() //HEDDOKO
 * @brief Notification handle menu handler
 */
static void app_menu_show_gatt_ext_evt_notif(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE GATT EXT Notif Handle Menu\r\n");
    QPRINTF("* 1. Write to NOD notif hdl to recieve data\r\n");
		QPRINTF("* 2. Write to NOD notif hdl to stop data\r\n");
		QPRINTF("* 3. Read NOD notif hdl value\r\n");
		QPRINTF(" 'r' Go back\r\n");
		QPRINTF(" 's' Show menu\r\n");
	app_menu_show_line();
}

/**
 * app_menu_show_gatt_ext_write_req() //HEDDOKO
 * @brief Write menu handler - LED type stuff should go here
 */
static void app_menu_show_gatt_ext_write_req(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE GATT EXT Write REQ Menu\r\n");
    QPRINTF("* 1. Write Req (LEDS ON)\r\n");
	  QPRINTF("* 2. Write Req (LEDS OFF)\r\n");
		QPRINTF(" 'r' Go back\r\n");
		QPRINTF(" 's' Show menu\r\n");
	app_menu_show_line();
}

/**
 * app_menu_show_gatt_ext_write_cmd() //HEDDOKO
 * @brief not used
 */
static void app_menu_show_gatt_ext_write_cmd(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE GATT EXT Write CMD Menu\r\n");
    QPRINTF("* 1. Write CMD\r\n");
		QPRINTF(" 'r' Go back\r\n");
		QPRINTF(" 's' Show menu\r\n");
	app_menu_show_line();
}

/**
 * app_menu_show_gatt_ext_read() // HEDDOKO
 * @brief not used
 */
static void app_menu_show_gatt_ext_read(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE GATT EXT Read Menu\r\n");
		QPRINTF("* 1. Read\r\n"); 
		QPRINTF(" 'r' Go back\r\n");
		QPRINTF(" 's' Show menu\r\n");
	app_menu_show_line();
}

static void app_menu_show_gatt_disp_all_svc(void)
{
    if (app_get_link_nb())
    {
        QPRINTF("\r\n");
        QPRINTF("Service record:\r\n");
        for (uint8_t idx = 0; idx < BLE_CONNECTION_MAX; idx++)
        {
            struct bd_addr peer_addr;
            if (app_get_bd_addr_by_idx(idx, &peer_addr))
            {
                QPRINTF("%d. %02x%02x%02x%02x%02x%02x\r\n", idx, 
                    peer_addr.addr[5],
                    peer_addr.addr[4],
                    peer_addr.addr[3],
                    peer_addr.addr[2],
                    peer_addr.addr[1],
                    peer_addr.addr[0]);
                for (uint8_t i = 0; i < app_env.dev_rec[idx].nb_svc; i++)
                {
                     QPRINTF("%04X ",app_env.dev_rec[idx].svc_rec[i].attr_hdl);
                }
                QPRINTF("\r\n");
            }
        }
    }
    else
    {
        QPRINTF("Service record is NULL.\r\n");
    }
    app_env.menu_id = menu_gatt;
}


/**
 * app_menu_handler_gatt_send_data() //HEDDOKO
 *	@brief Sends data to the connected smart device
 */

static void app_menu_handler_gatt_send_data(void){

	
}

/**
 * app_menu_handler_gatt_ext_evt_notif() //HEDDOKO
 * @brief Handles movement notification: start, stop, read
 */
static void app_menu_handler_gatt_ext_evt_notif(void) {
	  
		char input_d[2] = {0x00,0x00};
		
		switch (app_env.input[0]){
		case '1':
			// Write to notif hdl
			input_d[0] = 0x01;
			input_d[1] = 0x00;
			
			// GATTTOOL char-write-cmd 0x0043 0100
			for (uint16_t i=0; i<app_env.cn_count; i++){
			app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x0043,2,(uint8_t *)input_d);	
			}
			break;
		case '2':
			// Write to notif handle to stop
			input_d[0] = 0x00;
			input_d[1] = 0x00;
			
			// GATTTOOL char-write-cmd 0x0043 0000
			for (uint16_t i=0; i<app_env.cn_count; i++){
			app_gatt_write_char_req(GATT_WRITE_CHAR,i,0x0043,2,(uint8_t *)input_d);
			}
			break;
		case '3':
			for (uint16_t i=0; i<app_env.cn_count; i++){
			app_gatt_read_char_req(GATT_READ_CHAR,i,0x0043);
			}
			break;
		 case 'r':
      app_env.menu_id = menu_gatt_ext;
      break;
    case 's':
    default:
        break;
	}
	app_menu_show();
}

/**
 * app_menu_handler_gatt_ext_write_cmd(void) // HEDDOKO
 * @brief Sets up the write menu similarly to the GATT tool, but uses
 * a text based GUI over serial. DOES NOTHING RIGHT NOW
 */
static void app_menu_handler_gatt_ext_write_cmd(void) {
	switch (app_env.input[0]){
		case '1':
			// Enable feature x
			break;
		 case 'r':
      app_env.menu_id = menu_gatt_ext;
      break;
    case 's':
    default:
        break;
	}
	app_menu_show();
	
}

/**
 * app_menu_handler_gatt_ext_write_req() // HEDDOKO
 * @brief Handles GATT writes to 0x0062 for LEDS as per documentation,
 * can also turn on and off LEDS
 */
static void app_menu_handler_gatt_ext_write_req(void) {
	char input_d[4] = {0x00,0x00,0x00,0x00};
	switch (app_env.input[0]){
		case '1':
			// TURN ON ALL LEDS AT MAX BRIGHTNESS
			input_d[0] = 0x0F; // Command (from NOD doc)
			input_d[1] = 0x64; // Red LED (brightness between 0-64)
			input_d[2] = 0x64; // White LED
			input_d[3] = 0x64; // Blue LED

			app_gatt_write_char_req(GATT_WRITE_CHAR,0,0x0062,4,(uint8_t *)input_d);
			break;
		case '2':
			// TURN OFF LEDS
			input_d[0] = 0x0F;
			input_d[1] = 0x00;
			input_d[2] = 0x00;
			input_d[3] = 0x00;

			app_gatt_write_char_req(GATT_WRITE_CHAR,0,0x0062,4,(uint8_t *)input_d);			
		 case 'r':
      app_env.menu_id = menu_gatt_ext;
      break;
    case 's':
    default:
        break;
	}
	app_menu_show();
	
}

/**
 * app_menu_handler_gatt_ext_read() // HEDDOKO
 * @brief GATT ext menu handler DOES nothing yet
 */
static void app_menu_handler_gatt_ext_read(void) {
	switch (app_env.input[0]){
		case '1':
			// Enable feature x
			break;
		 case 'r':
      app_env.menu_id = menu_gatt;
      break;
    case 's':
    default:
        break;
	}
	app_menu_show();
	
}

/**
 * app_menu_handler_gatt_ext() // HEDDOKO
 * @brief GATT menu handler for read, write (req is no diff than command so use req),
 * then setup notif stuff the evt menu
 */
static void app_menu_handler_gatt_ext(void)
{
	switch (app_env.input[0]){
		case '1':
			app_env.menu_id = menu_gatt_ext_write_cmd;
			break;
		case '2':
			app_env.menu_id = menu_gatt_ext_write_req;
			break;
		case '3':
			app_env.menu_id = menu_gatt_ext_read;
			break;
		case '4':
			app_env.menu_id = menu_gatt_ext_evt_handle;
			break;
		case '5':
			app_env.menu_id = menu_gatt_send_data;
			break;
    case 'r':
      app_env.menu_id = menu_gatt;
      break;
    case 's':
    default:
        break;
	}
	app_menu_show();
}

/**
 * app_menu_handler_gatt()
 * @brief GATT handler for menu
 */
static void app_menu_handler_gatt(void)
{
    switch (app_env.input[0])
    {
    case '1':
        app_env.menu_id = menu_gatt_disc_all_svc;
        break;
    case '2':
        app_env.menu_id = menu_gatt_disp_all_svc;
        break;
		case '3':
        app_env.menu_id = menu_gatt_ext; // HEDDOKO
        break;
    case 'r':
        app_env.menu_id = menu_main;
        break;
    case 's':
    default:
        break;
    }
    app_menu_show();
}
#endif

#if (QN_SECURITY_ON)
static void app_menu_show_smp(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE SMP Menu\r\n");
    QPRINTF("* 1. Security Level setting\r\n");
    QPRINTF("* Default: ");
    switch (app_env.sec_lvl_set)
    {
    case GAP_SEC1_AUTH_PAIR_ENC:
        QPRINTF("M1L3\r\n");
        break;
    case GAP_SEC1_NOAUTH_PAIR_ENC:
        QPRINTF("M1L2\r\n");
        break;
    case GAP_NO_SEC:
    default:
        QPRINTF("M1L1\r\n");
        break;
    }
    QPRINTF("* Note: Change before Adv/Scan/Connect\r\n");
    QPRINTF("* 2. IO Capbility setting\r\n");
    QPRINTF("* Default: ");
    switch (app_env.iocap_set)
    {
    case SMP_IO_CAP_KB_DISPLAY:
        QPRINTF("Keyboard Display\r\n");
        break;
    case SMP_IO_CAP_DISPLAY_ONLY:
        QPRINTF("Display Only\r\n");
        break;
    case SMP_IO_CAP_NO_INPUT_NO_OUTPUT:
    default:
        QPRINTF("No Input No Output\r\n");
        break;
    }
	app_menu_show_line();
}

static void app_menu_handler_smp(void)
{
    switch (app_env.input[0])
    {
    case '1':
        app_env.menu_id = menu_smp_sec_lvl_set;
        break;
    case '2':
        app_env.menu_id = menu_smp_io_cap_set;
        break;
    case 'r':
        app_env.menu_id = menu_main;
        break;
    case 's':
    default:
        break;
    }
    app_menu_show();
}

static void app_menu_show_smp_sec_lvl_set(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE Security Level\r\n");
    QPRINTF("* 0. M1L1\r\n");
    QPRINTF("* 1. M1L2\r\n");
    QPRINTF("* 2. M1L3\r\n");
    QPRINTF("* 3. M2L1\r\n");
    QPRINTF("* 4. M2L2\r\n");
	app_menu_show_line();
}

static void app_menu_handler_smp_sec_lvl_set(void)
{
    switch (app_env.input[0])
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
        app_env.sec_lvl_set = app_env.input[0] - '0';
        app_gap_set_sec_req(app_env.sec_lvl_set);
        break;
    case 'r':
        app_env.menu_id = menu_smp;
        break;
    case 's':
    default:
        break;
    }
    app_menu_show();
}

static void app_menu_show_smp_iocap_set(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE SMP IO Capability\r\n");
    QPRINTF("* 0. Display Only\r\n");
    QPRINTF("* 1. Display YesNo\r\n");
    QPRINTF("* 2. Keyboard Only\r\n");
    QPRINTF("* 3. No Input No Output\r\n");
    QPRINTF("* 4. Keyboard Display\r\n");
	app_menu_show_line();
}

static void app_menu_handler_smp_iocap_set(void)
{
    switch (app_env.input[0])
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
        app_env.iocap_set = app_env.input[0] - '0';
        break;
    case 'r':
        app_env.menu_id = menu_smp;
        break;
    case 's':
    default:
        break;
    }
    app_menu_show();
}

static void app_menu_show_smp_tk_input(void)
{
	QPRINTF("Please input the passkey:\r\n");
}

static void app_menu_handler_smp_tk_input(void)
{
    struct smp_key tk = {0};
    uint32_t i = atoi((char *)app_env.input);
    if (i <= 999999)
    {
        app_i_to_array(i, (char *)tk.key);
        app_smpc_tk_req_rsp(app_env.tk_idx, CO_ERROR_NO_ERROR, &tk);
    }
    else
    {
        app_smpc_tk_req_rsp(app_env.tk_idx, !CO_ERROR_NO_ERROR, &tk);
    }
    app_env.menu_id = menu_gap;
}
#endif

#if BLE_QPP_CLIENT
static void app_menu_show_qppc(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE QPPC Menu\r\n");
    QPRINTF("* 0. Select device\r\n");
    QPRINTF("* 1. Enable\r\n");
		QPRINTF("* 2. Send data to smart device\r\n");	//HEDDOKO stuff
	app_menu_show_line();
}

static void app_menu_handler_qppc(void)
{
    uint8_t idx = app_env.select_idx;

    switch (app_env.input[0])
    {
    case '0':
        app_env.menu_id = menu_qppc_enable;
        break;
    case '1':
    {
        uint16_t conhdl = app_get_conhdl_by_idx(app_env.select_idx);

        if (0xFFFF != conhdl && idx < BLE_CONNECTION_MAX
            && false == app_get_qpp_client_service_status(idx))
        {
            app_qppc_enable_req(NULL, conhdl);
        }
        else
            QPRINTF("Enable QPPC disallowed.\r\n");
        break;
    }
		case '2':	//HEDDOKO: function to send data to smart device
			
			break;
		
    case 'r':
        app_env.menu_id = menu_main;
        break;
    case 's':
    default:
        break;
    }
    app_menu_show();
}
#endif

#if BLE_DIS_CLIENT
static void app_menu_show_disc(void)
{
	app_menu_show_line();
	QPRINTF("* QN BLE DISC Menu\r\n");
    QPRINTF("* 0. Select device\r\n");
    QPRINTF("* 1. Enable\r\n");
    QPRINTF("* 2. Read Manufacturer name\r\n");
    QPRINTF("* 3. Read Model Number\r\n");
    QPRINTF("* 4. Read Serial Number\r\n");
    QPRINTF("* 5. Read Hardware Revision\r\n");
    QPRINTF("* 6. Read Firmware Revision\r\n");
    QPRINTF("* 7. Read Software Revision\r\n");
    QPRINTF("* 8. Read System ID\r\n");
    QPRINTF("* 9. Read IEEE\r\n");
    QPRINTF("* a. Read PnP ID\r\n");
		QPRINTF(" 'r' Go back\r\n");
		QPRINTF(" 's' Show menu\r\n");
	app_menu_show_line();
}

static void app_menu_handler_disc(void)
{
    uint8_t idx = app_env.select_idx;

    switch (app_env.input[0])
    {
    case '0':
        app_env.menu_id = menu_disc_enable;
        break;
    case '1':
    {
        uint16_t conhdl = app_get_conhdl_by_idx(idx);
        if (0xFFFF != conhdl
            && false == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_enable_req(NULL, conhdl);
        }
        else
            QPRINTF("Enable DISC disallowed.\r\n");
        break;
    }
    case '2':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_MANUFACTURER_NAME_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case '3':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_MODEL_NB_STR_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case '4':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_SERIAL_NB_STR_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case '5':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_HARD_REV_STR_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case '6':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_FIRM_REV_STR_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case '7':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_SW_REV_STR_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case '8':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_SYSTEM_ID_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case '9':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_IEEE_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case 'a':
        if (true == app_get_client_service_status(idx, ATT_SVC_DEVICE_INFO))
        {
            app_disc_rd_char_req(DISC_PNP_ID_CHAR, app_disc_env[idx].conhdl);
        }
        break;
    case 'r':
        app_env.menu_id = menu_main;
        break;
    case 's':
    default:
        break;
    }
    app_menu_show();
}
#endif


/**
 ****************************************************************************************
 * @brief Show the BLE demo application.
 *
 ****************************************************************************************
 */
void app_menu_show(void)
{
    switch (app_env.menu_id)
    {
    case menu_start:
        app_menu_show_start();
        break;
    case menu_main:
        app_menu_show_main();
        break;
    case menu_gap:
        app_menu_show_gap();
        break;
#if QN_SVC_DISC_USED
	case menu_gatt:
        app_menu_show_gatt();
        break;
    case menu_gatt_disp_all_svc:
        app_menu_show_gatt_disp_all_svc();
        break;
		case menu_gatt_ext:
				app_menu_show_gatt_ext(); // HEDDOKO
				break;
		case menu_gatt_ext_read:
				app_menu_show_gatt_ext_read(); // HEDDOKO
				break;
		case menu_gatt_ext_write_req:
				app_menu_show_gatt_ext_write_req(); // HEDDOKO
				break;
		case menu_gatt_ext_write_cmd:
				app_menu_show_gatt_ext_write_cmd(); // HEDDOKO
				break;
		case menu_gatt_ext_evt_handle:
				app_menu_show_gatt_ext_evt_notif(); // HEDDOKO
				break;
		case menu_gatt_send_data:
				app_menu_show_gap_bond_record();	//HEDDOKO
				break;
#endif
    case menu_gap_create_connection:
        app_menu_show_gap_inq_record();
        break;
    case menu_gap_unpair:
        app_menu_show_gap_bond_record();
        break;
    case menu_gap_bond:
    case menu_gap_disconnection:
    case menu_gatt_disc_all_svc:
    case menu_disc_enable:
		case menu_qppc_enable:
			 app_menu_show_gap_dev_record();
       break;
#if QN_SECURITY_ON
    case menu_smp:
        app_menu_show_smp();
        break;
    case menu_smp_sec_lvl_set:
        app_menu_show_smp_sec_lvl_set();
        break;
    case menu_smp_io_cap_set:
        app_menu_show_smp_iocap_set();
        break;
    case menu_smp_tk_input:
        app_menu_show_smp_tk_input();
        break;
#endif

#if BLE_DIS_CLIENT
    case menu_disc:
        app_menu_show_disc();
        break;
#endif
#if BLE_QPP_CLIENT
    case menu_qppc:
        app_menu_show_qppc();
        break;
#endif
    default:
        break;
    }
}

/**
 ****************************************************************************************
 * @brief Handler function of the BLE demo application.
 *
 ****************************************************************************************
 */
void app_menu_hdl(void)
{
    switch (app_env.menu_id)
    {
    case menu_start:
        break;
    case menu_main:
        app_menu_handler_main();
        break;
    case menu_gap:
        app_menu_handler_gap();
        break;
#if QN_SVC_DISC_USED
    case menu_gatt:
        app_menu_handler_gatt();
        break;
		case menu_gatt_ext:
				app_menu_handler_gatt_ext(); // HEDDOKO
				break;
		case menu_gatt_ext_write_cmd:
				app_menu_handler_gatt_ext_write_cmd(); // HEDDOKO
				break;
		case menu_gatt_ext_write_req:
				app_menu_handler_gatt_ext_write_req(); // HEDDOKO
				break;
	 case menu_gatt_ext_read:
				app_menu_handler_gatt_ext_read(); // HEDDOKO
				break;
	 case menu_gatt_ext_evt_handle:
				app_menu_handler_gatt_ext_evt_notif(); // HEDDOKO
				break;
	 case menu_gatt_send_data:
				app_menu_handler_gatt_send_data();
				break;
#endif		
#if QN_SECURITY_ON
	case menu_smp:
        app_menu_handler_smp();
        break;
#endif
#if BLE_QPP_CLIENT
    case menu_qppc:
        app_menu_handler_qppc();
        break;
#endif
#if BLE_DIS_CLIENT
    case menu_disc:
        app_menu_handler_disc();
        break;
#endif
    case menu_gap_bond:
    case menu_gap_unpair:
    case menu_gap_create_connection:
    case menu_gap_disconnection:

#if QN_SVC_DISC_USED
    case menu_gatt_disc_all_svc:
#endif
		case menu_qppc_enable:	
			app_menu_handler_gap_dev_select();
			break;
#if (QN_SECURITY_ON)
    case menu_smp_sec_lvl_set:
        app_menu_handler_smp_sec_lvl_set();
        break;
    case menu_smp_io_cap_set:
        app_menu_handler_smp_iocap_set();
        break;
     case menu_smp_tk_input:
        app_menu_handler_smp_tk_input();
        break;
#endif
    default:
        break;
    }
}

/**
 ****************************************************************************************
 * @brief Callback function of MENU.
 *
 ****************************************************************************************
 */
void app_menu_cb(ke_msg_id_t const msgid, void *param)
{
	ke_state_t state = (ke_state_get(TASK_APP));
	
    if (APP_INIT == state)
    {
	    switch (msgid)
	    {
        case GAP_READY_EVT:
	    case GAP_RESET_REQ_CMP_EVT:
			app_env.menu_id = menu_start;
			app_menu_show();
	        break;
		case GAP_READ_VER_REQ_CMP_EVT:
//	        QPRINTF("*\r\n");
//	        QPRINTF("* Version:\r\n");
//	        QPRINTF("*   AP%sHL%1x.%1x.%1xLL%1x.%1x.%1x\r\n",	//Commented out for Heddoko
//	            QN_APP_VER,
//	            ((struct gap_read_ver_req_cmp_evt *)param)->host_ver,
//	            ((struct gap_read_ver_req_cmp_evt *)param)->host_subver>>8&0xFF,
//	            ((struct gap_read_ver_req_cmp_evt *)param)->host_subver&0xFF,
//	            ((struct gap_read_ver_req_cmp_evt *)param)->lmp_ver,
//	            ((struct gap_read_ver_req_cmp_evt *)param)->lmp_subver>>8&0xFF,
//	            ((struct gap_read_ver_req_cmp_evt *)param)->lmp_subver&0xFF);
			break;
		case GAP_READ_BDADDR_REQ_CMP_EVT:
//			QPRINTF("* BD Address:\r\n");
//			QPRINTF("*          %02x%02x%02x%02x%02x%02x\r\n",	//Commented out for Heddoko
//				((struct gap_read_bdaddr_req_cmp_evt *)param)->addr.addr[5],
//				((struct gap_read_bdaddr_req_cmp_evt *)param)->addr.addr[4],
//				((struct gap_read_bdaddr_req_cmp_evt *)param)->addr.addr[3],
//				((struct gap_read_bdaddr_req_cmp_evt *)param)->addr.addr[2],
//				((struct gap_read_bdaddr_req_cmp_evt *)param)->addr.addr[1],
//				((struct gap_read_bdaddr_req_cmp_evt *)param)->addr.addr[0]);
//			QPRINTF("*\r\n");
//			QPRINTF("* By Derek\r\n");
//			QPRINTF("* @ %11s, %8s\r\n",	//Commented out for Heddoko
//				__DATE__, 
//				__TIME__);
//			QPRINTF("*------------------------\r\n");		
			app_env.menu_id = menu_main;
			app_menu_show();
			break;
			
		default:
			break;
	    }
    }
    else
	if (APP_IDLE == state)
    {
	    switch (msgid)
	    {
#if (QN_SECURITY_ON)
	    case SMPC_TK_REQ_IND:
			if (app_env.tk_type != 0)
			{	   
				app_env.tk_idx = ((struct smpc_tk_req_ind *)param)->idx;
				app_env.menu_id = menu_smp_tk_input;
				app_menu_show();
			}
	        break;
#endif
		default:
			break;
	    }
    }
}
#endif
/// @} APP
