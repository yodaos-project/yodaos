#include "iotjs_def.h"
#include "iotjs_exception.h"
#include <limits.h>    // PATH_MAX on Solaris.
#include <netdb.h>     // MAXHOSTNAMELEN on Solaris.
#include <sys/param.h> // MAXHOSTNAMELEN on Linux and the BSDs.
#include <sys/utsname.h>
#include <unistd.h> // gethostname, sysconf

JS_FUNCTION(GetHostname) {
  char buf[MAXHOSTNAMELEN + 1];
  if (gethostname(buf, sizeof(buf))) {
    return JS_CREATE_ERROR(COMMON, "get hostname failed");
  }
  buf[sizeof(buf) - 1] = '\0';
  return jerry_create_string((jerry_char_t*)buf);
}

JS_FUNCTION(GetUptime) {
  double uptime;
  int err = uv_uptime(&uptime);
  if (err)
    return jerry_create_null();
  return jerry_create_number(uptime);
}

JS_FUNCTION(GetTotalMemory) {
  double amount = uv_get_total_memory();
  if (amount < 0)
    return jerry_create_null();
  return jerry_create_number(amount);
}

JS_FUNCTION(GetFreeMemory) {
  double amount = uv_get_free_memory();
  if (amount < 0)
    return jerry_create_null();
  return jerry_create_number(amount);
}

JS_FUNCTION(GetInterfaceAddresses) {
  uv_interface_address_t* interfaces;
  uint32_t count, i;
  char ip[INET6_ADDRSTRLEN];
  char netmask[INET6_ADDRSTRLEN];
  char mac[18];
  char broadcast[INET6_ADDRSTRLEN];

  int err = uv_interface_addresses(&interfaces, (int*)&count);
  if (err) {
    return JS_CREATE_ERROR(COMMON, "get network info failed");
  }

  jerry_value_t addrs = jerry_create_array(count);

  for (i = 0; i < count; i++) {
    jerry_value_t family;
    const char* const raw_name = interfaces[i].name;
    snprintf(mac, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             (unsigned char)interfaces[i].phys_addr[0],
             (unsigned char)interfaces[i].phys_addr[1],
             (unsigned char)interfaces[i].phys_addr[2],
             (unsigned char)interfaces[i].phys_addr[3],
             (unsigned char)interfaces[i].phys_addr[4],
             (unsigned char)interfaces[i].phys_addr[5]);

    if (interfaces[i].address.address4.sin_family == AF_INET) {
      uv_ip4_name(&interfaces[i].address.address4, ip, sizeof(ip));
      uv_ip4_name(&interfaces[i].netmask.netmask4, netmask, sizeof(netmask));
      uv_ip4_name(&interfaces[i].broadcast.broadcast4, broadcast,
                  sizeof(broadcast));
      family = jerry_create_string((jerry_char_t*)"IPv4");
    } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
      uv_ip6_name(&interfaces[i].address.address6, ip, sizeof(ip));
      uv_ip6_name(&interfaces[i].netmask.netmask6, netmask, sizeof(netmask));
      uv_ip6_name(&interfaces[i].broadcast.broadcast6, broadcast,
                  sizeof(broadcast));
      family = jerry_create_string((jerry_char_t*)"IPv6");
    } else {
      strncpy(ip, "<unknown sa family>", INET6_ADDRSTRLEN);
      family = jerry_create_string((jerry_char_t*)"Unknown");
    }

    jerry_value_t addr = jerry_create_object();
    jerry_value_t name_key = jerry_create_string((const jerry_char_t*)"name");
    jerry_value_t ip_name = jerry_create_string((const jerry_char_t*)"address");
    jerry_value_t netmask_name =
        jerry_create_string((const jerry_char_t*)"netmask");
    jerry_value_t broadcast_name =
        jerry_create_string((const jerry_char_t*)"broadcast");
    jerry_value_t family_name =
        jerry_create_string((const jerry_char_t*)"family");
    jerry_value_t mac_name = jerry_create_string((const jerry_char_t*)"mac");

    jerry_value_t name_data =
        jerry_create_string((const jerry_char_t*)raw_name);
    jerry_value_t ip_data = jerry_create_string((const jerry_char_t*)ip);
    jerry_value_t netmask_data =
        jerry_create_string((const jerry_char_t*)netmask);
    jerry_value_t mac_data = jerry_create_string((const jerry_char_t*)mac);
    jerry_value_t broadcast_data =
        jerry_create_string((const jerry_char_t*)broadcast);

    jerry_set_property(addr, name_key, name_data);
    jerry_set_property(addr, ip_name, ip_data);
    jerry_set_property(addr, netmask_name, netmask_data);
    jerry_set_property(addr, family_name, family);
    jerry_set_property(addr, broadcast_name, broadcast_data);
    jerry_set_property(addr, mac_name, mac_data);

    jerry_release_value(name_key);
    jerry_release_value(ip_name);
    jerry_release_value(netmask_name);
    jerry_release_value(family_name);
    jerry_release_value(broadcast_name);
    jerry_release_value(mac_name);

    jerry_release_value(name_data);
    jerry_release_value(ip_data);
    jerry_release_value(netmask_data);
    jerry_release_value(broadcast_data);
    jerry_release_value(family);
    jerry_release_value(mac_data);

    jerry_set_property_by_index(addrs, i, addr);
    jerry_release_value(addr);
  }
  uv_free_interface_addresses(interfaces, (int)count);
  return addrs;
}

JS_FUNCTION(GetOSRelease) {
  const char* rval;
  struct utsname info;
  if (uname(&info) < 0) {
    return JS_CREATE_ERROR(COMMON, "get os release failed");
  }
#ifdef _AIX
  char release[256];
  snprintf(release, sizeof(release), "%s.%s", info.version, info.release);
  rval = release;
#else
  rval = info.release;
#endif
  return jerry_create_string((const jerry_char_t*)rval);
}

JS_FUNCTION(GetPriority) {
  int priority;
  int pid = JS_GET_ARG(0, number);
  const int err = uv_os_getpriority(pid, &priority);
  if (err) {
    return iotjs_create_uv_exception(err, "uv_os_getpriority");
  }
  return jerry_create_number(priority);
}

JS_FUNCTION(SetPriority) {
  int pid = JS_GET_ARG(0, number);
  int priority = JS_GET_ARG(1, number);
  const int err = uv_os_setpriority(pid, priority);
  if (err) {
    return iotjs_create_uv_exception(err, "uv_os_setpriority");
  }
  return jerry_create_number(priority);
}

jerry_value_t InitOs() {
  jerry_value_t os = jerry_create_object();
  iotjs_jval_set_method(os, IOTJS_MAGIC_STRING_GETHOSTNAME, GetHostname);
  iotjs_jval_set_method(os, IOTJS_MAGIC_STRING_GETUPTIME, GetUptime);
  iotjs_jval_set_method(os, IOTJS_MAGIC_STRING_GETTOTALMEM, GetTotalMemory);
  iotjs_jval_set_method(os, IOTJS_MAGIC_STRING_GETFREEMEM, GetFreeMemory);
  iotjs_jval_set_method(os, IOTJS_MAGIC_STRING_GETIFACEADDR,
                        GetInterfaceAddresses);
  iotjs_jval_set_method(os, IOTJS_MAGIC_STRING__GETOSRELEASE, GetOSRelease);
  iotjs_jval_set_method(os, IOTJS_MAGIC_STRING_GETPRIORITY, GetPriority);
  iotjs_jval_set_method(os, IOTJS_MAGIC_STRING_SETPRIORITY, SetPriority);

  return os;
}
