#ifndef PULSEAUDIO_H
#define PULSEAUDIO_H

/*
 * Copyright (C) 2011 Clément Démoulins <clement@archivel.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pulse/pulseaudio.h>
#include <string>
#include <list>
#include <pulse/ext-stream-restore.h>
#include "device.hh"
#include "callbacks.hh"

class ServerInfo
{
public:
    std::string default_source_name;
    std::string default_sink_name;
};

enum state
{
    CONNECTING,
    CONNECTED,
    ERROR
};
typedef enum state state_t;

/**
 * Class to manipulate the pulseaudio server using the asynchronous C library.
 * When the constructor is called, a connection is established to a local pulseaudio server.
 * If the connection fail an exception is raised.
 */
class Pulseaudio
{
private:
    pa_mainloop* mainloop;
    pa_mainloop_api* mainloop_api;
    pa_proplist* paprop;
    pa_context* context;
    int retval;

    void iterate(pa_operation* op);

public:
    state_t state;
    pthread_mutex_t lock;
    /**
     * Initialize the connection to a local pulseaudio
     * @param client_name
     */
    Pulseaudio(std::string client_name);

    /**
     * Properly disconnect and free all the resources
     */
    ~Pulseaudio();

    /**
     * @return list of the available sinks
     */
    std::list<Device> get_sinks();

    /**
     * @return list of the available sinkinputs
     */
    std::list<Device> get_sinkinputs();

    /**
     * @return list of the available sources
     */
    std::list<Device> get_sources();

    /**
     * Get a specific sink
     * @param index index of the sink
     */
    Device get_sink(uint32_t index);

    /**
     * Get a specific sink
     * @param name name of the requested sink
     */
    Device get_sink(std::string name);

    /**
     * Get a specific source
     * @param index index of the source
     */
    Device get_source(uint32_t index);

    /**
     * Get a specific source
     * @param name name of the requested source
     */
    Device get_source(std::string name);

    /**
     * Get the default sink
     */
    Device get_default_sink();

    /**
     * Get the default source
     */
    Device get_default_source();

    /**
     * Set the volume to a new value for the specified device
     * @param device
     * @param new_volume
     */
    void set_volume(Device& device, pa_volume_t new_volume);

    /**
     * Change the mute state of a device
     * @param device
     * @param mute
     */
    void set_mute(Device& device, bool mute);
    /**
     * set volume at prepare st
     * @param type_name
     * @param new_volume
     */
    void set_volume_store(const char* type_name, pa_volume_t new_volume);
};

#endif
