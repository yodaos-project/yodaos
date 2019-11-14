#ifndef INPUT_EVENT_DAEMON_H
#define INPUT_EVENT_DAEMON_H

#define PROGRAM  "input-event-daemon"
#define VERSION  "0.1.3"

#define MAX_MODIFIERS      4
#define MAX_LISTENER       32
#define MAX_EVENTS         64

#define IDLE_RESET         0x00

#define test_bit(array, bit) ((array)[(bit)/8] & (1 << ((bit)%8)))

#define ACTION_ORDINARY_EVENT  0
#define ACTION_SINGLE_CLICK 1
#define ACTION_DOUBLE_CLICK 2
#define ACTION_LONG_CLICK 3
#define ACTION_SLIDE 4

#define NONE_KEY_CODE -1024
#define KEY_COUNTS  8




#define MAX_LEN 5
/**
 * Global Configuration
 *
 */


struct gesture {
    int key_code;
    int action;
    int slide_value;
    int click_count;
    bool new_action;
    long long_press_time;
};

struct keyevent {
    //struct gesture gest[MAX_LEN];
    //int counts;
    int action;
    int key_code;
    long key_time; // deprecated
    struct timeval key_timeval;
    int value;
    bool new_action;
};


struct {
    const char      *configfile;

    unsigned char   monitor;
    unsigned char   verbose;
    unsigned char   daemon;

    unsigned long   min_timeout;

    const char      *listen[MAX_LISTENER];
    int             listen_fd[MAX_LISTENER];
} conf;

/**
 * Event Structs
 *
 */

typedef struct key_event {
    const char *code;
    const char *modifiers[MAX_MODIFIERS];
    size_t     modifier_n;
    const char *exec;
} key_event_t;


typedef struct idle_event {
    unsigned long timeout;
    const char  *exec;
} idle_event_t;

typedef struct switch_event {
    const char *code;
    signed int value;
    const char *exec;
} switch_event_t;

/**
 * Event Lists
 *
 */

key_event_t       key_events[MAX_EVENTS];
idle_event_t     idle_events[MAX_EVENTS];
switch_event_t switch_events[MAX_EVENTS];

size_t    key_event_n = 0;
size_t   idle_event_n = 0;
size_t switch_event_n = 0;

/**
 * Functions
 *
 */

int
    key_event_compare(const key_event_t *a, const key_event_t *b);
 const char
    *key_event_name(unsigned int code);
 const char
    *key_event_modifier_name(const char* code);
key_event_t
    *key_event_parse(unsigned int code, int pressed, const char *src);


 int idle_event_compare(const idle_event_t *a, const idle_event_t *b);
 int idle_event_parse(unsigned long idle);


 int
    switch_event_compare(const switch_event_t *a, const switch_event_t *b);
 const char
    *switch_event_name(unsigned int code);
 switch_event_t
    *switch_event_parse(unsigned int code, int value, const char *src);


void        input_open_all_listener();
void        input_list_devices();
 void input_parse_event(struct input_event *event, const char *src);


void                config_parse_file();
 const char   *config_key_event(char *shortcut, char *exec);
 const char   *config_idle_event(char *timeout, char *exec);
 const char   *config_switch_event(char *switchcode, char *exec);
 unsigned int config_min_timeout(unsigned long a, unsigned long b);
 char         *config_trim_string(char *str);

void        daemon_init();
void daemon_start_listener(struct keyevent * down_up,struct gesture * gest);
 void daemon_exec(const char *command);
void        daemon_clean();
 void daemon_print_help();
 void daemon_print_version();
  bool click_event(struct gesture * p_gesture,int key_code,int value,unsigned long event_time);
  bool slide_event(struct gesture * p_gesture,int key_code,int value,unsigned long event_time);
bool init_input_key(bool has_slide_event,int select_time_out,int double_click_timeout,int slide_timeout);

#endif /* INPUT_EVENT_DAEMON_H */
