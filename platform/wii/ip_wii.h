#ifndef IP_WII_H
#define IP_WII_H

#include "core/io/ip.h"

class IP_Wii : public IP
{
    GDCLASS(IP_Wii, IP);
    
    virtual IP_Address _resolve_hostname(const String &p_hostname, IP::Type p_type);

    static IP *_create_wii();

public:
    virtual void get_local_interfaces(Map<String, Interface_Info> *r_interfaces) const;

    static void make_default();

};

#endif