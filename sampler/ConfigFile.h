/*
 * PSAMP Reference Implementation
 *
 * ConfigFile.h
 *
 * Structures that describe a config file (Sections, Keys, ...)
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

class ConfigKey
{
public:
  enum KeyType {
    CFG_STRING = 1;
    CFG_BOOL,
    CFG_INT,
    CFG_DOUBLE,
    CFG_BINARY
  };
  
  ConfigKey(char *name, char *value);
  ConfigKey(char *name, bool value);
  ConfigKey(char *name, int value);
  ConfigKey(char *name, double value);
  ConfigKey(char *name, void *value, int length);

  KeyType getType() const
  {
    return m_type;
  }
    
private:
  KeyType m_type;
};

#endif