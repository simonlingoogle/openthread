/*
 *    Copyright 2016 Nest Labs Inc. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: ncp.proto */

#ifndef PROTOBUF_C_ncp_2eproto__INCLUDED
#define PROTOBUF_C_ncp_2eproto__INCLUDED

#include <protobuf/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1001001 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif

#include "static-c.pb-c.h"

typedef struct _ThreadPrimitive ThreadPrimitive;
typedef struct _ThreadState ThreadState;
typedef struct _ThreadWhitelist ThreadWhitelist;
typedef struct _ThreadIp6Addresses ThreadIp6Addresses;
typedef struct _ThreadScanRequest ThreadScanRequest;
typedef struct _ThreadScanResult ThreadScanResult;
typedef struct _ThreadScanResultDone ThreadScanResultDone;
typedef struct _ThreadControl ThreadControl;


/* --- enums --- */

typedef enum _ThreadPrimitive__Type {
  THREAD_PRIMITIVE__TYPE__THREAD_KEY = 1,
  THREAD_PRIMITIVE__TYPE__THREAD_KEY_SEQUENCE = 2,
  THREAD_PRIMITIVE__TYPE__THREAD_MESH_LOCAL_PREFIX = 3,
  THREAD_PRIMITIVE__TYPE__THREAD_MODE = 4,
  THREAD_PRIMITIVE__TYPE__THREAD_STATUS = 5,
  THREAD_PRIMITIVE__TYPE__THREAD_TIMEOUT = 6,
  THREAD_PRIMITIVE__TYPE__IEEE802154_CHANNEL = 7,
  THREAD_PRIMITIVE__TYPE__IEEE802154_PANID = 8,
  THREAD_PRIMITIVE__TYPE__IEEE802154_EXTENDED_PANID = 9,
  THREAD_PRIMITIVE__TYPE__IEEE802154_NETWORK_NAME = 10,
  THREAD_PRIMITIVE__TYPE__IEEE802154_SHORT_ADDR = 11,
  THREAD_PRIMITIVE__TYPE__IEEE802154_EXT_ADDR = 12
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(THREAD_PRIMITIVE__TYPE)
} ThreadPrimitive__Type;
typedef enum _ThreadState__State {
  THREAD_STATE__STATE__DETACHED = 0,
  THREAD_STATE__STATE__CHILD = 1,
  THREAD_STATE__STATE__ROUTER = 2,
  THREAD_STATE__STATE__LEADER = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(THREAD_STATE__STATE)
} ThreadState__State;
typedef enum _ThreadWhitelist__Type {
  THREAD_WHITELIST__TYPE__STATUS = 0,
  THREAD_WHITELIST__TYPE__LIST = 1,
  THREAD_WHITELIST__TYPE__ADD = 2,
  THREAD_WHITELIST__TYPE__DELETE = 3,
  THREAD_WHITELIST__TYPE__CLEAR = 4
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(THREAD_WHITELIST__TYPE)
} ThreadWhitelist__Type;
typedef enum _ThreadWhitelist__Status {
  THREAD_WHITELIST__STATUS__DISABLE = 0,
  THREAD_WHITELIST__STATUS__ENABLE = 1
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(THREAD_WHITELIST__STATUS)
} ThreadWhitelist__Status;

/* --- messages --- */

typedef enum {
  THREAD_PRIMITIVE__VALUE__NOT_SET = 0,
  THREAD_PRIMITIVE__VALUE_BOOL = 2,
  THREAD_PRIMITIVE__VALUE_UINT32 = 3,
  THREAD_PRIMITIVE__VALUE_BYTES = 4,
} ThreadPrimitive__ValueCase;

struct  _ThreadPrimitive
{
  ProtobufCMessage base;
  ThreadPrimitive__Type type;
  ThreadPrimitive__ValueCase value_case;
  union {
    protobuf_c_boolean bool_;
    uint32_t uint32;
    struct {
      size_t len;
      uint8_t data[16];
    } bytes;
  };
};
#define THREAD_PRIMITIVE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&thread_primitive__descriptor) \
    , 0, THREAD_PRIMITIVE__VALUE__NOT_SET, {} }


struct  _ThreadState
{
  ProtobufCMessage base;
  protobuf_c_boolean has_state;
  ThreadState__State state;
};
#define THREAD_STATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&thread_state__descriptor) \
    , 0,0 }


struct  _ThreadWhitelist
{
  ProtobufCMessage base;
  ThreadWhitelist__Type type;
  protobuf_c_boolean has_status;
  ThreadWhitelist__Status status;
  size_t n_address;
  struct {
    size_t len;
    uint8_t data[8];
  } address[8];
};
#define THREAD_WHITELIST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&thread_whitelist__descriptor) \
    , 0, 0,0, 0,{{0, {0}}} }


struct  _ThreadIp6Addresses
{
  ProtobufCMessage base;
  size_t n_address;
  struct {
    size_t len;
    uint8_t data[16];
  } address[4];
};
#define THREAD_IP6_ADDRESSES__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&thread_ip6_addresses__descriptor) \
    , 0,{{0, {0}}} }


struct  _ThreadScanRequest
{
  ProtobufCMessage base;
  protobuf_c_boolean has_channel_mask;
  uint32_t channel_mask;
  protobuf_c_boolean has_scan_interval_per_channel;
  uint32_t scan_interval_per_channel;
};
#define THREAD_SCAN_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&thread_scan_request__descriptor) \
    , 0,0, 0,0 }


struct  _ThreadScanResult
{
  ProtobufCMessage base;
  struct {
    size_t len;
    uint8_t data[16];
  } network_name;
  struct {
    size_t len;
    uint8_t data[8];
  } ext_panid;
  struct {
    size_t len;
    uint8_t data[8];
  } ext_addr;
  uint32_t panid;
  uint32_t channel;
  int32_t rssi;
};
#define THREAD_SCAN_RESULT__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&thread_scan_result__descriptor) \
	 , {0,{0}}, {0,{0}}, {0,{0}}, 0, 0, 0 }


struct  _ThreadScanResultDone
{
  ProtobufCMessage base;
};
#define THREAD_SCAN_RESULT_DONE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&thread_scan_result_done__descriptor) \
     }


typedef enum {
  THREAD_CONTROL__MESSAGE__NOT_SET = 0,
  THREAD_CONTROL__MESSAGE_PRIMITIVE = 1,
  THREAD_CONTROL__MESSAGE_STATE = 2,
  THREAD_CONTROL__MESSAGE_ADDRESSES = 3,
  THREAD_CONTROL__MESSAGE_WHITELIST = 4,
  THREAD_CONTROL__MESSAGE_SCAN_REQUEST = 5,
  THREAD_CONTROL__MESSAGE_SCAN_RESULT = 6,
  THREAD_CONTROL__MESSAGE_SCAN_RESULT_DONE = 7,
} ThreadControl__MessageCase;

struct  _ThreadControl
{
  ProtobufCMessage base;
  ThreadControl__MessageCase message_case;
  union {
    ThreadPrimitive primitive;
    ThreadState state;
    ThreadIp6Addresses addresses;
    ThreadWhitelist whitelist;
    ThreadScanRequest scan_request;
    ThreadScanResult scan_result;
    ThreadScanResultDone scan_result_done;
  };
};
#define THREAD_CONTROL__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&thread_control__descriptor) \
    , THREAD_CONTROL__MESSAGE__NOT_SET, {} }


/* ThreadPrimitive methods */
void   thread_primitive__init
                     (ThreadPrimitive         *message);
size_t thread_primitive__pack
                     (const ThreadPrimitive   *message,
                      uint8_t             *out);
ThreadPrimitive *
       thread_primitive__unpack
                     (size_t               len,
                      const uint8_t       *data,
                      ThreadPrimitive         *message);
/* ThreadState methods */
void   thread_state__init
                     (ThreadState         *message);
size_t thread_state__pack
                     (const ThreadState   *message,
                      uint8_t             *out);
ThreadState *
       thread_state__unpack
                     (size_t               len,
                      const uint8_t       *data,
                      ThreadState         *message);
/* ThreadWhitelist methods */
void   thread_whitelist__init
                     (ThreadWhitelist         *message);
size_t thread_whitelist__pack
                     (const ThreadWhitelist   *message,
                      uint8_t             *out);
ThreadWhitelist *
       thread_whitelist__unpack
                     (size_t               len,
                      const uint8_t       *data,
                      ThreadWhitelist         *message);
/* ThreadIp6Addresses methods */
void   thread_ip6_addresses__init
                     (ThreadIp6Addresses         *message);
size_t thread_ip6_addresses__pack
                     (const ThreadIp6Addresses   *message,
                      uint8_t             *out);
ThreadIp6Addresses *
       thread_ip6_addresses__unpack
                     (size_t               len,
                      const uint8_t       *data,
                      ThreadIp6Addresses         *message);
/* ThreadScanRequest methods */
void   thread_scan_request__init
                     (ThreadScanRequest         *message);
size_t thread_scan_request__pack
                     (const ThreadScanRequest   *message,
                      uint8_t             *out);
ThreadScanRequest *
       thread_scan_request__unpack
                     (size_t               len,
                      const uint8_t       *data,
                      ThreadScanRequest         *message);
/* ThreadScanResult methods */
void   thread_scan_result__init
                     (ThreadScanResult         *message);
size_t thread_scan_result__pack
                     (const ThreadScanResult   *message,
                      uint8_t             *out);
ThreadScanResult *
       thread_scan_result__unpack
                     (size_t               len,
                      const uint8_t       *data,
                      ThreadScanResult         *message);
/* ThreadScanResultDone methods */
void   thread_scan_result_done__init
                     (ThreadScanResultDone         *message);
size_t thread_scan_result_done__pack
                     (const ThreadScanResultDone   *message,
                      uint8_t             *out);
ThreadScanResultDone *
       thread_scan_result_done__unpack
                     (size_t               len,
                      const uint8_t       *data,
                      ThreadScanResultDone         *message);
/* ThreadControl methods */
void   thread_control__init
                     (ThreadControl         *message);
size_t thread_control__pack
                     (const ThreadControl   *message,
                      uint8_t             *out);
ThreadControl *
       thread_control__unpack
                     (size_t               len,
                      const uint8_t       *data,
                      ThreadControl         *message);
/* --- per-message closures --- */

typedef void (*ThreadPrimitive_Closure)
                 (const ThreadPrimitive *message,
                  void *closure_data);
typedef void (*ThreadState_Closure)
                 (const ThreadState *message,
                  void *closure_data);
typedef void (*ThreadWhitelist_Closure)
                 (const ThreadWhitelist *message,
                  void *closure_data);
typedef void (*ThreadIp6Addresses_Closure)
                 (const ThreadIp6Addresses *message,
                  void *closure_data);
typedef void (*ThreadScanRequest_Closure)
                 (const ThreadScanRequest *message,
                  void *closure_data);
typedef void (*ThreadScanResult_Closure)
                 (const ThreadScanResult *message,
                  void *closure_data);
typedef void (*ThreadScanResultDone_Closure)
                 (const ThreadScanResultDone *message,
                  void *closure_data);
typedef void (*ThreadControl_Closure)
                 (const ThreadControl *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor thread_primitive__descriptor;
extern const ProtobufCEnumDescriptor    thread_primitive__type__descriptor;
extern const ProtobufCMessageDescriptor thread_state__descriptor;
extern const ProtobufCEnumDescriptor    thread_state__state__descriptor;
extern const ProtobufCMessageDescriptor thread_whitelist__descriptor;
extern const ProtobufCEnumDescriptor    thread_whitelist__type__descriptor;
extern const ProtobufCEnumDescriptor    thread_whitelist__status__descriptor;
extern const ProtobufCMessageDescriptor thread_ip6_addresses__descriptor;
extern const ProtobufCMessageDescriptor thread_scan_request__descriptor;
extern const ProtobufCMessageDescriptor thread_scan_result__descriptor;
extern const ProtobufCMessageDescriptor thread_scan_result_done__descriptor;
extern const ProtobufCMessageDescriptor thread_control__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_ncp_2eproto__INCLUDED */
