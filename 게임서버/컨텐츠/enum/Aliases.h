#ifndef __LOBBYSERVER_Aliases_H__
#define __LOBBYSERVER_Aliases_H__
#include <unordered_map>
#include <memory>

class st_User;

using UID = unsigned long long;

using Sid = unsigned long long;

using Users = std::unordered_map<Sid, st_User*>;

#endif