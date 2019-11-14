#ifndef DEVICE_H
#define DEVICE_H

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

enum device_type
{
    SOURCE,
    SINK,
    SINKINPUT
};
typedef enum device_type device_type_t;

/**
 * Class to store device (sink or source) related informations
 *
 * @see pa_sink_info
 * @see pa_source_info
 */
class Device
{
public:
    uint32_t index;
    int32_t sink;
    device_type_t type;
    std::string name;
    std::string description;
    pa_cvolume volume;
    pa_volume_t volume_avg;
    int volume_percent;
    bool mute;

    Device(const pa_source_info* i);
    Device(const pa_sink_info* i);
    Device(const pa_sink_input_info* i);

private:
    void setVolume(const pa_cvolume* v);
};

#endif
