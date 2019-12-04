#include <fd.h>

#include <argp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define MAX_CONNECTIONS 2
#define MAX_EVENTS 10
#define MAX_READ_BUFFER_SIZE 32

#define LOG(level, fmt, ...) \
    printf("["level"] " fmt "\n", ##__VA_ARGS__)

#define LOGE(fmt, ...) \
    LOG("Error", fmt, ##__VA_ARGS__)

#define LOGI(fmt, ...) \
    LOG("Info", fmt, ##__VA_ARGS__)

#define LOGD(fmt, ...) \
    LOG("DEBUG", fmt, ##__VA_ARGS__)

struct test_parameters {
    char *host;
    int port;
};

struct connection {
    int socket;
    struct sockaddr_in socket_address;
};

struct test_data {
    struct syncer *syncer;
    struct test_parameters parameters;
    struct connection connection;
    struct fd_handler *handler;
    eloop_id_t socket_handler_id;
};

struct client_data {
    int socket;
    eloop_id_t handle_id;
    struct test_data *test_data;
};

static bool server_stop = false;

static char cmdline_doc[] = "-u URL -p PORT";

static struct argp_option cmdline_options[] = {
    { "url", 'u', "URL", 0, "Host URL", 0 },
    { "port", 'p', "PORT", 0, "Host port", 0 },
    { 0 }
};

static error_t parse_cmdline_options(int key,
                                     char *arg,
                                     struct argp_state *state)
{
    struct test_parameters *parameters = state->input;
    static int mandatory = 0;

    switch (key)
    {
        case 'u':
            parameters->host = arg;
            mandatory++;
            break;
        case 'p':
            parameters->port = atoi(arg);
            mandatory++;
            break;
        case ARGP_KEY_END:
            if (mandatory < 2)
                argp_usage(state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {
    .options = cmdline_options,
    .parser = parse_cmdline_options,
    .args_doc = cmdline_doc
};

static void socket_set_address(struct sockaddr_in *addr,
                               char *host, int port)
{
    addr->sin_family = AF_INET;
    addr->sin_port   = htons(port);
    inet_pton(AF_INET, host, &addr->sin_addr);
}

static int socket_set_nonblocking(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0)
        return -1;

    int res = fcntl(socket, F_SETFL, flags |= O_NONBLOCK);
    if (res < 0)
        return -1;

    return 0;
}

static void display_incomming_message(int client_fd)
{
    char read_buffer[MAX_READ_BUFFER_SIZE + 1];
    int res, part = 1;

    do {
        res = read(client_fd, read_buffer, MAX_READ_BUFFER_SIZE);
        if (res < 0) {
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
                LOGE("Error reading incomming data: %s (%d)",
                     strerror(errno), errno);
            break;
        } else if (res == 0) {
            break;
        }

        read_buffer[res] = '\0';
        LOGI("    part %d: %s", part, read_buffer);
        part++;
    } while (res > 0);
}

static void server_handle_incomming(void *ctx, uint32_t event_mask)
{
    struct client_data *client_data = ctx;
    struct test_data *test_data = client_data->test_data;

    if (event_mask & ELOOP_FD_EVENT_ERROR) {
        LOGE("Error occured");
    } else if (event_mask & ELOOP_FD_EVENT_HUP) {
        LOGI("Client hung up");
        fd_handler_remove_fd(test_data->handler,
                             client_data->handle_id);
        close(client_data->socket);
        free(client_data);
    } else if (event_mask & ELOOP_FD_EVENT_READ) {
        LOGI("Data is ready on socket %d", client_data->socket);
        display_incomming_message(client_data->socket);
    } else if (event_mask & ELOOP_FD_EVENT_WRITE) {
        LOGI("Write event on socket");
    }
}

static int accept_client(struct test_data *test_data)
{
    int client_fd;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    char ip_str[INET_ADDRSTRLEN];

    client_fd = accept(test_data->connection.socket, &addr, &len);
    if (client_fd < 0) {
        LOGE("Failed accepting new connection: %s (%d)",
             strerror(errno), errno);
        return -1;
    }

    if (socket_set_nonblocking(client_fd)) {
        LOGE("Failed setting new connection as non-blocking: %s (%d)",
             strerror(errno), errno);
        close(client_fd);
        return -1;
    }

    inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);
    LOGI("Connection attempt from %s:%d", ip_str, addr.sin_port);

    return client_fd;
}

static void server_handle_new_connection(void *ctx, uint32_t event_mask)
{
    struct test_data *test_data = ctx;
    struct client_data *client_data;

    LOGD("New connection pending on socket fd %d",
         test_data->connection.socket);

    client_data = malloc(sizeof(*client_data));
    if (!client_data) {
        LOGE("Failed to allocate client data");
        return;
    }

    client_data->test_data = test_data;

    client_data->socket = accept_client(test_data);
    if (client_data->socket < 0) {
        LOGE("Failed to accept client");
        free(client_data);
        return;
    }

    client_data->handle_id = fd_handler_add_fd(test_data->handler,
                                               client_data->socket,
                                               server_handle_incomming,
                                               client_data);
    if (client_data->handle_id == INVALID_ID) {
        LOGE("Failed adding connection to socket handler");
        close(client_data->socket);
        free(client_data);
        return;
    }

    LOGI("Connection accepted");
}

static int server_init(struct test_data *test_data)
{
    int res;

    test_data->connection.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (test_data->connection.socket <= 0)
        return -1;

    socket_set_address(&test_data->connection.socket_address,
                       test_data->parameters.host,
                       test_data->parameters.port);

    res = bind(test_data->connection.socket,
               &test_data->connection.socket_address,
               sizeof(test_data->connection.socket_address));
    if (res)
        return -1;

    res = socket_set_nonblocking(test_data->connection.socket);
    if (res)
        return -1;

    res = listen(test_data->connection.socket, MAX_CONNECTIONS);
    if (res)
        return -1;

    test_data->socket_handler_id =
        fd_handler_add_fd(test_data->handler,
                          test_data->connection.socket,
                          server_handle_new_connection,
                          test_data);
    if (test_data->socket_handler_id == INVALID_ID)
        return -1;

    return 0;
}

void server_clear(struct test_data *test_data)
{
    fd_handler_remove_fd(test_data->handler, test_data->socket_handler_id);
    close(test_data->connection.socket);
}

void terminate_server(int signum)
{
    LOGI("Terminating service");

    server_stop = true;
}

static void register_sigterm_handler()
{
    struct sigaction action;

    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate_server;
    sigaction(SIGINT, &action, NULL);
}

static int test_init(struct test_data *test_data)
{
    test_data->syncer = syncer_new();
    if (!test_data->syncer)
        return -1; 

    test_data->handler = fd_handler_new(test_data->syncer,
                                        MAX_EVENTS);
    if (!test_data->handler) {
        LOGE("Failed creating fd handler");
        return -1;
    }

    if (server_init(test_data)) {
        LOGE("Failed initialilzing server");
        return -1;
    }

    register_sigterm_handler();

    LOGI("Server initialized successfully");

    return 0;
}

static void test_clear(struct test_data *test_data)
{
    server_clear(test_data);
    fd_handler_delete(test_data->handler);
    syncer_delete(test_data->syncer);
}

int main(int argc, char *argv[])
{
    struct test_data test_data = {0};

    argp_parse(&argp, argc, argv, 0, 0, &test_data.parameters);

    if (test_init(&test_data)) {
        return 1;
    }

    while (!server_stop) {
        fd_handler_handle_events(test_data.handler);
        syncer_process_queue(test_data.syncer);
    }

    test_clear(&test_data);

    return 0;
}
