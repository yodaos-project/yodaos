/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>

#include "gdbus/gdbus.h"
//#include "display.h"
#include "agent.h"
#include <hardware/bt_common.h>
#include <app_manager.h>

#define AGENT_PATH "/org/bluez/agent"
#define AGENT_INTERFACE "org.bluez.Agent1"

#define AGENT_PROMPT	"[agent]"

static gboolean agent_registered = FALSE;
static const char *agent_capability = NULL;
static DBusMessage *pending_message = NULL;

extern manager_cxt *_global_manager_ctx;

static void agent_release_prompt(void)
{
	if (!pending_message)
		return;

	//rl_release_prompt("");
}

int has_agent_registered()
{
    return agent_registered;
}

dbus_bool_t agent_completion(void)
{
	if (!pending_message)
		return FALSE;

	return TRUE;
}

static void pincode_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;

	g_dbus_send_reply(conn, pending_message, DBUS_TYPE_STRING, &input,
							DBUS_TYPE_INVALID);
}

static void passkey_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;
	dbus_uint32_t passkey;

	if (sscanf(input, "%u", &passkey) == 1)
		g_dbus_send_reply(conn, pending_message, DBUS_TYPE_UINT32,
						&passkey, DBUS_TYPE_INVALID);
	else if (!strcmp(input, "no"))
		g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Rejected", NULL);
	else
		g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Canceled", NULL);
}

static void confirm_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;

	if (!strcmp(input, "yes"))
		g_dbus_send_reply(conn, pending_message, DBUS_TYPE_INVALID);
	else if (!strcmp(input, "no"))
		g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Rejected", NULL);
	else
		g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Canceled", NULL);
}

static void agent_release(DBusConnection *conn)
{
	agent_registered = FALSE;
	agent_capability = NULL;

	if (pending_message) {
		dbus_message_unref(pending_message);
		pending_message = NULL;
	}

	agent_release_prompt();

	g_dbus_unregister_interface(conn, AGENT_PATH, AGENT_INTERFACE);
}

static DBusMessage *release_agent(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	BT_LOGI("Agent released\n");

	agent_release(conn);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_pincode(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	const char *device;
    char *input;

	BT_LOGI("Request PIN code\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
							DBUS_TYPE_INVALID);

	//rl_prompt_input("agent", "Enter PIN code:", pincode_response, conn);
    pending_message = dbus_message_ref(msg);
    //g_dbus_send_reply(conn, pending_message, DBUS_TYPE_INVALID);
    input = g_strdup("0000");
    g_dbus_send_reply(conn, pending_message, DBUS_TYPE_STRING, &input, DBUS_TYPE_INVALID);
    g_free(input);

	return NULL;
}

static DBusMessage *display_pincode(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	const char *device;
	const char *pincode;

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
				DBUS_TYPE_STRING, &pincode, DBUS_TYPE_INVALID);

	BT_LOGI(AGENT_PROMPT "PIN code: %s\n", pincode);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_passkey(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	const char *device;
    dbus_uint32_t passkey = 0;

	BT_LOGI("Request passkey\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
							DBUS_TYPE_INVALID);

	//rl_prompt_input("agent", "Enter passkey (number in 0-999999):",
	//		passkey_response, conn);

	pending_message = dbus_message_ref(msg);
    g_dbus_send_reply(conn, pending_message, DBUS_TYPE_UINT32,
						&passkey, DBUS_TYPE_INVALID);

	return NULL;
}

static DBusMessage *display_passkey(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	const char *device;
	dbus_uint32_t passkey;
	dbus_uint16_t entered;
	char passkey_full[7];

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_UINT16, &entered,
							DBUS_TYPE_INVALID);

	snprintf(passkey_full, sizeof(passkey_full), "%.6u", passkey);
	passkey_full[6] = '\0';

	if (entered > strlen(passkey_full))
		entered = strlen(passkey_full);

	BT_LOGI("Passkey: %.*s %s",
				entered, passkey_full, passkey_full + entered);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_confirmation(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	const char *device;
	dbus_uint32_t passkey;
	//char *str;

	BT_LOGI("Request confirmation\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
				DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_INVALID);

	//str = g_strdup_printf("Confirm passkey %06u (yes/no):", passkey);
	//rl_prompt_input("agent", str, confirm_response, conn);
	BT_LOGI("passkey %06u", passkey);
    pending_message = dbus_message_ref(msg);
    g_dbus_send_reply(conn, pending_message, DBUS_TYPE_INVALID);
	//g_free(str);

	//pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *request_authorization(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	const char *device;

	BT_LOGI("Request authorization\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
							DBUS_TYPE_INVALID);

	//rl_prompt_input("agent", "Accept pairing (yes/no):", confirm_response,
	//							conn);

	pending_message = dbus_message_ref(msg);
    g_dbus_send_reply(conn, pending_message, DBUS_TYPE_INVALID);

	return NULL;
}

static DBusMessage *authorize_service(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    const char *device, *uuid;
    //char *str;

    //BT_LOGI("Authorize service\n");

    dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
				DBUS_TYPE_STRING, &uuid, DBUS_TYPE_INVALID);

    //str = g_strdup_printf("Authorize service %s (yes/no):", uuid);
    //rl_prompt_input("agent", str, confirm_response, conn);
    //g_free(str);
    BT_LOGI("service %s", uuid);
    pending_message = dbus_message_ref(msg);
    /* todo not support HSP HFP yet */
    if (!strcmp(uuid, HSP_AG_UUID) || !strcmp(uuid, HSP_HS_UUID)
        || !strcmp(uuid, HFP_HS_UUID) || !strcmp(uuid, HFP_AG_UUID)) {
        BT_LOGW("service %s Rejected, we dont want this service", uuid);
        g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Rejected", NULL);
    }
    else if (/*!strcmp(uuid, ADVANCED_AUDIO_UUID) && */_global_manager_ctx
            && _global_manager_ctx->a2dp_sink && _global_manager_ctx->a2dp_sink->enabled) {
        g_dbus_send_reply(conn, pending_message, DBUS_TYPE_INVALID);
    }
    else if (/*!strcmp(uuid, ADVANCED_AUDIO_UUID) && */_global_manager_ctx
            && _global_manager_ctx->a2dp_ctx && _global_manager_ctx->a2dp_ctx->enabled) {
            g_dbus_send_reply(conn, pending_message, DBUS_TYPE_INVALID);
    }
    else {
        BT_LOGW("service %s Rejected, we dont want this service", uuid);
        g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Rejected", NULL);
    }
    return NULL;
}

static DBusMessage *cancel_request(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	BT_LOGI("Request canceled\n");

	agent_release_prompt();
	dbus_message_unref(pending_message);
	pending_message = NULL;

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable methods[] = {
	{ GDBUS_METHOD("Release", NULL, NULL, release_agent) },
 #if 1
	{ GDBUS_ASYNC_METHOD("RequestPinCode",
			GDBUS_ARGS({ "device", "o" }),
			GDBUS_ARGS({ "pincode", "s" }), request_pincode) },
#endif
	{ GDBUS_METHOD("DisplayPinCode",
			GDBUS_ARGS({ "device", "o" }, { "pincode", "s" }),
			NULL, display_pincode) },
	{ GDBUS_ASYNC_METHOD("RequestPasskey",
			GDBUS_ARGS({ "device", "o" }),
			GDBUS_ARGS({ "passkey", "u" }), request_passkey) },
	{ GDBUS_METHOD("DisplayPasskey",
			GDBUS_ARGS({ "device", "o" }, { "passkey", "u" },
							{ "entered", "q" }),
			NULL, display_passkey) },
	{ GDBUS_ASYNC_METHOD("RequestConfirmation",
			GDBUS_ARGS({ "device", "o" }, { "passkey", "u" }),
			NULL, request_confirmation) },
	{ GDBUS_ASYNC_METHOD("RequestAuthorization",
			GDBUS_ARGS({ "device", "o" }),
			NULL, request_authorization) },
	{ GDBUS_ASYNC_METHOD("AuthorizeService",
			GDBUS_ARGS({ "device", "o" }, { "uuid", "s" }),
			NULL,  authorize_service) },
	{ GDBUS_METHOD("Cancel", NULL, NULL, cancel_request) },
	{ }
};

static void register_agent_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = AGENT_PATH;
	const char *capability = agent_capability;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &capability);
}

static void register_agent_reply(DBusMessage *message, void *user_data)
{
	DBusConnection *conn = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == FALSE) {
		agent_registered = TRUE;
		BT_LOGI("Agent registered\n");
	} else {
		BT_LOGI("Failed to register agent: %s\n", error.name);
		dbus_error_free(&error);

		if (g_dbus_unregister_interface(conn, AGENT_PATH,
						AGENT_INTERFACE) == FALSE)
			BT_LOGI("Failed to unregister agent object\n");
	}
}

void agent_register(DBusConnection *conn, GDBusProxy *manager,
						const char *capability)

{
	if (agent_registered == TRUE) {
		BT_LOGI("Agent is already registered\n");
		return;
	}
    BT_LOGI("entry");
	agent_capability = capability;

	if (g_dbus_register_interface(conn, AGENT_PATH,
					AGENT_INTERFACE, methods,
					NULL, NULL, NULL, NULL) == FALSE) {
		BT_LOGI("Failed to register agent object\n");
		return;
	}

	if (g_dbus_proxy_method_call(manager, "RegisterAgent",
						register_agent_setup,
						register_agent_reply,
						conn, NULL) == FALSE) {
		BT_LOGI("Failed to call register agent method\n");
		return;
	}

	agent_capability = NULL;
}

static void unregister_agent_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = AGENT_PATH;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void unregister_agent_reply(DBusMessage *message, void *user_data)
{
	DBusConnection *conn = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == FALSE) {
		BT_LOGI("Agent unregistered\n");
		agent_release(conn);
	} else {
		BT_LOGI("Failed to unregister agent: %s\n", error.name);
		dbus_error_free(&error);
	}
}

void agent_unregister(DBusConnection *conn, GDBusProxy *manager)
{
	if (agent_registered == FALSE) {
		BT_LOGI("No agent is registered\n");
		return;
	}
    BT_LOGI("entry");
	if (!manager) {
		BT_LOGI("Agent unregistered\n");
		agent_release(conn);
		return;
	}

	if (g_dbus_proxy_method_call(manager, "UnregisterAgent",
						unregister_agent_setup,
						unregister_agent_reply,
						conn, NULL) == FALSE) {
		BT_LOGI("Failed to call unregister agent method\n");
		return;
	}
}

static void request_default_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = AGENT_PATH;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void request_default_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		BT_LOGI("Failed to request default agent: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	BT_LOGI("Default agent request successful\n");
}

void agent_default(DBusConnection *conn, GDBusProxy *manager)
{
	if (agent_registered == FALSE) {
		BT_LOGI("No agent is registered\n");
		return;
	}
    BT_LOGI("entry");
	if (g_dbus_proxy_method_call(manager, "RequestDefaultAgent",
						request_default_setup,
						request_default_reply,
						NULL, NULL) == FALSE) {
		BT_LOGI("Failed to call request default agent method\n");
		return;
	}
}
