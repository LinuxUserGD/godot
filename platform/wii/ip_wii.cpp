#include "ip_wii.h"

#include <network.h>

IP_Address IP_Wii::_resolve_hostname(const String &p_hostname, Type p_type) {
    if(p_type == IP::TYPE_IPV6)
    {
        ERR_PRINT("Wii does not support IPv6");
        return IP_Address();
    }

    hostent *ent = net_gethostbyname(p_hostname.utf8().get_data());

    if(!ent || ent->h_length == 0)
    {
        ERR_PRINT("Failed to resolve \"" + p_hostname + "\"");
        return IP_Address();
    }

    IP_Address ret;
    ret.set_ipv4(reinterpret_cast<const uint8_t *>(ent->h_addr_list[0]));
    return ret;
}

void IP_Wii::get_local_interfaces(Map<String, Interface_Info> *r_interfaces) const
{
    // TODO: what the fuck counts as a network interface

    Map<String, Interface_Info>::Element *E = r_interfaces->find("wii");
    if(!E) {
        Interface_Info info;
        info.name = "wii";
        info.name_friendly = "wii";
        info.index = "1";
        E = r_interfaces->insert("wii", info);
        ERR_FAIL_COND(!E);
    }

    Interface_Info &info = E->get();
    IP_Address ip;
    u32 ip_num = net_gethostip();
    ip.set_ipv4(reinterpret_cast<const uint8_t *>(ip_num));
    info.ip_addresses.push_front(ip);
}

IP *IP_Wii::_create_wii()
{
    return memnew(IP_Wii);
}

void IP_Wii::make_default()
{
    _create = _create_wii;
}