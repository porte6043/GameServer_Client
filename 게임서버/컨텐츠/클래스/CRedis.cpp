#include "CRedis.h"
#include "공용 라이브러리/CTlsPool.h"


CTlsPool<cpp_redis::client> RedisPool(1, 0);

CRedis::CRedis()
{
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);

	client = RedisPool.Alloc();
	if (!client->is_connected())
		client->connect();
}

CRedis::~CRedis()
{
	RedisPool.Free(client);
}

optional<string> CRedis::get_as_string(const string& key)
{
	auto future = client->get(key);
	client->sync_commit();

	auto reply = future.get();
	if (reply.is_null())
		return std::nullopt;

	return reply.as_string();
}

optional<long long> CRedis::get_as_integer(const string& key)
{
	auto future = client->get(key);
	client->sync_commit();

	auto reply = future.get();
	if (reply.is_null())
		return std::nullopt;
	
	return reply.as_integer();
}

optional<string> CRedis::hget_as_string(const string& key, const string& field)
{
	auto future = client->hget(key, field);
	client->sync_commit();

	auto reply = future.get();
	if (reply.is_null())
		return std::nullopt;

	return reply.as_string();
}

optional<long long> CRedis::hget_as_integer(const string& key, const string& field)
{
	auto future = client->hget(key, field);
	client->sync_commit();

	auto reply = future.get();
	if (reply.is_null())
		return std::nullopt;

	return reply.as_integer();
}

optional<vector<st_HashEntry>> CRedis::hgetall(const string& key)
{
	auto future = client->hgetall(key);
	client->sync_commit();

	auto reply = future.get();
	if (reply.is_null())
		return std::nullopt;

	auto& List = reply.as_array();
	vector<st_HashEntry> answer(List.size() / 2);
	for (int idx = 0; idx < answer.size(); ++idx)
	{
		answer[idx].field = List[idx * 2].as_string();
		answer[idx].value = List[idx * 2 + 1].as_string();
	}

	return answer;
}

bool CRedis::set(const string& key, const string& value)
{
	auto future = client->set(key, value);
	client->sync_commit();

	if (future.get().ko())
		return false;

	return true;
}

bool CRedis::setex(const string& key, long long seconds, const string& value)
{
	auto future = client->setex(key, seconds, value);
	client->sync_commit();
	
	return future.get().ok();
}

bool CRedis::setnx(const string& key, const string& value)
{
	auto future = client->setnx(key, value);
	client->sync_commit();

	return static_cast<bool>(future.get().as_integer());
}

bool CRedis::hset(const string& key, const string& field, const string& value)
{
	auto future = client->hset(key, field, value);
	client->sync_commit();

	if (future.get().ko())
		return false;

	return true;
}

bool CRedis::hsetnx(const string& key, const string& field, const string& value)
{
	auto future = client->hsetnx(key, field, value);
	client->sync_commit();
	
	return static_cast<bool>(future.get().as_integer());
}

vector<string> CRedis::lrange(const string& key, int start, int stop)
{
	auto future = client->lrange(key, start, stop);
	client->sync_commit();

	auto reply = future.get();
	auto& List = reply.as_array();
	vector<string> answer(List.size());
	for (int idx = 0; idx < List.size(); ++idx)
		answer[idx] = List[idx].as_string();

	return answer;
}

bool CRedis::rpush(const string& key, const vector<string>& values)
{
	auto future = client->rpush(key, values);
	client->sync_commit();

	return future.get().ok();
}

bool CRedis::ltrim(const string& key, int start, int stop)
{
	auto future = client->ltrim(key, start, stop);
	client->sync_commit();

	return future.get().ok();
}

void CRedis::del(const vector<string>& key)
{
	client->del(key);
	client->sync_commit();
	return;
}

bool CRedis::exists(const vector<std::string>& keys)
{
	auto future = client->exists(keys);
	client->sync_commit();
	
	return static_cast<bool>(future.get().as_integer());
}