#ifndef __LOBBYSERVER_CHARACTERSEQ_H__
#define __LOBBYSERVER_CHARACTERSEQ_H__
#include <string>
using std::string;

struct st_CharacterSeq
{
	string			nickname;
	BYTE			seq;

	st_CharacterSeq(string Nickname, BYTE Seq) : nickname(Nickname), seq(Seq) {}
};


#endif
