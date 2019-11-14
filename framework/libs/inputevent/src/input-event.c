#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#include <sys/wait.h>
#include <sys/select.h>

#include <linux/input.h>

#include <sys/reboot.h>
#include "input-event.h"
#include "input-event-table.h"
#define LOG_TAG "input-event"
#include "rklog/RKLog.h"

static int i, select_r, fd_len;
static unsigned long tms_start, tms_end, idle_time = 0;
static fd_set fdset, initial_fdset;
static struct input_event event={0};
static struct timeval tv={0}, tv_start={0}, tv_end={0};
static int MAX_DOUBLE_CLICK_DELAY_TIME = 500;
static int MAX_SLIDE_DELAY_TIME = 500;
static int TIME_OUT = 300;
static bool HAS_SLIDE_EVENT;

static const char *sw_config = "/etc/gpio.config";
static int sw_keycode[MAX_LISTENER]={0};
static int sw_keyvalue[MAX_LISTENER]={0};
static int sw_keynumber = 0;

// click =============================== start
static int click_key_code = -1;
static int click_key_count = 0;
static unsigned long click_time = 0;
static unsigned long cache_press_time = 0;
static bool long_press_state = false;
static int long_press_code = -1;
// click ===============================  end

// slide =============================== start
static int slide_key_code = -1;
static int KEY_ARRAYS[KEY_COUNTS] = {2,3,4,5,6,7,8,9};
static int MINUS_KEY_VALUE = -2;
static unsigned long slide_time = 0;
// slide ===============================  end



void string_left_space_trim(char *pStr)
{
    char *pTmp = pStr;

    while (*pTmp == ' ')
    {
        pTmp++;
    }
    while(*pTmp != '\0')
    {
        *pStr = *pTmp;
        pStr++;
        pTmp++;
    }
    *pStr = '\0';
}
int execute_local_command(const char *cmd, char *result, int size)
{
	char chLine[128] = {0};
	char chPs[128] = { 0 };
	FILE *pp = NULL;
	int ret = -1;

	sprintf(chPs, "%s", cmd);
	if((pp = popen(chPs, "r")) != NULL)
	{
		while(fgets(chLine, sizeof(chLine), pp) != NULL)
		{
			fprintf(stderr, "%s, read out line is %s", __func__, chLine);
			if(strlen(chLine) > 1)
			{
				string_left_space_trim(chLine);
				strcat(result, chLine);
			}
		}
		pclose(pp);
		pp = NULL;

		ret = 0;
	}
	else
	{
		fprintf(stderr, "popen %s error\n", chPs);
		ret = -1;
	}

	return ret;
}

//static char dbus_address[128] = "DBUS_SESSION_BUS_ADDRESS";
//static char dbus_pid[128] = "DBUS_SESSION_BUS_PID";
/*int DbusEnvInit(const char* path)
{
    FILE *fp = NULL;
    char buf[128];
    int ret = 0;

    if (path == NULL)
        path = DEFAULT_PAHT;

    fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "open %s failed. errno:%d\n", path, errno);
        ret = -ENXIO;
        return ret;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        if (!strncmp(buf, dbus_address, 24)) {
            memset(dbus_address, 0, 128);
            strncpy(dbus_address, buf, strlen(buf)-1);
            putenv(dbus_address);
            continue;
        }
        if (!strncmp(buf, dbus_pid, 20)) {
            memset(dbus_pid, 0, 128);
            strncpy(dbus_pid, buf, strlen(buf)-1);
            putenv(dbus_pid);
            continue;
        }
    }
    fprintf(stderr, "getenv DBUS_SESSION_BUS_ADDRESS=%s\n", getenv("DBUS_SESSION_BUS_ADDRESS"));
    fprintf(stderr, "getenv DBUS_SESSION_BUS_PID=%s\n", getenv("DBUS_SESSION_BUS_PID"));

err:
    fclose(fp);
    return ret;
}*/

static int isCharging = 0;
#define IS_CHARGING "ls -l /etc/systemd/system/ | grep default | grep sysinit"
#define SIZE_4K (4 * 1024)
int ChargeModeDetect(void)
{
	char chQueryRes[SIZE_4K] = {0};
	int ret = -1;

	ret = execute_local_command(IS_CHARGING, chQueryRes, SIZE_4K);
	if (!ret && (chQueryRes != NULL) && (strlen(chQueryRes) > 2)) {
		isCharging = 1;
		fprintf(stderr, "Input Event run in charging mode.\n");
	} else {
		isCharging = 0;
		fprintf(stderr, "Input Event run in default mode.\n");
	}

	return ret;
}

int key_event_compare(const key_event_t *a, const key_event_t *b) {
    int i, r_cmp;

    if((r_cmp = strcmp(a->code, b->code)) != 0) {
        return r_cmp;
    } else if(a->modifier_n != b->modifier_n) {
        return (a->modifier_n - b->modifier_n);
    } else {
        for(i=0; i < a->modifier_n; i++) {
            if((r_cmp = strcmp(a->modifiers[i], b->modifiers[i])) != 0) {
                return r_cmp;
            }
        }
    }
    return 0;
}

const char *key_event_name(unsigned int code) {
    if(code < KEY_MAX && KEY_NAME[code] != NULL) {
        return KEY_NAME[code];
    } else {
        return "UNKNOWN";
    }
}

const char *key_event_modifier_name(const char* code) {
    if(
        strcmp(code, "LEFTCTRL") == 0 || strcmp(code, "RIGHTCTRL") == 0
    ) {
        return "CTRL";
    } else if(
        strcmp(code, "LEFTALT") == 0 || strcmp(code, "RIGHTALT") == 0
    ) {
        return "ALT";
    } else if(
        strcmp(code, "LEFTSHIFT") == 0 || strcmp(code, "RIGHTSHIFT") == 0
    ) {
        return "SHIFT";
    } else if(
        strcmp(code, "LEFTMETA") == 0 || strcmp(code, "RIGHTMETA") == 0
    ) {
        return "META";
    }

    return code;
}

key_event_t
*key_event_parse(unsigned int code, int pressed, const char *src) {
    key_event_t *fired_key_event = NULL;
    static key_event_t current_key_event = {
        .code = NULL,
        .modifier_n = 0
    };

    if(pressed) {

        /* ignore if repeated */
        if(
            current_key_event.code != NULL &&
            strcmp(current_key_event.code, key_event_name(code)) == 0
        ) {
            return NULL;
        }

        /* if previous key present and modifier limit not yet reached */
        if(
            current_key_event.code != NULL &&
            current_key_event.modifier_n < MAX_MODIFIERS
        ) {
            int i;

            /* if not already in the modifiers array */
            for(i=0; i < current_key_event.modifier_n; i++) {
                if(strcmp(
                        current_key_event.modifiers[i], current_key_event.code
                    ) == 0
                ) {
                    return NULL;
                }
            }

            /* add previous key as modifier */
            current_key_event.modifiers[current_key_event.modifier_n++] =
                key_event_modifier_name(current_key_event.code);

        }

        current_key_event.code = key_event_name(code);

        if(current_key_event.modifier_n == 0) {
            if(conf.monitor) {
                printf("%s:\n  keys      : ", src);
                printf("%s\n\n", current_key_event.code);
            }

            fired_key_event = bsearch(
                &current_key_event,
                key_events,
                key_event_n,
                sizeof(key_event_t),
                (int (*)(const void *, const void *)) key_event_compare
            );
        }


    } else {
        int i, new_modifier_n = 0;
        const char *new_modifiers[MAX_MODIFIERS], *modifier_code;

        if(current_key_event.code != NULL && current_key_event.modifier_n > 0) {

            if(conf.monitor) {
                printf("%s:\n  keys     : ", src);
                for(i=0; i < current_key_event.modifier_n; i++) {
                     printf("%s + ", current_key_event.modifiers[i]);
                }
                printf("%s\n\n", current_key_event.code);
            }

            qsort(
                current_key_event.modifiers,
                current_key_event.modifier_n,
                sizeof(const char*),
                (int (*)(const void *, const void *)) strcmp
            );

            fired_key_event = bsearch(
                &current_key_event,
                key_events,
                key_event_n,
                sizeof(key_event_t),
                (int (*)(const void *, const void *)) key_event_compare
            );

        }

        if(
            current_key_event.code != NULL &&
            strcmp(current_key_event.code, key_event_name(code)) == 0
        ) {
            current_key_event.code = NULL;
        }

        /* remove released key from modifiers */
        modifier_code = key_event_modifier_name(key_event_name(code));
        for(i=0; i < current_key_event.modifier_n; i++) {
            if(strcmp(current_key_event.modifiers[i], modifier_code) != 0) {
                new_modifiers[new_modifier_n++] =
                    current_key_event.modifiers[i];
            }
        }
        memcpy(current_key_event.modifiers, new_modifiers,
            MAX_MODIFIERS*sizeof(const char*));
        current_key_event.modifier_n = new_modifier_n;

    }

    if(conf.verbose && fired_key_event) {
        int i;

        fprintf(stderr, "\nkey_event:\n"
                        "  code     : ");
        for(i=0; i < fired_key_event->modifier_n; i++) {
            fprintf(stderr, "%s + ", fired_key_event->modifiers[i]);
        }

        fprintf(stderr, "%s\n"
                        "  source   : %s\n"
                        "  exec     : \"%s\"\n\n",
                        fired_key_event->code,
                        src,
                        fired_key_event->exec
        );
    }

    return fired_key_event;
}

int idle_event_compare(const idle_event_t *a, const idle_event_t *b) {
    return (a->timeout - b->timeout);
}

int idle_event_parse(unsigned long idle) {
    idle_event_t *fired_idle_event;
    idle_event_t current_idle_event = {
        .timeout = idle
    };

    fired_idle_event = bsearch(
        &current_idle_event,
        idle_events,
        idle_event_n,
        sizeof(idle_event_t),
        (int (*)(const void *, const void *)) idle_event_compare
    );

    if(fired_idle_event != NULL) {
        if(conf.verbose) {
            fprintf(stderr, "\nidle_event:\n");

            if(fired_idle_event->timeout == IDLE_RESET) {
                fprintf(stderr, "  time     : idle reset\n");
            } else {
                fprintf(stderr, "  time     : %ldh %ldm %lds\n",
                                fired_idle_event->timeout / 3600,
                                fired_idle_event->timeout % 3600 / 60,
                                fired_idle_event->timeout % 60
                );
            }

            fprintf(stderr, "  exec     : \"%s\"\n\n", fired_idle_event->exec);
        }
        daemon_exec(fired_idle_event->exec);
    }

    return (fired_idle_event != NULL);
}

int
switch_event_compare(const switch_event_t *a, const switch_event_t *b) {
    int r_cmp;

    if((r_cmp = strcmp(a->code, b->code)) != 0) {
        return r_cmp;
    } else {
        return (a->value - b->value);
    }
}

const char *switch_event_name(unsigned int code) {
    if(code < SW_MAX && SW_NAME[code] != NULL) {
        return SW_NAME[code];
    } else {
        return "UNKNOWN";
    }
}

switch_event_t
*switch_event_parse(unsigned int code, int value, const char *src) {
    switch_event_t *fired_switch_event;
    switch_event_t current_switch_event = {
        .code = switch_event_name(code),
        .value = value
    };

    if(conf.monitor) {
        printf("%s:\n  switch   : %s:%d\n\n",
            src,
            current_switch_event.code,
            current_switch_event.value
        );
    }

    fired_switch_event = bsearch(
        &current_switch_event,
        switch_events,
        switch_event_n,
        sizeof(switch_event_t),
        (int (*)(const void *, const void *)) switch_event_compare
    );

    if(conf.verbose && fired_switch_event) {
        fprintf(stderr, "\nswitch_event:\n"
                        "  switch   : %s:%d\n"
                        "  source   : %s\n"
                        "  exec     : \"%s\"\n\n",
                        fired_switch_event->code,
                        fired_switch_event->value,
                        src,
                        fired_switch_event->exec
        );
    }

    return fired_switch_event;
}

void input_open_all_listener() {
    int i, listen_len = 0;
    char filename[32];

    for(i=0; i<MAX_LISTENER; i++) {
        snprintf(filename, sizeof(filename), "/dev/input/event%d", i);
        if(access(filename, R_OK) != 0) {
            if(errno != ENOENT) {
                fprintf(stderr, PROGRAM": access(%s): %s\n",
                    filename, strerror(errno));
            }
            continue;
        }
        conf.listen[listen_len++] = strdup(filename);
    }
}

void input_list_devices() {
    int fd, i, e;
    unsigned char evmask[EV_MAX/8 + 1];

    for(i=0; i < MAX_LISTENER && conf.listen[i] != NULL; i++) {
        char phys[64] = "no physical path", name[256] = "Unknown Device";

        fd = open(conf.listen[i], O_RDONLY);
        if(fd < 0) {
            fprintf(stderr, PROGRAM": open(%s): %s\n",
                conf.listen[i], strerror(errno));
            continue;
        }

        memset(evmask, '\0', sizeof(evmask));

        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        ioctl(fd, EVIOCGPHYS(sizeof(phys)), phys);
        ioctl(fd, EVIOCGBIT(0, sizeof(evmask)), evmask);

        printf("%s:\n", conf.listen[i]);
        printf("  name     : %s\n", name);
        printf("  phys     : %s\n", phys);

        printf("  features :");
        for(e=0; e<EV_MAX; e++) {
            if (test_bit(evmask, e)) {
                const char *feature = "unknown";
                switch(e) {
                    case EV_SYN: feature = "syn";      break;
                    case EV_KEY: feature = "keys";     break;
                    case EV_REL: feature = "relative"; break;
                    case EV_ABS: feature = "absolute"; break;
                    case EV_MSC: feature = "reserved"; break;
                    case EV_LED: feature = "leds";     break;
                    case EV_SND: feature = "sound";    break;
                    case EV_REP: feature = "repeat";   break;
                    case EV_FF:  feature = "feedback"; break;
                    case EV_SW:  feature = "switch";   break;
                }
                printf(" %s", feature);
            }
        }
        printf("\n\n");
        close(fd);
    }
}

void input_parse_event(struct input_event *event, const char *src) {
    key_event_t *fired_key_event;
    switch_event_t *fired_switch_event;

    switch(event->type) {
        case EV_KEY:
            fired_key_event = key_event_parse(event->code, event->value, src);

            if(fired_key_event != NULL) {
                daemon_exec(fired_key_event->exec);
            }
            break;
        case EV_SW:
            fired_switch_event =
                switch_event_parse(event->code, event->value, src);

            if(fired_switch_event != NULL) {
                daemon_exec(fired_switch_event->exec);
            }

            break;
    }
}


void config_parse_file() {
    FILE *config_fd;
    char buffer[512], *line;
    char *section = NULL;
    char *key, *value, *ptr;
    const char *error = NULL;
    int line_num = 0;
    int listen_len = 0;

    if((config_fd = fopen(conf.configfile, "r")) == NULL) {
        fprintf(stderr, PROGRAM": fopen(%s): %s\n",
            conf.configfile, strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, sizeof(buffer));

    while(fgets(buffer, sizeof(buffer), config_fd) != NULL) {
        line = config_trim_string(buffer);
        line_num++;

        if(line[0] == '\0' || line[0] == '#') {
            continue;
        }

        if((ptr = strchr(line, '#'))) {
            *ptr = '\0';
        }

        if(line[0] == '[' && line[strlen(line)-1] == ']') {
            if(section != NULL) {
                free(section);
            }
            line[strlen(line)-1] = '\0';
            section = config_trim_string(strdup(line+1));
            continue;
        }

        key = value = line;
        strsep(&value, "=");
        if(value == NULL) {
            error = "Invalid syntax!";
            goto print_error;
        }

        key = config_trim_string(key);
        value = config_trim_string(value);

        if(section == NULL) {
            error = "Missing section!";
        } else if(strlen(key) == 0 || strlen(value) == 0) {
            error = "Invlaid syntax!";
        } else if(strcasecmp(section, "Global") == 0) {
            if(strcmp(key, "listen") == 0) {
                if(listen_len < MAX_LISTENER) {
                    conf.listen[listen_len++] = strdup(value);
                } else {
                    error = "Listener limit exceeded!";
                }
            } else {
                error = "Unkown option!";
            }
        } else if(strcasecmp(section, "Keys") == 0) {
            error = config_key_event(key, value);
        } else if(strcasecmp(section, "Idle") == 0) {
            error = config_idle_event(key, value);
        } else if(strcasecmp(section, "Switches") == 0) {
            error = config_switch_event(key, value);
        } else {
            error = "Unknown section!";
            section = NULL;
        }

        print_error:
        if(error != NULL) {
            fprintf(stderr, PROGRAM": %s (%s:%d)\n",
                error, conf.configfile, line_num);
        }


    }

    qsort(key_events, key_event_n, sizeof(key_event_t),
        (int (*)(const void *, const void *)) key_event_compare);

    qsort(idle_events, idle_event_n, sizeof(idle_event_t),
        (int (*)(const void *, const void *)) idle_event_compare);

    qsort(switch_events, switch_event_n, sizeof(switch_event_t),
        (int (*)(const void *, const void *)) switch_event_compare);

    if(section != NULL) {
        free(section);
    }

    fclose(config_fd);
}

const char *config_key_event(char *shortcut, char *exec) {
    int i;
    char *code, *modifier;
    key_event_t *new_key_event;

    if(key_event_n >= MAX_EVENTS) {
        return "Key event limit exceeded!";
    } else {
        new_key_event = &key_events[key_event_n++];
    }

    new_key_event->modifier_n = 0;
    for(i=0; i < MAX_MODIFIERS; i++) {
        new_key_event->modifiers[i] = NULL;
    }

    if((code = strrchr(shortcut, '+')) != NULL) {
        *code = '\0';
        code = config_trim_string(code+1);

        new_key_event->code = strdup(code);

        modifier = strtok(shortcut, "+");
        while(modifier != NULL && new_key_event->modifier_n < MAX_MODIFIERS) {
            modifier = config_trim_string(modifier);
            new_key_event->modifiers[new_key_event->modifier_n++] =
                strdup(modifier);

            modifier = strtok(NULL, "+");
        }
    } else {
        new_key_event->code = strdup(shortcut);
    }

    new_key_event->exec = strdup(exec);

    qsort(new_key_event->modifiers, new_key_event->modifier_n,
        sizeof(const char*), (int (*)(const void *, const void *)) strcmp);

    return NULL;
}

const char *config_idle_event(char *timeout, char *exec) {
    idle_event_t *new_idle_event;
    unsigned long count;
    char *unit;

    if(idle_event_n >= MAX_EVENTS) {
        return "Idle event limit exceeded!";
    } else {
        new_idle_event = &idle_events[idle_event_n++];
    }

    new_idle_event->timeout = 0;
    new_idle_event->exec = strdup(exec);

    if(strcasecmp(timeout, "RESET") == 0) {
        new_idle_event->timeout = IDLE_RESET;
        return NULL;
    }

    while(*timeout) {
        while(*timeout && !isdigit(*timeout)) timeout++;
        count = strtoul(timeout, &unit, 10);

        switch(*unit) {
        case 'h':
            new_idle_event->timeout += count * 3600;
            break;
        case 'm':
            new_idle_event->timeout += count * 60;
            break;
        case 's':
        default:
            new_idle_event->timeout += count;
            break;
        }

        timeout = unit;
    }

    conf.min_timeout = config_min_timeout(
        new_idle_event->timeout, conf.min_timeout);

    return NULL;
}

const char *config_switch_event(char *switchcode, char *exec) {
    char *code, *value;
    switch_event_t *new_switch_event;

    if(switch_event_n >= MAX_EVENTS) {
        return "Switch event limit exceeded!";
    } else {
        new_switch_event = &switch_events[switch_event_n++];
    }

    code = value = switchcode;
    strsep(&value, ":");
    if(value == NULL) {
        switch_event_n--;
        return "Invalid switch identifier";
    }

    code = config_trim_string(code);
    value = config_trim_string(value);

    new_switch_event->code = strdup(code);
    new_switch_event->value = atoi(value);
    new_switch_event->exec = strdup(exec);

    return NULL;
}

unsigned int config_min_timeout(unsigned long a, unsigned long b) {
    if(b == 0) return a;

    return config_min_timeout(b, a % b);
}

char *config_trim_string(char *str) {
    char *end;

    while(isspace(*str)) str++;

    end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) end--;

    *(end+1) = '\0';

    return str;
}

void daemon_init() {
    int i;

    conf.configfile  = "/etc/input-event.conf";

    conf.monitor     = 1;
    conf.verbose     = 0;
    conf.daemon      = 1;

    conf.min_timeout = 3600;

    for(i=0; i<MAX_LISTENER; i++) {
        conf.listen[i]    = NULL;
        conf.listen_fd[i] = 0;
    }
}

#define SET_DEFAULT "systemctl set-default multi-user"
#define ISOLATE_DEFAULT "systemctl isolate multi-user"
static unsigned long tm_start = 0;
void input_powerkey_check(struct input_event *event) {
    unsigned long tmp = 0;	//ms
    unsigned int type = 0;
    unsigned int code = 0;
    unsigned int value = 0;
    char chQueryRes[SIZE_4K] = {0};

	tmp = event->time.tv_sec * 1000 + event->time.tv_usec / 1000;
    type = event->type;
    code = event->code;
    value = event->value;

    if (type == EV_KEY && code == KEY_POWER) {
        if (value == 1) {
            tm_start = tmp;
        } else if (value == 2) {
            if ((tmp - tm_start) > 1999) { /* 2s */
                /* reboot */
                //reboot(RB_AUTOBOOT);
                /* start other services */
                execute_local_command(SET_DEFAULT, chQueryRes, SIZE_4K);
                execute_local_command(ISOLATE_DEFAULT, chQueryRes, SIZE_4K);
            }
        } else if (value == 0) {
            tm_start = 0;
        }
    }
}

/*static void input_dbus_report(struct input_event *event) {
    int ret = 0;
    DBusError err;
    DBusConnection *connection;
    DBusMessage *msg;
    DBusMessageIter arg;
    dbus_uint32_t serial = 0;
    dbus_uint64_t tm = 0;
    dbus_uint16_t type = 0;
    dbus_uint16_t code = 0;
    dbus_int32_t value = 0;

    if (event == NULL)
        return;

    tm = event->time.tv_sec * 1000 + event->time.tv_usec / 1000;
    type = event->type;
    code = event->code;
    value = event->value;

    dbus_error_init(&err);
    connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Err : %s/n", err.message);
        dbus_error_free(&err);
    }
    if (connection == NULL) {
        return;
    }
    if ((msg = dbus_message_new_signal("/rokid/rkinput", "com.rokid.rkinput", "key")) == NULL) {
        fprintf(stderr, "Message NULL/n");
        return;
    }
    dbus_message_iter_init_append(msg, &arg);

    if (!dbus_message_iter_append_basic(&arg, DBUS_TYPE_UINT64, &tm)) {
        fprintf(stderr, "Out Of Memory!/n");
        goto free;
    }
    if (!dbus_message_iter_append_basic(&arg, DBUS_TYPE_UINT16, &type)) {
        fprintf(stderr, "Out Of Memory!/n");
        goto free;
    }
    if (!dbus_message_iter_append_basic(&arg, DBUS_TYPE_UINT16, &code)) {
        fprintf(stderr, "Out Of Memory!/n");
        goto free;
    }
    if (!dbus_message_iter_append_basic(&arg, DBUS_TYPE_INT32, &value)) {
        fprintf(stderr, "Out Of Memory!/n");
        goto free;
    }

    if (!dbus_connection_send(connection, msg, &serial)) {
        fprintf(stderr, "Out of Memory!/n");
        goto free;
    }
    dbus_connection_flush(connection);

free:
    dbus_message_unref(msg);

    return;
}*/

 void daemon_start_listener(struct keyevent * down_up,struct gesture * gest) {


    /* ignored forked processes */
    //signal(SIGCHLD, SIG_IGN);
       // printf("conf.monitor is %d conf.file is %s \n",conf.monitor,conf.configfile);
       // printf("daemon_start_listener is called\n");
        fdset = initial_fdset;
        tv.tv_sec = 0;
        tv.tv_usec = TIME_OUT * 1000;
        //printf("delay time is %lu\n",tv.tv_sec*1000 +tv.tv_usec);
        //tv.tv_sec = 0;

        down_up->new_action = false;
        gest->new_action = false;

        gettimeofday(&tv_start, NULL);
        tms_start = tv_start.tv_sec * 1000 + tv_start.tv_usec / 1000;
        if(sw_keynumber > 0)
        {
            sw_keynumber--;
            down_up->new_action = true;
            down_up->action = ACTION_ORDINARY_EVENT;
            down_up->key_code = sw_keycode[sw_keynumber];
            down_up->value = sw_keyvalue[sw_keynumber];
            down_up->key_time = tms_start;
            return;
        }
	select_r = -1;

	while(select_r < 0){
	    select_r = select(conf.listen_fd[fd_len-1]+1, &fdset, NULL, NULL, &tv);
	}
        gettimeofday(&tv_end, NULL);
        //printf("input-event select is called\n");

        tms_end   = tv_end.tv_sec  *  1000 + tv_end.tv_usec  /  1000;
        if(select_r < 0) {
            perror(PROGRAM": select()");
            down_up->new_action = false;
            gest->new_action = false;
            RKLoge("select_r < 0\n");
            return;
        } else if(select_r == 0) {
            idle_time += conf.min_timeout;
            idle_event_parse(idle_time);
            if(click_event(gest,NONE_KEY_CODE,1,tms_end)){
                return;
            }
            //printf("select_r = 0\n");
            down_up->new_action = false;
            gest->new_action = false;
            return;
        }


        if(tms_end - tms_start > 750 || idle_time > 0) {
            idle_event_parse(IDLE_RESET);
            idle_time = 0;
        }


        for(i=0; i<fd_len; i++) {
            if(FD_ISSET(conf.listen_fd[i], &fdset)) {
                if(read(conf.listen_fd[i], &event, sizeof(event)) < 0) {
                    fprintf(stderr, PROGRAM": read(%s): %s\n",
                        conf.listen[i], strerror(errno));
                    if(access(conf.listen[i], R_OK) != 0) {//file not exist yet, we should remove it
                        fprintf(stderr, PROGRAM": (%s) not exist, remove it!!\n",
                        conf.listen[i]);
                        FD_CLR(conf.listen_fd[i], &fdset);
                        free((void*) conf.listen[i]);
                        conf.listen[i] = NULL;
                        if(conf.listen_fd[i] != -1) {
                            close(conf.listen_fd[i]);
                            conf.listen_fd[i] = -1;
                        }
                    }
                    continue;
                }
                if (isCharging) {
                    input_powerkey_check(&event);
                }
                //input_dbus_report(&event);
                //input_parse_event(&event, conf.listen[i]);
                if(event.type == EV_SW){
                    down_up->new_action = true;
                    down_up->action = ACTION_ORDINARY_EVENT;
                    down_up->key_code = event.code;
                    down_up->value = event.value;
                    down_up->key_time = tms_start;
                    return;
                }
                else if(event.type != EV_KEY){
                    //printf("event.type is not EV_KEY\n");
                    continue;
                }
                if(event.code != 0){
                    down_up->new_action = true;
                    down_up->action = ACTION_ORDINARY_EVENT;
                    down_up->key_code = event.code;
                    down_up->value = event.value;
                    down_up->key_time = event.time.tv_sec * 1000 + event.time.tv_usec/1000;
                    down_up->key_timeval = event.time;
                }
                RKLogi("input_event[%d] type[%d] code[%d] value[%d] time[%ld]\n",i,event.type,event.code,event.value,down_up->key_time);
                if(event.code == 0){
                    //printf("event.code is 0\n");
                    event.code = NONE_KEY_CODE;
                }
                tms_start = event.time.tv_sec * 1000 + event.time.tv_usec / 1000;
                if(event.value ==2){
                    RKLogi("=============== long press ==================\n");
                    click_key_code = -1;
                    click_time = 0;
                    click_key_count = 0;
                    if (((tms_start - cache_press_time) > 2000) && event.code == long_press_code){
                        long_press_code = event.code;
                        gest->new_action = true;
                        gest->action = ACTION_LONG_CLICK;
                        gest->key_code = event.code;
                        gest->long_press_time = tms_start - cache_press_time;
                        return;
                    }
                    long_press_state = true;
                    long_press_code = event.code;
                    gest->new_action = false;
                    return;
                }
                if(long_press_state && event.code == long_press_code){
                    click_key_code = -1;
                    click_time = 0;
                    slide_key_code = -1;
                    slide_time = 0;
                    long_press_state = false;
                    long_press_code = -1;
                    gest->new_action = true;
                    gest->key_code = event.code;
                    gest->action = ACTION_LONG_CLICK;
                    gest->long_press_time = tms_start - cache_press_time;
                    return;
                }
                if(HAS_SLIDE_EVENT && slide_event(gest,event.code,event.value,tms_start)){
                    RKLogi("===========slide============== \n");
                    click_key_code = -1;
                    click_time = 0;
                    click_key_count = 0;
                    return;
                }else if(click_event(gest,event.code,event.value,tms_start)){
                    RKLogi("===========click============== \n");
                    return;
                }
            }
        }
    }

bool click_event(struct gesture * p_gesture,int key_code,int value,unsigned long event_time){//处理单击和双击事件
   if(value != 1){
        RKLogi("click gesture ignore key event up \n");
        return false;
   }
   if(click_time == 0){//重新开始click事件分析
        if(key_code == NONE_KEY_CODE){
            return false;
        }
        click_key_code = key_code;
        click_key_count = 1;
        click_time = event_time;
        cache_press_time = event_time;
        return false;

   }else{//已经有单击事件缓存了,
        RKLogi("event_time:[%ld],click_time:[%ld]",event_time,click_time);
        if(click_key_code == key_code){//如果超时则上抛单击事件并且替换当前缓存事件为新的事件
            if(event_time - click_time > MAX_DOUBLE_CLICK_DELAY_TIME){
                RKLogi("last cache single click event: code is %d\n",click_key_code);
                p_gesture->action = ACTION_SINGLE_CLICK;
                p_gesture->new_action = true;
                p_gesture->key_code = click_key_code;
                click_key_code = key_code;
                click_key_count = 1;
                click_time = event_time;
                return true;
            }else{//未超时 且key_code与上一次一致,需上抛双击事件
                RKLogi("double click event happened: code is %d\n",click_key_code);
                click_key_code = -1;
                click_key_count = 0;
                click_time = 0;
                p_gesture->action = ACTION_DOUBLE_CLICK;
                p_gesture->new_action = true;
                p_gesture->key_code = key_code;
                return true;
            }

        }else{//新事件与缓存事件的key code不一样,上抛缓存事件
            if(key_code == NONE_KEY_CODE){
                if(event_time - click_time > MAX_DOUBLE_CLICK_DELAY_TIME){
                    RKLogi("last cache single click event: code is %d\n",click_key_code);
                    p_gesture->action = ACTION_SINGLE_CLICK;
                    p_gesture->new_action = true;
                    p_gesture->key_code = click_key_code;
                    click_key_code = -1;
                    click_key_count = 0;
                    click_time = 0;
                    return true;
                }
            }
        }
   }
   return false;
}

bool slide_event(struct gesture * p_gesture,int key_code,int value,unsigned long event_time){
    if(value !=1){
        RKLogi("slide gesture ignore key up event\n");
        return false;
    }

    if(slide_time == 0){//首次进入
        if(key_code == NONE_KEY_CODE){//首次进入需要忽略无效的事件
            p_gesture->new_action = false;
            return false;
        }
        slide_key_code = key_code;
        slide_time = event_time;
    }else{
        if(key_code == NONE_KEY_CODE){
            if(event_time - slide_time > MAX_SLIDE_DELAY_TIME){//无效的key 超时重置
                p_gesture->new_action = false;
                slide_key_code = -1;
                slide_time = 0;
                return false;
            }else{//无效的key但是未超时继续等待
                RKLogi("wait for slide event code is %d\n",slide_key_code);
                p_gesture->new_action = false;
                return false;
            }
        }else{
            if(event_time - slide_time > MAX_SLIDE_DELAY_TIME){//超时 仅仅替换slide键及时间
                slide_key_code = key_code;
                slide_time = event_time;
                p_gesture->new_action = false;
                return false;
            }
            int slide_key_subscript = slide_key_code + MINUS_KEY_VALUE;
            int prev_key_subscript = slide_key_subscript -1;
            int next_key_subscript = slide_key_subscript +1;
            int prev_key_code,next_key_code;
            if(prev_key_subscript == -1){
                prev_key_subscript = KEY_COUNTS -1;
            }
            if(next_key_subscript == KEY_COUNTS){
                next_key_subscript = 0;
            }
            prev_key_code = KEY_ARRAYS[prev_key_subscript];
            next_key_code = KEY_ARRAYS[next_key_subscript];
            if(key_code == prev_key_code){
                RKLogi("slide down event happened\n");
                slide_key_code = key_code;
                slide_time = event_time;
                p_gesture->action = ACTION_SLIDE;
                p_gesture->slide_value = -1;
                p_gesture->new_action = true;
                return true;
            }else if(key_code == next_key_code){
                RKLogi("slide up event happened\n");
                slide_key_code = key_code;
                slide_time = event_time;
                p_gesture->action = ACTION_SLIDE;
                p_gesture->slide_value = 1;
                p_gesture->new_action = true;
                return true;
            }else {//不是滑动事件 也不是NONE_KEY_CODE 缓存最新的键值和时间
                slide_key_code = key_code;
                slide_time = event_time;
                p_gesture->new_action = false;
                return false;
            }
        }
    }
    return false;

}

void daemon_exec(const char *command) {
    pid_t pid = fork();
    if(pid == 0) {
        const char *args[] = {
            "sh", "-c", command, NULL
        };


        if(!conf.verbose) {
            int null_fd = open("/dev/null", O_RDWR, 0);
            if(null_fd > 0) {
                dup2(null_fd, STDIN_FILENO);
                dup2(null_fd, STDOUT_FILENO);
                dup2(null_fd, STDERR_FILENO);
                if(null_fd > STDERR_FILENO) {
                    close(null_fd);
                }
            }
        }

        signal(SIGINT,  SIG_IGN);
        signal(SIGQUIT, SIG_IGN);

        execv("/bin/sh", (char *const *) args);
        _exit(127);
    } else if(pid < 0) {
        perror(PROGRAM": fork()");
    }

    return;
}

void daemon_clean() {
    int i, j;

    if(conf.verbose) {
        fprintf(stderr, "\n"PROGRAM": Exiting...\n");
    }

    for(i=0; i<key_event_n; i++) {
        free((void*) key_events[i].code);
        for(j=0; j<key_events[i].modifier_n; j++) {
            free((void*) key_events[i].modifiers[j]);
        }
        free((void*) key_events[i].exec);
    }
    key_event_n = 0;

    for(i=0; i<idle_event_n; i++) {
        free((void*) idle_events[i].exec);
    }
    idle_event_n = 0;

    for(i=0; i<switch_event_n; i++) {
        free((void*) switch_events[i].code);
        free((void*) switch_events[i].exec);
    }
    switch_event_n = 0;

    for(i=0; i < MAX_LISTENER && conf.listen[i] != NULL; i++) {
        free((void*) conf.listen[i]);
        conf.listen[i] = NULL;
        if(conf.listen_fd[i]) {
            close(conf.listen_fd[i]);
        }
    }

    _exit(EXIT_SUCCESS);
}

void daemon_print_help() {
    printf("Usage:\n\n"
            "    "PROGRAM" "
            "[ [ --monitor | --list | --help | --version ] |\n"
            "                         "
            "[--config=FILE] [--verbose] [--no-daemon] ]\n"
            "\n"
            "Available Options:\n"
            "\n"
            "    -m, --monitor       Start in monitoring mode\n"
            "    -l, --list          List all input devices and quit\n"
            "    -c, --config FILE   Use specified config file\n"
            "    -v, --verbose       Verbose output\n"
            "    -D, --no-daemon     Don't run in background\n"
            "\n"
            "    -h, --help          Show this help and quit\n"
            "    -V, --version       Show version number and quit\n"
            "\n"
    );
    exit(EXIT_SUCCESS);
}

void daemon_print_version() {
    printf(PROGRAM" "VERSION"\n");
    exit(EXIT_SUCCESS);
}

int parse_gpio()
{
    FILE *fp;
    char line[1024];
    int j = 0;
    char ch = ':';
    char *gpio;
    if((fp = fopen(sw_config,"r")) != NULL)
    {
        while(!feof(fp))
        {
            fgets(line, sizeof(line), fp);
            if(line[strlen(line)-1] == '\n')
            {
                line[strlen(line)-1] = '\0';
            }
            if(line[0] == '\n' || line[0] == '\0' || line[0] == '\r')
            {
                continue;
            }
            else
            {
                gpio = strchr(line, ch);
                if(gpio != NULL)
                {
                    *gpio = '\0';
                    gpio++;
                    while((*gpio != '/') && (*gpio != '\0'))
                    {
                        gpio++;
                    }
                    if(*gpio == '/')
                    {
                        int fd = open(gpio, O_RDONLY|O_NONBLOCK);
                        if(fd < 0)
                        {
                            RKLoge("open failed:%s\n",strerror(errno));
                            continue;
                        }
                        else
                        {
                            unsigned char keyvalue;
                            if(read(fd, &keyvalue, sizeof(keyvalue)) > 0)
                            {
                                sw_keycode[j] = atoi(line);
                                if(keyvalue == '0')
                                {
                                    sw_keyvalue[j] = 0;
                                }
                                else if(keyvalue == '1')
                                {
                                    sw_keyvalue[j] = 1;
                                }
                                else
                                {
                                    RKLoge("wrong value\n");
                                    continue;
                                }
                                j++;
                            }
                            else
                            {
                                RKLoge(" read failed\n");
                            }

                        }
                    }
                }
            }
        }
    }
    return j;
}
bool init_input_key(bool has_slide_event,int select_timeout,int double_click_timeout,int slide_timeout) {
//    printf("init_input_key is called\n");
    HAS_SLIDE_EVENT = has_slide_event;
    MAX_DOUBLE_CLICK_DELAY_TIME =  double_click_timeout;
    MAX_SLIDE_DELAY_TIME = slide_timeout;
    TIME_OUT = select_timeout;
    daemon_init();



       /*if (DbusEnvInit(NULL)) {
           fprintf(stderr, "input-event dbus env init failed.\n");
           return EXIT_FAILURE;
       }*/
   	if (ChargeModeDetect()) {
           printf( "input-event charge mode detect failed.\n");
           return false;
   	}
   	atexit(daemon_clean);
    signal(SIGTERM, daemon_clean);
    signal(SIGINT,  daemon_clean);

    conf.daemon = 0;
       if(conf.monitor) {
           RKLogi("input_open_all_listener\n");
           input_open_all_listener();
       } else {
           RKLogi("config_parse_file\n");
           config_parse_file();
       }

//        signal(SIGCHLD, SIG_IGN);
           FD_ZERO(&initial_fdset);
           memset(sw_keycode, -1, sizeof(sw_keycode));
           for(i=0; i < MAX_LISTENER && conf.listen[i] != NULL; i++) {
               conf.listen_fd[i] = open(conf.listen[i], O_RDONLY);

               if(conf.listen_fd[i] < 0) {
                   RKLogi( ": open(%s): %s\n",
                       conf.listen[i]);
                    return false;
               }
               FD_SET(conf.listen_fd[i], &initial_fdset);
           }
           sw_keynumber = parse_gpio();
           fd_len = i;
            if(fd_len == 0) {
               RKLogi(": no listener found!\n");
               return false;
           }

           if(conf.verbose) {
               RKLogi(" Start listening on %d devices...\n", fd_len);
           }

           if(conf.monitor) {
               RKLogi(PROGRAM": Monitoring mode started. Press CTRL+C to abort.\n");
           } else if(conf.daemon) {
               if(daemon(1, conf.verbose) < 0) {
                   perror(PROGRAM": daemon()");
                    return false;
               }
           }
    //printf("conf.monitor is %d conf.file is %s \n",conf.monitor,conf.configfile);
    return  true;
}