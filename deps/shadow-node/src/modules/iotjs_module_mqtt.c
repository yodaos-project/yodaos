#include "iotjs_def.h"
#include "iotjs_module_buffer.h"
#include "iotjs_objectwrap.h"
#include <MQTTPacket/MQTTPacket.h>
#include <stdlib.h>

enum QoS { QOS0, QOS1, QOS2, SUBFAIL = 0x80 };

typedef struct {
  iotjs_jobjectwrap_t jobjectwrap;
  unsigned char* src;
  int srclen;
  int offset;
  MQTTPacket_connectData options_;
} IOTJS_VALIDATED_STRUCT(iotjs_mqtt_t);

static iotjs_mqtt_t* iotjs_mqtt_create(const jerry_value_t value);
static void iotjs_mqtt_destroy(iotjs_mqtt_t* mqtt);
static JNativeInfoType this_module_native_info = {
  .free_cb = (jerry_object_native_free_callback_t)iotjs_mqtt_destroy
};

iotjs_mqtt_t* iotjs_mqtt_create(const jerry_value_t value) {
  iotjs_mqtt_t* mqtt = IOTJS_ALLOC(iotjs_mqtt_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_mqtt_t, mqtt);
  iotjs_jobjectwrap_initialize(&_this->jobjectwrap, value,
                               &this_module_native_info);
  return mqtt;
}

void iotjs_mqtt_destroy(iotjs_mqtt_t* mqtt) {
  IOTJS_VALIDATED_STRUCT_DESTRUCTOR(iotjs_mqtt_t, mqtt);

#define IOTJS_MQTT_RELEASE_OPTION(name)                 \
  if (_this->options_.name.lenstring.data != NULL) {    \
    IOTJS_RELEASE(_this->options_.name.lenstring.data); \
  }
  IOTJS_MQTT_RELEASE_OPTION(username);
  IOTJS_MQTT_RELEASE_OPTION(password);
  IOTJS_MQTT_RELEASE_OPTION(clientID);
  IOTJS_MQTT_RELEASE_OPTION(will.topicName);
  IOTJS_MQTT_RELEASE_OPTION(will.message);
#undef IOTJS_MQTT_RELEASE_OPTION

  iotjs_jobjectwrap_destroy(&_this->jobjectwrap);
  IOTJS_RELEASE(mqtt);
}

static void iotjs_mqtt_alloc_buf(unsigned char** buf, int expected_size,
                                 int* alloc_size) {
  static int buf_size = 128 * 1024;
  static unsigned char* buf_ = NULL;
  if (buf_ == NULL) {
    buf_ = malloc((size_t)buf_size * sizeof(unsigned char));
    if (buf_ == NULL) {
      return;
    }
  }
  if (expected_size > buf_size) {
    return;
  }
  *buf = buf_;
  *alloc_size = buf_size;
}

JS_FUNCTION(MqttConstructor) {
  DJS_CHECK_THIS();

  const jerry_value_t self = JS_GET_THIS();
  jerry_value_t ret = jerry_create_undefined();
  iotjs_mqtt_t* mqtt = iotjs_mqtt_create(self);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_mqtt_t, mqtt);

  jerry_value_t opts = JS_GET_ARG(0, object);
  jerry_value_t version =
      iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_PROTOCOL_VERSION);
  jerry_value_t keepalive =
      iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_KEEPALIVE);
  jerry_value_t username =
      iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_USERNAME);
  jerry_value_t password =
      iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_PASSWORD);
  jerry_value_t clientID =
      iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_CLIENT_ID);
  jerry_value_t will = iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_WILL);

  MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
  options.willFlag = 0;
  options.MQTTVersion = (int)iotjs_jval_as_number(version);
  options.keepAliveInterval = (int)iotjs_jval_as_number(keepalive);
  options.cleansession = 1;

#define MQTT_OPTION_ASSIGN_FROM(owner, name)                           \
  do {                                                                 \
    if (jerry_value_is_string(name)) {                                 \
      iotjs_string_t str = iotjs_jval_as_string(name);                 \
      size_t size = iotjs_string_size(&str);                           \
      MQTTString mqttstr = MQTTString_initializer;                     \
      mqttstr.lenstring.data = strdup((char*)iotjs_string_data(&str)); \
      mqttstr.lenstring.len = (int)size;                               \
      owner.name = mqttstr;                                            \
      iotjs_string_destroy(&str);                                      \
    }                                                                  \
  } while (0)

  MQTT_OPTION_ASSIGN_FROM(options, username);
  MQTT_OPTION_ASSIGN_FROM(options, password);
  MQTT_OPTION_ASSIGN_FROM(options, clientID);

  if (jerry_value_is_object(will)) {
    MQTTPacket_willOptions willOpts = MQTTPacket_willOptions_initializer;
    jerry_value_t topicName =
        iotjs_jval_get_property(will, IOTJS_MAGIC_STRING_TOPIC);
    jerry_value_t message =
        iotjs_jval_get_property(will, IOTJS_MAGIC_STRING_PAYLOAD);
    jerry_value_t retained =
        iotjs_jval_get_property(will, IOTJS_MAGIC_STRING_RETAIN);
    jerry_value_t qos = iotjs_jval_get_property(will, IOTJS_MAGIC_STRING_QOS);
    willOpts.retained = iotjs_jval_as_boolean(retained);
    willOpts.qos = iotjs_jval_as_number(qos);

    MQTT_OPTION_ASSIGN_FROM(willOpts, topicName);

    // handle the `willOpts.message` that accepts a Buffer.
    do {
      iotjs_bufferwrap_t* buffer = iotjs_bufferwrap_from_jbuffer(message);
      size_t buf_len = iotjs_bufferwrap_length(buffer);
      char* buf_str = strndup((char*)iotjs_bufferwrap_buffer(buffer), buf_len);
      MQTTString message_str = MQTTString_initializer;
      message_str.lenstring.data = buf_str;
      message_str.lenstring.len = (int)buf_len;
      willOpts.message = message_str;
      // set will opts to will.
      options.will = willOpts;
      options.willFlag = 1;
    } while (0);

    jerry_release_value(topicName);
    jerry_release_value(message);
    jerry_release_value(retained);
    jerry_release_value(qos);
  }
  _this->options_ = options;

  jerry_release_value(version);
  jerry_release_value(keepalive);
  jerry_release_value(username);
  jerry_release_value(password);
  jerry_release_value(clientID);
  jerry_release_value(will);
#undef MQTT_OPTION_ASSIGN_FROM

  return ret;
}

JS_FUNCTION(MqttReadPacket) {
  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(jargv[0]);
  unsigned char* buf = (unsigned char*)iotjs_bufferwrap_buffer(wrap);
  int buf_size = (int)iotjs_bufferwrap_length(wrap);

  MQTTHeader header = { 0 };
  header.byte = buf[0];
  int type = header.bits.type;

  jerry_value_t ret;
  do {
    if (type == CONNACK) {
      unsigned char present;
      unsigned char code;
      int r = MQTTDeserialize_connack(&present, &code, buf, buf_size);
      if (r != 1) {
        ret = JS_CREATE_ERROR(COMMON, "CONNACK Decode Failed");
        break;
      }
      if (code == MQTT_UNNACCEPTABLE_PROTOCOL) {
        ret = JS_CREATE_ERROR(COMMON, "Unacceptable Protocol Version");
        break;
      }
      if (code == MQTT_CLIENTID_REJECTED) {
        ret = JS_CREATE_ERROR(COMMON, "Identifier Rejected");
        break;
      }
      if (code == MQTT_SERVER_UNAVAILABLE) {
        ret = JS_CREATE_ERROR(COMMON, "Server Unavailable");
        break;
      }
      if (code == MQTT_BAD_USERNAME_OR_PASSWORD) {
        ret = JS_CREATE_ERROR(COMMON, "Bad User Name Or Password");
        break;
      }
      if (code == MQTT_NOT_AUTHORIZED) {
        ret = JS_CREATE_ERROR(COMMON, "Not Authorized");
        break;
      }
      if (code != MQTT_CONNECTION_ACCEPTED) {
        char err_str[64];
        sprintf(err_str, "Unknown CONNACK Code %d", code);
        ret = JS_CREATE_ERROR(COMMON, err_str);
        break;
      }
    } else if (type == SUBACK) {
      int count = 0;
      int qos = 0;
      unsigned short msgId;
      int r = MQTTDeserialize_suback(&msgId, 1, &count, &qos, buf, buf_size);
      if (r != 1 || qos == SUBFAIL) {
        ret = JS_CREATE_ERROR(COMMON, "SUBACK failed with server.");
        break;
      }
    }
    ret = jerry_create_object();
    iotjs_jval_set_property_jval(ret, "type", jerry_create_number(type));
  } while (0);
  return ret;
}

JS_FUNCTION(MqttGetConnect) {
  JS_DECLARE_THIS_PTR(mqtt, mqtt);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_mqtt_t, mqtt);

  unsigned char* buf = NULL;
  int buf_size = 0;
  iotjs_mqtt_alloc_buf(&buf, 256, &buf_size);
  if (buf == NULL) {
    return JS_CREATE_ERROR(COMMON, "mqtt payload buf create error on connect");
  }
  int len = MQTTSerialize_connect(buf, buf_size, &_this->options_);
  if (len == MQTTPACKET_BUFFER_TOO_SHORT) {
    return JS_CREATE_ERROR(COMMON, "connection length is too short.");
  }

  jerry_value_t retbuf = iotjs_bufferwrap_create_buffer((size_t)len);
  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(retbuf);
  iotjs_bufferwrap_copy(wrap, (const char*)buf, (size_t)len);
  return retbuf;
}

JS_FUNCTION(MqttGetPublish) {
  iotjs_string_t topic = JS_GET_ARG(0, string);
  jerry_value_t opts = JS_GET_ARG(1, object);
  jerry_value_t msg_id = iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_ID);
  jerry_value_t msg_qos = iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_QOS);
  jerry_value_t msg_dup = iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_DUP);
  jerry_value_t msg_retain =
      iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_RETAIN);
  jerry_value_t msg_payload_ =
      iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_PAYLOAD);
  iotjs_bufferwrap_t* msg_payload = iotjs_bufferwrap_from_jbuffer(msg_payload_);

  MQTTString top = MQTTString_initializer;
  top.cstring = (char*)iotjs_string_data(&topic);

  jerry_value_t ret;
  do {
    int msg_size = (int)iotjs_bufferwrap_length(msg_payload);
    int buf_size = 0;
    unsigned char* buf = NULL;
    iotjs_mqtt_alloc_buf(&buf, msg_size, &buf_size);
    if (buf == NULL) {
      ret = JS_CREATE_ERROR(COMMON, "mqtt payload buf create error");
      break;
    }

    unsigned short msg_id_num = (unsigned short)iotjs_jval_as_number(msg_id);
    unsigned char* msg_payload_str =
        (unsigned char*)iotjs_bufferwrap_buffer(msg_payload);

    int len = MQTTSerialize_publish(buf, buf_size,
                                    iotjs_jval_as_boolean(msg_dup) ? 1 : 0,
                                    iotjs_jval_as_number(msg_qos),
                                    iotjs_jval_as_boolean(msg_retain) ? 1 : 0,
                                    msg_id_num, top, msg_payload_str, msg_size);
    if (len < 0) {
      ret = JS_CREATE_ERROR(COMMON, "mqtt payload is too large");
      break;
    }
    jerry_value_t retbuf = iotjs_bufferwrap_create_buffer((size_t)len);
    iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(retbuf);
    iotjs_bufferwrap_copy(wrap, (const char*)buf, (size_t)len);
    ret = retbuf;
  } while (false);

  jerry_release_value(msg_id);
  jerry_release_value(msg_qos);
  jerry_release_value(msg_dup);
  jerry_release_value(msg_retain);
  jerry_release_value(msg_payload_);
  iotjs_string_destroy(&topic);
  return ret;
}

JS_FUNCTION(MqttGetPingReq) {
  unsigned char* buf = NULL;
  int buf_size = 0;
  iotjs_mqtt_alloc_buf(&buf, 100, &buf_size);
  if (buf == NULL) {
    return JS_CREATE_ERROR(COMMON, "mqtt payload buf create error on ping req");
  }
  int len = MQTTSerialize_pingreq(buf, buf_size);
  jerry_value_t retbuf = iotjs_bufferwrap_create_buffer((size_t)len);
  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(retbuf);
  iotjs_bufferwrap_copy(wrap, (const char*)buf, (size_t)len);
  return retbuf;
}

JS_FUNCTION(MqttGetAck) {
  int msg_id = JS_GET_ARG(0, number);
  int qos = JS_GET_ARG(1, number);
  unsigned char* buf = NULL;
  int buf_size = 0;
  iotjs_mqtt_alloc_buf(&buf, 100, &buf_size);
  if (buf == NULL) {
    return JS_CREATE_ERROR(COMMON, "mqtt payload buf create error on get ack");
  }
  int len = 0;

  if (qos == QOS1) {
    len = MQTTSerialize_ack(buf, buf_size, PUBACK, 0, msg_id);
  } else if (qos == QOS2) {
    len = MQTTSerialize_ack(buf, buf_size, PUBREC, 0, msg_id);
  } else {
    return JS_CREATE_ERROR(COMMON, "invalid qos from message.");
  }

  if (len <= 0) {
    return JS_CREATE_ERROR(COMMON, "invalid serialization for ack.")
  }
  jerry_value_t retbuf = iotjs_bufferwrap_create_buffer((size_t)len);
  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(retbuf);
  iotjs_bufferwrap_copy(wrap, (const char*)buf, (size_t)len);
  return retbuf;
}

JS_FUNCTION(MqttGetSubscribe) {
  jerry_value_t opts = JS_GET_ARG(1, object);
  jerry_value_t msg_id_ = iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_ID);
  unsigned short msg_id = iotjs_jval_as_number(msg_id_);
  jerry_release_value(msg_id_);
  jerry_value_t msg_qos_ =
      iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_QOS);
  int qos = (int)iotjs_jval_as_number(msg_qos_);
  jerry_release_value(msg_qos_);

  jerry_value_t jtopics = jargv[0];
  uint32_t size = jerry_get_array_length(jtopics);

  MQTTString topics[size];
  for (uint32_t i = 0; i < size; i++) {
    jerry_value_t jtopic = iotjs_jval_get_property_by_index(jtopics, i);
    iotjs_string_t topicstr = iotjs_jval_as_string(jtopic);
    MQTTString curr = MQTTString_initializer;
    curr.cstring = (char*)strdup(iotjs_string_data(&topicstr));
    topics[i] = curr;

    jerry_release_value(jtopic);
    iotjs_string_destroy(&topicstr);
  }

  unsigned char* buf = NULL;
  int buf_size = 0;
  iotjs_mqtt_alloc_buf(&buf, 256, &buf_size);
  if (buf == NULL) {
    return JS_CREATE_ERROR(COMMON, "mqtt payload buf create error on get ack");
  }
  int len = MQTTSerialize_subscribe(buf, buf_size, 0, msg_id, (int)size, topics,
                                    (int*)&qos);
  if (len < 0) {
    return JS_CREATE_ERROR(COMMON, "mqtt subscribe topic is too large");
  }

  jerry_value_t retbuf = iotjs_bufferwrap_create_buffer((size_t)len);
  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(retbuf);
  iotjs_bufferwrap_copy(wrap, (const char*)buf, (size_t)len);

  for (uint32_t i = 0; i < size; i++) {
    if (topics[i].cstring != NULL)
      free(topics[i].cstring);
  }
  return retbuf;
}

JS_FUNCTION(MqttGetUnsubscribe) {
  jerry_value_t jtopics = jargv[0];
  uint32_t size = jerry_get_array_length(jtopics);
  jerry_value_t opts = JS_GET_ARG(1, object);
  jerry_value_t msg_id_ = iotjs_jval_get_property(opts, IOTJS_MAGIC_STRING_ID);
  unsigned short msg_id = iotjs_jval_as_number(msg_id_);
  jerry_release_value(msg_id_);

  MQTTString topics[size];
  for (uint32_t i = 0; i < size; i++) {
    jerry_value_t jtopic = iotjs_jval_get_property_by_index(jtopics, i);
    iotjs_string_t topicstr = iotjs_jval_as_string(jtopic);
    MQTTString curr = MQTTString_initializer;
    curr.cstring = (char*)strdup(iotjs_string_data(&topicstr));
    topics[i] = curr;

    jerry_release_value(jtopic);
    iotjs_string_destroy(&topicstr);
  }

  unsigned char* buf = NULL;
  int buf_size = 0;
  iotjs_mqtt_alloc_buf(&buf, 256, &buf_size);
  if (buf == NULL) {
    return JS_CREATE_ERROR(COMMON, "mqtt payload buf create error on get ack");
  }
  int len =
      MQTTSerialize_unsubscribe(buf, buf_size, 0, msg_id, (int)size, topics);

  if (len < 0) {
    return JS_CREATE_ERROR(COMMON, "mqtt unsubscribe topic is too large");
  }
  jerry_value_t retbuf = iotjs_bufferwrap_create_buffer((size_t)len);
  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(retbuf);
  iotjs_bufferwrap_copy(wrap, (const char*)buf, (size_t)len);

  for (uint32_t i = 0; i < size; i++) {
    if (topics[i].cstring != NULL)
      free(topics[i].cstring);
  }
  return retbuf;
}

JS_FUNCTION(MqttGetDisconnect) {
  unsigned char* buf = NULL;
  int buf_size = 0;
  iotjs_mqtt_alloc_buf(&buf, 100, &buf_size);
  if (buf == NULL) {
    return JS_CREATE_ERROR(COMMON, "mqtt payload buf create error on get ack");
  }
  int len = MQTTSerialize_disconnect(buf, buf_size);
  if (len < 0) {
    return JS_CREATE_ERROR(COMMON, "mqtt get disconnect is too large");
  }
  jerry_value_t retbuf = iotjs_bufferwrap_create_buffer((size_t)len);
  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(retbuf);
  iotjs_bufferwrap_copy(wrap, (const char*)buf, (size_t)len);
  return retbuf;
}

JS_FUNCTION(MqttDeserialize) {
  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(jargv[0]);
  unsigned short msgId = 0;
  unsigned char dup = 0;
  unsigned char retain = 0;
  int qos = 0;

  MQTTString topic;
  unsigned char* payload;
  int payload_size;
  unsigned char* buf = (unsigned char*)iotjs_bufferwrap_buffer(wrap);
  int buf_size = (int)iotjs_bufferwrap_length(wrap);

  int r = MQTTDeserialize_publish(&dup, &qos, &retain, &msgId, &topic, &payload,
                                  &payload_size, buf, buf_size);
  if (r != 1) {
    return JS_CREATE_ERROR(COMMON, "failed to deserialize publish message.");
  }
  int header_size = (int)(payload - buf);
  int payload_real_size = buf_size - header_size;
  int payload_missing_size = payload_size - payload_real_size;

  jerry_value_t msg = jerry_create_object();
  iotjs_jval_set_property_number(msg, IOTJS_MAGIC_STRING_ID, msgId);
  iotjs_jval_set_property_number(msg, IOTJS_MAGIC_STRING_QOS, qos);
  iotjs_jval_set_property_number(msg, IOTJS_MAGIC_STRING_HEADER_SIZE,
                                 header_size);
  iotjs_jval_set_property_number(msg, IOTJS_MAGIC_STRING_PAYLOAD_REAL_SIZE,
                                 payload_real_size);
  iotjs_jval_set_property_number(msg, IOTJS_MAGIC_STRING_PAYLOAD_MISSING_SIZE,
                                 payload_missing_size);
  iotjs_jval_set_property_number(msg, IOTJS_MAGIC_STRING_PAYLOAD_SIZE,
                                 payload_size);
  iotjs_jval_set_property_boolean(msg, IOTJS_MAGIC_STRING_DUP, dup);
  iotjs_jval_set_property_boolean(msg, IOTJS_MAGIC_STRING_RETAIN, retain);

  size_t topic_len = (size_t)topic.lenstring.len;
  char* topic_data = topic.lenstring.data;
  iotjs_string_t topic_str =
      iotjs_string_create_with_size(topic_data, topic_len);
  iotjs_jval_set_property_string(msg, IOTJS_MAGIC_STRING_TOPIC, &topic_str);
  iotjs_string_destroy(&topic_str);

  // don't generate payload if pyload is incomplete currently
  if (payload_missing_size <= 0) {
    size_t size = (size_t)payload_size;
    jerry_value_t payload_buffer = iotjs_bufferwrap_create_buffer(size);
    iotjs_bufferwrap_t* payload_wrap =
        iotjs_bufferwrap_from_jbuffer(payload_buffer);
    iotjs_bufferwrap_copy(payload_wrap, (const char*)payload, size);
    iotjs_jval_set_property_jval(msg, IOTJS_MAGIC_STRING_PAYLOAD,
                                 payload_buffer);
    jerry_release_value(payload_buffer);
  }
  return msg;
}

jerry_value_t InitMQTT() {
  jerry_value_t exports = jerry_create_object();
  jerry_value_t mqttConstructor =
      jerry_create_external_function(MqttConstructor);
  iotjs_jval_set_property_jval(exports, "MqttHandle", mqttConstructor);

  jerry_value_t proto = jerry_create_object();
  iotjs_jval_set_method(proto, "_readPacket", MqttReadPacket);
  iotjs_jval_set_method(proto, "_getConnect", MqttGetConnect);
  iotjs_jval_set_method(proto, "_getPublish", MqttGetPublish);
  iotjs_jval_set_method(proto, "_getPingReq", MqttGetPingReq);
  iotjs_jval_set_method(proto, "_getAck", MqttGetAck);
  iotjs_jval_set_method(proto, "_getSubscribe", MqttGetSubscribe);
  iotjs_jval_set_method(proto, "_getUnsubscribe", MqttGetUnsubscribe);
  iotjs_jval_set_method(proto, "_getDisconnect", MqttGetDisconnect);
  iotjs_jval_set_method(proto, "_deserialize", MqttDeserialize);
  // iotjs_jval_set_method(proto, "disconnect", MqttDisconnect);
  iotjs_jval_set_property_jval(mqttConstructor, "prototype", proto);

  jerry_release_value(proto);
  jerry_release_value(mqttConstructor);
  return exports;
}
