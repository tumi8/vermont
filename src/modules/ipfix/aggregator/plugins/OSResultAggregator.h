/*
 * Vermont Aggregator Subsystem
 * Copyright (C) 2009 Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifdef PLUGIN_SUPPORT_ENABLED
#ifndef OSANALYSER_H_
#define OSANALYSER_H_

#include "OSDetail.h"
#include "common/Thread.h"
#include <fstream>
#include <boost/unordered_map.hpp>
#include <list>

typedef boost::unordered_map< uint32_t, std::list<OSDetail> > os_result_map_t;
typedef boost::unordered_map< uint32_t, uint32_t > expiry_map_t;


class OSResultAggregator{

public:
    enum e_mode {
        Console = 1,
        File
    };

    OSResultAggregator();
    ~OSResultAggregator();
    void insertResult(uint32_t ip, OSDetail details);
    void removeResult(uint32_t ip);
    void analyseResults(uint32_t ip);
    void exporterThread();
    void startExporterThread();

    void setInterval(uint32_t interval);
    void setOutputMode(e_mode mode);
    void setOuputFile(std::string file);

private:
    os_result_map_t results;
    expiry_map_t expired;
    Thread thread;
    bool exitFlag;
    e_mode outputMode;
    std::string aggregationFile;
    std::ofstream filestream;
    uint32_t pollInterval; /**< polling interval in milliseconds */

    static void* threadWrapper(void* instance);
};

#endif
#endif

