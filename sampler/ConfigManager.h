/*
 * PSAMP Reference Implementation
 *
 * ConfigManager.h
 *
 * Manages a config file
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <iostream>
#include <fstream>
#include <string>

// Singleton class
class ConfigManager
{
private:
  int numPaths;
  
public:
  ConfigManager(const char *fileName)
  {
    ReloadConfig(fileName);
  }

  void ReloadConfig(const char *fileName)
  {
    std::ifstream f(fileName);
    std::string l;

    while (!f.eof())
    {
    }
  }
        
};

#endif
