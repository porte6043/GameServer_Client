#ifndef __LOBBYSERVER_CREDIS_H__
#define __LOBBYSERVER_CREDIS_H__
// Redis
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "redis/cpp_redis.lib")
#pragma comment (lib, "redis/tacopie.lib")

// std
#include <optional>
#include <string>
#include <vector>
using std::optional;
using std::string;
using std::vector;



struct st_HashEntry
{
	string field;
	string value;
};


class CRedis
{
	cpp_redis::client* client;

	CRedis(CRedis& other) = delete;
	CRedis(const CRedis& other) = delete;
	CRedis(const CRedis&& other) = delete;
	CRedis& operator=(const CRedis& other) = delete;
	
public:
	CRedis();
	~CRedis();

	optional<string> get_as_string(const string& key);

	optional<long long> get_as_integer(const string& key);

	optional<string> hget_as_string(const string& key, const string& field);

	optional<long long> hget_as_integer(const string& key, const string& field);

	optional<vector<st_HashEntry>> hgetall(const string& key);

	bool set(const string& key, const string& value);

	bool setex(const string& key, long long seconds, const string& value);

	bool setnx(const string& key, const string& value);

	bool hset(const string& key, const string& field, const string& value);

	bool hsetnx(const string& key, const string& field, const string& value);

	optional<vector<string>> lrange(const string& key, int start, int stop);

	bool rpush(const string& key, const vector<string>& values);

	bool ltrim(const string& key, int start, int stop);

	void del(const vector<string>& key);

	bool exists(const vector<std::string>& keys);
};

#endif