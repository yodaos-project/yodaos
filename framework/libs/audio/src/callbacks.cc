
#include "callbacks.hh"

void state_cb(pa_context* context, void* raw)
{
    Pulseaudio* pulse = (Pulseaudio*)raw;
    switch (pa_context_get_state(context))
    {
        case PA_CONTEXT_READY:
            pulse->state = CONNECTED;
            break;
        case PA_CONTEXT_FAILED:
            pulse->state = ERROR;
            break;
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_TERMINATED:
            break;
    }
}

void sink_list_cb(pa_context* UNUSED(c), const pa_sink_info* i, int eol, void* raw)
{
    if (eol != 0)
        return;

    std::list<Device>* sinks = (std::list<Device>*)raw;
    Device s(i);
    sinks->push_back(s);
}

void sink_input_list_cb(pa_context* UNUSED(c), const pa_sink_input_info* i, int eol, void* raw)
{
    if (eol != 0)
        return;

    std::list<Device>* sinks = (std::list<Device>*)raw;
    Device s(i);
    sinks->push_back(s);
}

void source_list_cb(pa_context* UNUSED(c), const pa_source_info* i, int eol, void* raw)
{
    if (eol != 0)
        return;

    std::list<Device>* sources = (std::list<Device>*)raw;
    Device s(i);
    sources->push_back(s);
}

void server_info_cb(pa_context* UNUSED(context), const pa_server_info* i, void* raw)
{
    ServerInfo* info = (ServerInfo*)raw;
    info->default_sink_name = i->default_sink_name;
    info->default_source_name = i->default_source_name;
}

void success_cb(pa_context* UNUSED(context), int UNUSED(success), void* UNUSED(raw)) {}
