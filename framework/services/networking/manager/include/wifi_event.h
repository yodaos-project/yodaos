

#ifndef __WIFI_EVENT_H
#define __WIFI_EVENT_H
#include <stdint.h>

#include <common.h>
#include <network_report.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_FAILED_TIMES 3
#define WIFI_ROAM_LIMIT_SIGNAL -65
#define WIFI_SWITCH_LIMIT_SIGNAL -80

enum {
    /* Event messages with fixed prefix */
    /** Authentication completed successfully and data connection enabled */
    WPA_EVENT_CONNECTED=0,
    /** Disconnected, data connection is not available */
    WPA_EVENT_DISCONNECTED,
    /** Association rejected during connection attempt */
    WPA_EVENT_ASSOC_REJECT,
    /** Authentication rejected during connection attempt */
    WPA_EVENT_AUTH_REJECT,
    /** wpa_supplicant is exiting */
    WPA_EVENT_TERMINATING,
    /** Password change was completed successfully */
    WPA_EVENT_PASSWORD_CHANGED,
    /** EAP-Request/Notification received */
    WPA_EVENT_EAP_NOTIFICATION,
    /** EAP authentication started (EAP-Request/Identity received) */
    WPA_EVENT_EAP_STARTED,
    /** EAP method proposed by the server */
    WPA_EVENT_EAP_PROPOSED_METHOD,
    /** EAP method selected */
    WPA_EVENT_EAP_METHOD,
    /** EAP peer certificate from TLS */
    WPA_EVENT_EAP_PEER_CERT,
    /** EAP peer certificate alternative subject name component from TLS */
    WPA_EVENT_EAP_PEER_ALT,
    /** EAP TLS certificate chain validation error */
    WPA_EVENT_EAP_TLS_CERT_ERROR,
    /** EAP status */
    WPA_EVENT_EAP_STATUS,
    /** EAP authentication completed successfully */
    WPA_EVENT_EAP_SUCCESS,
    /** EAP authentication failed (EAP-Failure received) */
    WPA_EVENT_EAP_FAILURE,
    /** Network block temporarily disabled (e.g., due to authentication failure) */
    WPA_EVENT_TEMP_DISABLED,
    /** Temporarily disabled network block re-enabled */
    WPA_EVENT_REENABLED,
    /** New scan started */
    WPA_EVENT_SCAN_STARTED,
    /** New scan results available */
    WPA_EVENT_SCAN_RESULTS,
    /** Scan command failed */
    WPA_EVENT_SCAN_FAILED,
    /** wpa_supplicant state change */
    WPA_EVENT_STATE_CHANGE,
    /** A new BSS entry was added (followed by BSS entry id and BSSID) */
    WPA_EVENT_BSS_ADDED,
    /** A BSS entry was removed (followed by BSS entry id and BSSID) */
    WPA_EVENT_BSS_REMOVED,
    /** No suitable network was found */
    WPA_EVENT_NETWORK_NOT_FOUND,
    /** Change in the signal level was reported by the driver */
    WPA_EVENT_SIGNAL_CHANGE,
    /** Regulatory domain channel */
    WPA_EVENT_REGDOM_CHANGE,
    /** Channel switch (followed by freq=<MHz> and other channel parameters) */
    WPA_EVENT_CHANNEL_SWITCH,
    /** IP subnet status change notification
     *
     * When using an offloaded roaming mechanism where driver/firmware takes care
     * of roaming and IP subnet validation checks post-roaming, this event can
     * indicate whether IP subnet has changed.
     *
     * The event has a status=<0/1/2> parameter where
     * 0 = unknown
     * 1 = IP subnet unchanged (can continue to use the old IP address)
     * 2 = IP subnet changed (need to get a new IP address)
     */
    WPA_EVENT_SUBNET_STATUS_UPDATE,
}WIFI_EVENT_MSG;

typedef struct 
{
  int flag ;
  uint32_t result;
  uint32_t reason;
}tRESULT;

//extern int wifi_get_result(tRESULT *result);
//extern int wifi_set_result(tRESULT *src);

extern int  wifi_start_monitor(void);
extern void wifi_stop_monitor(void);
/*
extern  int wifi_wait_for_return(uint32_t ms);
extern void wifi_wakeup_wait(void);
*/

extern void wifi_wakeup_wait(tRESULT *src);
extern int wifi_wait_for_result(tRESULT *result,uint32_t ms);

extern void wifi_trigger_scan(void *eloop_ctx); 

extern void wifi_set_state(int state);
extern void wifi_report_status(void);
extern  int wifi_respond_status(flora_call_reply_t reply);

extern  int wifi_event_get_status(tWIFI_STATUS *status);
extern int wifi_event_set_status(tWIFI_STATUS *status);

#ifdef __cplusplus
}
#endif
#endif
