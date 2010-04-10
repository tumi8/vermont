/*
* Vermont PCAP Exporter
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

#include "PCAPExporterFifo.h"

#include "modules/packet/Packet.h"
#include <unistd.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace bfs = boost::filesystem;

PCAPExporterFifo::PCAPExporterFifo(const std::string& logfile)
    : logFileName(logfile), fifoReaderCmd(""), fifoReaderPid(0), 
        dummy(NULL), sigKillTimeout(1)
{
}

PCAPExporterFifo::~PCAPExporterFifo()
{
}
void PCAPExporterFifo::setFifoReaderCmd(const std::string& cmd)
{
    fifoReaderCmd = cmd;
}

void PCAPExporterFifo::setSigKillTimeout(int s)
{
    if (s < 0) s *= -1;
    sigKillTimeout = s;
}

int PCAPExporterFifo::execCmd(std::string& cmd)
{
    //char *command[] = {"tcpdump","-nr", "-" , (char*)0}; //"-w", "/tmp/pcap.dump",(char*)0};
    char *command[64];
    char tmp[1024];
    if (strlen(cmd.c_str()) > 1023) {
        THROWEXCEPTION("Command too long");
    }
    strcpy(tmp, cmd.c_str());
    command[0] = strtok(tmp, " \t");
    int i = 0;
    //TODO:  handle arguments containg whitespaces
    while (command[i]) {
        i++;
        if (i == 63) {
            THROWEXCEPTION("Argument list too long");
        }
        command[i] = strtok(NULL, " \t");
    }
    command[++i] = (char*)0;

    int pid = fork();
    if (pid == 0) {
        close(STDIN_FILENO); //close stdin 
        if (dup(fd[0]) == -1) { 
            THROWEXCEPTION("dup(fd[0]) failed");
        }
        if (logFileName != "") {
            time_t rawtime;
            struct tm * timeinfo;
            char buffer [20];
            time (&rawtime);
            timeinfo = localtime (&rawtime);
            if (strftime(buffer, 20, "%Y-%m-%d_%H:%M:%S", timeinfo) == 0) {
                THROWEXCEPTION("strftime failed!");
            }

            std::string logfile = logFileName + "_" + buffer + ".log";
            if (freopen (logfile.c_str(),"w",stdout) == NULL) {
                THROWEXCEPTION("Could not reopen stdout!");
            }
            close(STDERR_FILENO); //redirect stderr to stdout
            if (dup(STDOUT_FILENO) == -1) {
                THROWEXCEPTION("Could no redirect stderr!");
            }
            /*
            if (setvbuf(stderr, NULL, _IONBF, 0)) {
                THROWEXCEPTION("setvbuf failed");
            }
            if (setvbuf(stdout, NULL, _IONBF, 0)) {
                THROWEXCEPTION("setvbuf failed");
            }
            */
        }
        if (execvp(command[0], command)<0) {
            THROWEXCEPTION("Failed to execute command: %s", cmd.c_str());
        }
    } else if (pid > 0) {
        return pid;
    } else { 
        THROWEXCEPTION("fork() failed");
    }
    return -1;
}

void PCAPExporterFifo::performStart()
{
    char errbuf[PCAP_ERRBUF_SIZE];
    dummy = pcap_open_dead(link_type, snaplen);
    if (!dummy) {
        THROWEXCEPTION("Could not open dummy device: %s", errbuf);
    }
    if (pipe(fd)) {
        THROWEXCEPTION("pipe() command failed");
    }
    FILE *f = fdopen(fd[1], "w");
    if (! f) {
        THROWEXCEPTION("fdopen failed");
    }
    dumper = pcap_dump_fopen(dummy, f);
    if (!dumper) {
        THROWEXCEPTION("Could not open dump file: %s", errbuf);
    }
    if (fifoReaderCmd != "") {
        fifoReaderPid = execCmd(fifoReaderCmd);
    }

	msg(MSG_INFO, "Started PCAPExporterFifo with the following parameters:");
    if (fifoReaderCmd != ""){
	    msg(MSG_INFO, "  - fifoReaderCmd = %s", fifoReaderCmd.c_str());
	    msg(MSG_INFO, "  - fifoReaderPid = %d", fifoReaderPid);
    } else {
        msg(MSG_ERROR, "Warning: No fifoReaderCmd specified!");
    }
    if (logFileName != "")
    	msg(MSG_INFO, "  - logfileBaseName = %s", logFileName.c_str());
    else
        msg(MSG_ERROR, "No Logfile specified - dumping to stdout!");
	msg(MSG_INFO, "  - sigKillTimeout = %d" , sigKillTimeout);
}
//void PCAPExporterFifo::postReconfiguration(){ msg(MSG_INFO, "postReconfiguration"); }
//void PCAPExporterFifo::onReconfiguration1(){ msg(MSG_INFO, "onReconfiguration1"); }
//void PCAPExporterFifo::onReconfiguration2(){ msg(MSG_INFO, "onReconfiguration1"); }
//void PCAPExporterFifo::preReconfiguration(){ msg(MSG_INFO, "preReconfiguration"); }

void PCAPExporterFifo::performShutdown()
{
    msg(MSG_DEBUG, "Performing shutdown for PID %d", fifoReaderPid);
    sleep(1);
    if (dumper) {
         if (-1 == pcap_dump_flush(dumper)) {
            msg(MSG_FATAL, "PCAPExporterFifo: Could not flush dump file");
         }

        pcap_dump_close(dumper);
        if (fifoReaderPid != 0 ) {
            kill_all(fifoReaderPid);
        }
    }
}

void PCAPExporterFifo::receive(Packet* packet) {
    writePCAP(packet);
}

void PCAPExporterFifo::kill_pid(int pid)
{
    int i = sigKillTimeout;
    std::ostringstream s;
    s << pid;
    std::string path = "/proc/" + s.str(); 
    kill(pid, 15);
    while(i--){
        msg(MSG_INFO, "waiting for pid %d, but no longer than %d seconds...", pid, i+1);
        if(!bfs::exists(path) ) return; //process no longer exists
        std::string filename = path + "/stat";
        std::ifstream myfile(filename.c_str());
        std::string line;
        if (myfile.is_open()) {
            getline (myfile,line);
            char *token = strtok (const_cast<char*>(line.c_str())," ");
            int count = 0;
            while (token != NULL) {
                if (count++ == 2) { //status
                    if(! strncmp(token, "Z", 1)){
                         msg(MSG_DEBUG, "Found Zombie");
                         return;
                    }
                    break;
                }
                token = strtok (NULL, " ");
            }
        }
        myfile.close();
        sleep(1);
    }
    kill(pid, 9);
}
bool PCAPExporterFifo::checkint(const char *my_string) {
    size_t stringlength = strlen(my_string);
    for (size_t j=0; j<stringlength; j++)
        if((int)my_string[j] < 0x30 || (int)my_string[j] > 0x39) 
            return false;

    return true;
}

void PCAPExporterFifo::kill_all(int ppid)
{
    bfs::path path("/proc/");
    if (!bfs::exists(path) || !bfs::is_directory(path)){
        THROWEXCEPTION("Directory /proc not found...");
    }

    std::string line;
    std::vector<int> my_pids;
    std::vector<int> my_ppids;
    my_ppids.push_back(ppid);
    bfs::directory_iterator dir_iterator = bfs::directory_iterator(path);
    bfs::directory_iterator end_itr;
    while (dir_iterator != end_itr) {
        if (bfs::is_directory(dir_iterator->status())  && 
                this->checkint(dir_iterator->leaf().c_str()))
        {
            std::string filename = dir_iterator->path().file_string() + "/stat";
            std::ifstream myfile(filename.c_str());
            if (myfile.is_open()) {
                getline (myfile,line);
                char *token = strtok (const_cast<char*>(line.c_str())," ");
                int count = 0;
                while (token != NULL) {
                    if (count++ == 3) { // field of parent pid
                        for (std::vector<int>::iterator it = my_ppids.begin(); it != my_ppids.end(); it++) {
                            if (atoi(token) == *it) {
                                msg(MSG_DEBUG, "Pid %s  is a child of %d", dir_iterator->leaf().c_str(), *it );
                                my_pids.push_back(boost::lexical_cast<int>(dir_iterator->leaf()));
                                my_ppids.push_back(boost::lexical_cast<int>(dir_iterator->leaf()));
                                break;
                            }
                        }
                    }
                    token = strtok (NULL, " ");
                }
                myfile.close();
            }
        }
        dir_iterator++;
    }
    my_pids.push_back(ppid);
    for (std::vector<int>::iterator it = my_pids.begin(); it != my_pids.end(); it++) {
        msg(MSG_DEBUG, "Killing Pid %d", *it);
        kill_pid(*it);
    }
    return;
}
