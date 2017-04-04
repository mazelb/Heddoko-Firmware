/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Copyright Heddoko(TM) 2016, all rights reserved */
/* Generated from: heddokoPacket.proto */

#ifndef PROTOBUF_C_heddokoPacket_2eproto__INCLUDED
#define PROTOBUF_C_heddokoPacket_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1002001 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Heddoko__ImuDataFrame Heddoko__ImuDataFrame;
typedef struct _Heddoko__FullDataFrame Heddoko__FullDataFrame;
typedef struct _Heddoko__Endpoint Heddoko__Endpoint;
typedef struct _Heddoko__WifiConfiguration Heddoko__WifiConfiguration;
typedef struct _Heddoko__RecordingsResponse Heddoko__RecordingsResponse;
typedef struct _Heddoko__FileDownload Heddoko__FileDownload;
typedef struct _Heddoko__FirmwareUpdate Heddoko__FirmwareUpdate;
typedef struct _Heddoko__Packet Heddoko__Packet;


/* --- enums --- */

typedef enum _Heddoko__PacketType {
  HEDDOKO__PACKET_TYPE__StatusRequest = 0,
  HEDDOKO__PACKET_TYPE__StatusResponse = 1,
  HEDDOKO__PACKET_TYPE__SetWifiConfiguration = 2,
  HEDDOKO__PACKET_TYPE__RecordingsListRequest = 3,
  HEDDOKO__PACKET_TYPE__RecordingListReponse = 4,
  HEDDOKO__PACKET_TYPE__FileDownloadRequest = 5,
  HEDDOKO__PACKET_TYPE__FileDownloadReponse = 6,
  HEDDOKO__PACKET_TYPE__ClearBrainpackRequest = 7,
  HEDDOKO__PACKET_TYPE__CalibrationRequest = 8,
  HEDDOKO__PACKET_TYPE__CalibrationResponse = 9,
  HEDDOKO__PACKET_TYPE__StartDataStream = 10,
  HEDDOKO__PACKET_TYPE__StopDataStream = 11,
  HEDDOKO__PACKET_TYPE__ConfigureRecordingSettings = 12,
  HEDDOKO__PACKET_TYPE__DataFrame = 13,
  HEDDOKO__PACKET_TYPE__RecordingInformation = 14,
  HEDDOKO__PACKET_TYPE__LastCalibrationRequest = 15,
  HEDDOKO__PACKET_TYPE__LastCalibrationResponse = 16,
  HEDDOKO__PACKET_TYPE__AdvertisingPacket = 17,
  HEDDOKO__PACKET_TYPE__UpdateFirmwareRequest = 18,
  HEDDOKO__PACKET_TYPE__UpdatedFirmwareResponse = 19,
  HEDDOKO__PACKET_TYPE__SetTimeRequest = 20,
  HEDDOKO__PACKET_TYPE__MessageStatus = 21
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(HEDDOKO__PACKET_TYPE)
} Heddoko__PacketType;
typedef enum _Heddoko__BrainpackState {
  HEDDOKO__BRAINPACK_STATE__Initializing = 0,
  HEDDOKO__BRAINPACK_STATE__Idle = 1,
  HEDDOKO__BRAINPACK_STATE__Recording = 2,
  HEDDOKO__BRAINPACK_STATE__Streaming = 3,
  HEDDOKO__BRAINPACK_STATE__Error = 4
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(HEDDOKO__BRAINPACK_STATE)
} Heddoko__BrainpackState;
typedef enum _Heddoko__ChargeState {
  HEDDOKO__CHARGE_STATE__BatteryLow = 0,
  HEDDOKO__CHARGE_STATE__BatteryNominal = 1,
  HEDDOKO__CHARGE_STATE__BatteryFull = 2,
  HEDDOKO__CHARGE_STATE__Charging = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(HEDDOKO__CHARGE_STATE)
} Heddoko__ChargeState;
typedef enum _Heddoko__ReportType {
  HEDDOKO__REPORT_TYPE__pain = 0,
  HEDDOKO__REPORT_TYPE__concern = 1
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(HEDDOKO__REPORT_TYPE)
} Heddoko__ReportType;
typedef enum _Heddoko__WifiSecurityType {
  HEDDOKO__WIFI_SECURITY_TYPE__WEP = 0,
  HEDDOKO__WIFI_SECURITY_TYPE__WPA = 1,
  HEDDOKO__WIFI_SECURITY_TYPE__Open = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(HEDDOKO__WIFI_SECURITY_TYPE)
} Heddoko__WifiSecurityType;

/* --- messages --- */

struct  _Heddoko__ImuDataFrame
{
  ProtobufCMessage base;
  uint32_t imuid;
  protobuf_c_boolean has_sensormask;
  uint32_t sensormask;
  protobuf_c_boolean has_quat_x_yaw;
  float quat_x_yaw;
  protobuf_c_boolean has_quat_y_pitch;
  float quat_y_pitch;
  protobuf_c_boolean has_quat_z_roll;
  float quat_z_roll;
  protobuf_c_boolean has_quat_w;
  float quat_w;
  protobuf_c_boolean has_mag_x;
  int32_t mag_x;
  protobuf_c_boolean has_mag_y;
  int32_t mag_y;
  protobuf_c_boolean has_mag_z;
  int32_t mag_z;
  protobuf_c_boolean has_accel_x;
  int32_t accel_x;
  protobuf_c_boolean has_accel_y;
  int32_t accel_y;
  protobuf_c_boolean has_accel_z;
  int32_t accel_z;
  protobuf_c_boolean has_rot_x;
  int32_t rot_x;
  protobuf_c_boolean has_rot_y;
  int32_t rot_y;
  protobuf_c_boolean has_rot_z;
  int32_t rot_z;
};
#define HEDDOKO__IMU_DATA_FRAME__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&heddoko__imu_data_frame__descriptor) \
    , 0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0 }


struct  _Heddoko__FullDataFrame
{
  ProtobufCMessage base;
  uint32_t timestamp;
  size_t n_imudataframe;
  Heddoko__ImuDataFrame **imudataframe;
  protobuf_c_boolean has_reporttype;
  Heddoko__ReportType reporttype;
  char *gpscoordinates;
  protobuf_c_boolean has_calibrationid;
  uint32_t calibrationid;
};
#define HEDDOKO__FULL_DATA_FRAME__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&heddoko__full_data_frame__descriptor) \
    , 0, 0,NULL, 0,0, NULL, 0,0 }


struct  _Heddoko__Endpoint
{
  ProtobufCMessage base;
  char *address;
  uint32_t port;
};
#define HEDDOKO__ENDPOINT__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&heddoko__endpoint__descriptor) \
    , NULL, 0 }


struct  _Heddoko__WifiConfiguration
{
  ProtobufCMessage base;
  char *ssid;
  Heddoko__WifiSecurityType securitytype;
  protobuf_c_boolean wifistate;
  char *passphrase;
  protobuf_c_boolean has_udpbroadcastport;
  uint32_t udpbroadcastport;
  protobuf_c_boolean has_connectiontimeout;
  uint32_t connectiontimeout;
};
#define HEDDOKO__WIFI_CONFIGURATION__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&heddoko__wifi_configuration__descriptor) \
    , NULL, 0, 0, NULL, 0,0, 0,0 }


struct  _Heddoko__RecordingsResponse
{
  ProtobufCMessage base;
  uint32_t recordingscount;
  uint32_t calibrationcount;
  size_t n_recordingfilename;
  char **recordingfilename;
  size_t n_calibrationfilename;
  char **calibrationfilename;
};
#define HEDDOKO__RECORDINGS_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&heddoko__recordings_response__descriptor) \
    , 0, 0, 0,NULL, 0,NULL }


struct  _Heddoko__FileDownload
{
  ProtobufCMessage base;
  Heddoko__Endpoint *downloadendpoint;
  char *downloadfilename;
};
#define HEDDOKO__FILE_DOWNLOAD__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&heddoko__file_download__descriptor) \
    , NULL, NULL }


struct  _Heddoko__FirmwareUpdate
{
  ProtobufCMessage base;
  Heddoko__Endpoint *fwendpoint;
  char *fwfilename;
};
#define HEDDOKO__FIRMWARE_UPDATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&heddoko__firmware_update__descriptor) \
    , NULL, NULL }


struct  _Heddoko__Packet
{
  ProtobufCMessage base;
  Heddoko__PacketType type;
  protobuf_c_boolean has_brainpackstate;
  Heddoko__BrainpackState brainpackstate;
  char *firmwareversion;
  char *serialnumber;
  protobuf_c_boolean has_batterylevel;
  uint32_t batterylevel;
  protobuf_c_boolean has_chargestate;
  Heddoko__ChargeState chargestate;
  protobuf_c_boolean has_messagestatus;
  protobuf_c_boolean messagestatus;
  char *calibrationfilename;
  protobuf_c_boolean has_recordingrate;
  uint32_t recordingrate;
  char *recordingfilename;
  char *datetime;
  protobuf_c_boolean has_configurationport;
  uint32_t configurationport;
  protobuf_c_boolean has_sensormask;
  uint32_t sensormask;
  Heddoko__WifiConfiguration *wificonfiguration;
  Heddoko__RecordingsResponse *recordingsresponse;
  Heddoko__FileDownload *filedownload;
  Heddoko__FirmwareUpdate *firmwareupdate;
  Heddoko__FullDataFrame *fulldataframe;
  Heddoko__Endpoint *endpoint;
};
#define HEDDOKO__PACKET__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&heddoko__packet__descriptor) \
    , 0, 0,0, NULL, NULL, 0,0, 0,0, 0,0, NULL, 0,0, NULL, NULL, 0,0, 0,0, NULL, NULL, NULL, NULL, NULL, NULL }


/* Heddoko__ImuDataFrame methods */
void   heddoko__imu_data_frame__init
                     (Heddoko__ImuDataFrame         *message);
size_t heddoko__imu_data_frame__get_packed_size
                     (const Heddoko__ImuDataFrame   *message);
size_t heddoko__imu_data_frame__pack
                     (const Heddoko__ImuDataFrame   *message,
                      uint8_t             *out);
size_t heddoko__imu_data_frame__pack_to_buffer
                     (const Heddoko__ImuDataFrame   *message,
                      ProtobufCBuffer     *buffer);
Heddoko__ImuDataFrame *
       heddoko__imu_data_frame__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   heddoko__imu_data_frame__free_unpacked
                     (Heddoko__ImuDataFrame *message,
                      ProtobufCAllocator *allocator);
/* Heddoko__FullDataFrame methods */
void   heddoko__full_data_frame__init
                     (Heddoko__FullDataFrame         *message);
size_t heddoko__full_data_frame__get_packed_size
                     (const Heddoko__FullDataFrame   *message);
size_t heddoko__full_data_frame__pack
                     (const Heddoko__FullDataFrame   *message,
                      uint8_t             *out);
size_t heddoko__full_data_frame__pack_to_buffer
                     (const Heddoko__FullDataFrame   *message,
                      ProtobufCBuffer     *buffer);
Heddoko__FullDataFrame *
       heddoko__full_data_frame__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   heddoko__full_data_frame__free_unpacked
                     (Heddoko__FullDataFrame *message,
                      ProtobufCAllocator *allocator);
/* Heddoko__Endpoint methods */
void   heddoko__endpoint__init
                     (Heddoko__Endpoint         *message);
size_t heddoko__endpoint__get_packed_size
                     (const Heddoko__Endpoint   *message);
size_t heddoko__endpoint__pack
                     (const Heddoko__Endpoint   *message,
                      uint8_t             *out);
size_t heddoko__endpoint__pack_to_buffer
                     (const Heddoko__Endpoint   *message,
                      ProtobufCBuffer     *buffer);
Heddoko__Endpoint *
       heddoko__endpoint__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   heddoko__endpoint__free_unpacked
                     (Heddoko__Endpoint *message,
                      ProtobufCAllocator *allocator);
/* Heddoko__WifiConfiguration methods */
void   heddoko__wifi_configuration__init
                     (Heddoko__WifiConfiguration         *message);
size_t heddoko__wifi_configuration__get_packed_size
                     (const Heddoko__WifiConfiguration   *message);
size_t heddoko__wifi_configuration__pack
                     (const Heddoko__WifiConfiguration   *message,
                      uint8_t             *out);
size_t heddoko__wifi_configuration__pack_to_buffer
                     (const Heddoko__WifiConfiguration   *message,
                      ProtobufCBuffer     *buffer);
Heddoko__WifiConfiguration *
       heddoko__wifi_configuration__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   heddoko__wifi_configuration__free_unpacked
                     (Heddoko__WifiConfiguration *message,
                      ProtobufCAllocator *allocator);
/* Heddoko__RecordingsResponse methods */
void   heddoko__recordings_response__init
                     (Heddoko__RecordingsResponse         *message);
size_t heddoko__recordings_response__get_packed_size
                     (const Heddoko__RecordingsResponse   *message);
size_t heddoko__recordings_response__pack
                     (const Heddoko__RecordingsResponse   *message,
                      uint8_t             *out);
size_t heddoko__recordings_response__pack_to_buffer
                     (const Heddoko__RecordingsResponse   *message,
                      ProtobufCBuffer     *buffer);
Heddoko__RecordingsResponse *
       heddoko__recordings_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   heddoko__recordings_response__free_unpacked
                     (Heddoko__RecordingsResponse *message,
                      ProtobufCAllocator *allocator);
/* Heddoko__FileDownload methods */
void   heddoko__file_download__init
                     (Heddoko__FileDownload         *message);
size_t heddoko__file_download__get_packed_size
                     (const Heddoko__FileDownload   *message);
size_t heddoko__file_download__pack
                     (const Heddoko__FileDownload   *message,
                      uint8_t             *out);
size_t heddoko__file_download__pack_to_buffer
                     (const Heddoko__FileDownload   *message,
                      ProtobufCBuffer     *buffer);
Heddoko__FileDownload *
       heddoko__file_download__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   heddoko__file_download__free_unpacked
                     (Heddoko__FileDownload *message,
                      ProtobufCAllocator *allocator);
/* Heddoko__FirmwareUpdate methods */
void   heddoko__firmware_update__init
                     (Heddoko__FirmwareUpdate         *message);
size_t heddoko__firmware_update__get_packed_size
                     (const Heddoko__FirmwareUpdate   *message);
size_t heddoko__firmware_update__pack
                     (const Heddoko__FirmwareUpdate   *message,
                      uint8_t             *out);
size_t heddoko__firmware_update__pack_to_buffer
                     (const Heddoko__FirmwareUpdate   *message,
                      ProtobufCBuffer     *buffer);
Heddoko__FirmwareUpdate *
       heddoko__firmware_update__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   heddoko__firmware_update__free_unpacked
                     (Heddoko__FirmwareUpdate *message,
                      ProtobufCAllocator *allocator);
/* Heddoko__Packet methods */
void   heddoko__packet__init
                     (Heddoko__Packet         *message);
size_t heddoko__packet__get_packed_size
                     (const Heddoko__Packet   *message);
size_t heddoko__packet__pack
                     (const Heddoko__Packet   *message,
                      uint8_t             *out);
size_t heddoko__packet__pack_to_buffer
                     (const Heddoko__Packet   *message,
                      ProtobufCBuffer     *buffer);
Heddoko__Packet *
       heddoko__packet__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   heddoko__packet__free_unpacked
                     (Heddoko__Packet *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Heddoko__ImuDataFrame_Closure)
                 (const Heddoko__ImuDataFrame *message,
                  void *closure_data);
typedef void (*Heddoko__FullDataFrame_Closure)
                 (const Heddoko__FullDataFrame *message,
                  void *closure_data);
typedef void (*Heddoko__Endpoint_Closure)
                 (const Heddoko__Endpoint *message,
                  void *closure_data);
typedef void (*Heddoko__WifiConfiguration_Closure)
                 (const Heddoko__WifiConfiguration *message,
                  void *closure_data);
typedef void (*Heddoko__RecordingsResponse_Closure)
                 (const Heddoko__RecordingsResponse *message,
                  void *closure_data);
typedef void (*Heddoko__FileDownload_Closure)
                 (const Heddoko__FileDownload *message,
                  void *closure_data);
typedef void (*Heddoko__FirmwareUpdate_Closure)
                 (const Heddoko__FirmwareUpdate *message,
                  void *closure_data);
typedef void (*Heddoko__Packet_Closure)
                 (const Heddoko__Packet *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCEnumDescriptor    heddoko__packet_type__descriptor;
extern const ProtobufCEnumDescriptor    heddoko__brainpack_state__descriptor;
extern const ProtobufCEnumDescriptor    heddoko__charge_state__descriptor;
extern const ProtobufCEnumDescriptor    heddoko__report_type__descriptor;
extern const ProtobufCEnumDescriptor    heddoko__wifi_security_type__descriptor;
extern const ProtobufCMessageDescriptor heddoko__imu_data_frame__descriptor;
extern const ProtobufCMessageDescriptor heddoko__full_data_frame__descriptor;
extern const ProtobufCMessageDescriptor heddoko__endpoint__descriptor;
extern const ProtobufCMessageDescriptor heddoko__wifi_configuration__descriptor;
extern const ProtobufCMessageDescriptor heddoko__recordings_response__descriptor;
extern const ProtobufCMessageDescriptor heddoko__file_download__descriptor;
extern const ProtobufCMessageDescriptor heddoko__firmware_update__descriptor;
extern const ProtobufCMessageDescriptor heddoko__packet__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_heddokoPacket_2eproto__INCLUDED */
