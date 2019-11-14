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

#include "device.hh"

#include <cmath>
#include <string.h>

Device::Device(const pa_source_info* info)
{
    type = SOURCE;
    index = info->index;
    name = info->name;
    description = info->description;
    mute = info->mute == 1;
    setVolume(&(info->volume));
}

Device::Device(const pa_sink_info* info)
{
    type = SINK;
    index = info->index;
    name = info->name;
    description = info->description;
    mute = info->mute == 1;
    setVolume(&(info->volume));
}

Device::Device(const pa_sink_input_info* info)
{
    type = SINKINPUT;
    index = info->index;
    sink = info->sink;
    name = info->name;
    mute = info->mute == 1;
    setVolume(&(info->volume));
}

void Device::setVolume(const pa_cvolume* v)
{
    volume = *v;
    volume_avg = pa_cvolume_avg(v);
    volume_percent = (int)round((double)volume_avg * 100. / PA_VOLUME_NORM);
}
