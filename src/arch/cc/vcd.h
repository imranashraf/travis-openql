/**
 * @file   vcd.h
 * @date   20190515
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  generate Value Change Dump file for GTKWave viewer
 * @remark based on https://github.com/SanDisk-Open-Source/pyvcd/tree/master/vcd
 */

#ifndef _VCD_H
#define _VCD_H

#include <string>
#include <sstream>
#include <map>

class Vcd {
public:
    typedef enum { VT_INT, VT_STRING } tVarType;
    typedef enum { ST_MODULE } tScopeType;

public:
    void start();
    void scope(tScopeType type, std::string name);
    int registerVar(std::string name, tVarType type, tScopeType scope=ST_MODULE);
    void upscope();
    void change(int var, int timestamp, std::string value);
    void change(int var, int timestamp, int value);
    void finish();
    std::string getVcd();

private:
    typedef struct {
        int intVal;
        std::string strVal;
    } tValue;
    typedef std::map<int, tValue> tVarChangeMap;        // map variable 'id' to 'tValue'
    typedef std::map<int, tVarChangeMap> tTimestampMap; // map 'timestamp' to variables

private:
    int lastId;
    tTimestampMap timestampMap;
    std::stringstream vcd;
};

#endif // ndef _VCD_H
