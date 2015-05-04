#pragma once

using namespace std;
#include <string>
#include <vector>
#include <stdint.h>

vector<string>& srs_get_local_ipv4_ips();
int srs_get_log_level(std::string level);
string srs_get_peer_ip(int fd);
int64_t srs_get_system_time_ms();
void srs_update_system_time_ms();
