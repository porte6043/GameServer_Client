#include "��Ʈ��ũ/CRoom2.h"
#include "��Ʈ��ũ/PqcsEvent.h"

CRoom2::CRoom2()
{
	InitializeCriticalSection(&CS_Room);
	InitializeCriticalSection(&CS_RoomMessageQ);
	RoomNo = 0;
	CalledChangeRoom = false;
	Frame = 50;
	FrameTime = 20;
	CurrentFrame = 0;
	Network = nullptr;
	MessageQ = &RoomMessageQ;
	WorkingQ = &RoomWorkingQ;
}
CRoom2::~CRoom2()
{
	DeleteCriticalSection(&CS_Room);
	DeleteCriticalSection(&CS_RoomMessageQ);
}

void CRoom2::SendMSG(DWORD64 SessionID, CPacket& packet)
{
	Network->SendMSG_PQCS(SessionID, packet);
	return;
}
void CRoom2::SendMSG_Disconnect(DWORD64 SessionID, CPacket& packet, DWORD dwSeconds)
{
	Network->SendMSG_PQCS_Disconnect(SessionID, packet, dwSeconds);
	return;
}
void CRoom2::Disconnect(DWORD64 SessionID)
{
	Network->Disconnect(SessionID);
	return;
}

void CRoom2::SuspendRecv(DWORD64 SessionID)
{
	Session* pSession = Network->SessionFind(SessionID);
	pSession->SuspendRecv = true;
}
bool CRoom2::ChangeRoom(DWORD64 SessionID, DWORD RoomNo, Param param)
{
	AcquireSRWLockShared(&Network->SRW_RoomMap);
	{
		auto iter = Network->RoomMap.find(RoomNo);
		if (iter == Network->RoomMap.end())
		{
			ReleaseSRWLockShared(&Network->SRW_RoomMap);
			return false;
		}
		// Map ���� ���� ó��
		if(!SessionMap.erase(SessionID))
		{
			ReleaseSRWLockShared(&Network->SRW_RoomMap);
			return false;
		}
		
		OnLeaveRoom(SessionID);
		CalledChangeRoom = true;

		// Room ���� ��û
		Session* pSession = Network->SessionFind(SessionID);
		pSession->RoomNo = RoomNo;
		CRoom2* pRoom = iter->second;
		EnterCriticalSection(&pRoom->CS_RoomMessageQ);
		{
			pRoom->MessageQ->push({ en_JOB_ROOM::ENTER, SessionID, param });
		}
		LeaveCriticalSection(&pRoom->CS_RoomMessageQ);
	}
	ReleaseSRWLockShared(&Network->SRW_RoomMap);
	return true;
}

void CRoom2::SetFrame(int frame)
{
	if (frame == 0)
	{
		Frame = 0;
		FrameTime = 0;
		return;
	}

	Frame = frame;
	FrameTime = static_cast<int>(1000 / frame);
	return;
}
long CRoom2::GetFrame()
{
	return InterlockedExchange(&CurrentFrame, 0);
}
DWORD CRoom2::GetRoomNo()
{
	return RoomNo;
}


void CRoom2::SetNetwork(CWanServerRoom2* network)
{
	Network = network;
	OnSetNetwork(network);
}
void CRoom2::QueueSwap()
{
	EnterCriticalSection(&CS_RoomMessageQ);
	{
		RoomQueue* temp = MessageQ;
		MessageQ = WorkingQ;
		WorkingQ = temp;
	}
	LeaveCriticalSection(&CS_RoomMessageQ);

	return;
}
void CRoom2::RoomUpdate()
{
	QueueSwap();

	// MessageQueue ó��
	while (1)
	{
		if (WorkingQ->empty())
			break;
		Job_RoomMessageQ Job = WorkingQ->front();
		WorkingQ->pop();

		switch (Job.Type)
		{
		case en_JOB_ROOM::ENTER:
		{
			Session* pSession = Network->SessionFind(Job.SessionID);

			// ���� �׷쿡�� �޾Ҵ� Session�� ��Ŷ�� ���� ���� �մϴ�. -> �κ�뿡���� �ع����� �κ�뿡 ���������� Ŭ���� ��Ŷ�� ������ �κ�뿡�� �α��� ��Ŷ�� �������!!
			if(pSession->RoomNo != INVALID_ROOMNO)
			{
				SerialRingBuffer* Packets[800];
				DWORD Size = pSession->MessageQ.GetUseSize();
				DWORD Count = static_cast<DWORD>(Size / 8);
				pSession->MessageQ.dequeue(Packets, Size);
				for (int iCnt = 0; iCnt < Count; ++iCnt)
				{
					Packets[iCnt]->SubRef();
				}
			}

			SessionMap.insert(std::make_pair(Job.SessionID, pSession));
			OnEnterRoom(Job.SessionID, Job.param);
		}break;

		case en_JOB_ROOM::ROOM_DELETE:
		{
			// room messageQ, workingQ ���� ������ ó��
		}break;

		case en_JOB_ROOM::ROOM_MESSAGE:
		{
			OnRecvToRoom(Job.param);
		}break;

		case en_JOB_ROOM::ROOM_MESSAGE_SESSION:
		{
			auto iter = SessionMap.find(Job.SessionID);
			if (iter == SessionMap.end())
			{
				Network->SendToRoomSession(Job.SessionID, Job.param);
				break;
			}

			OnRecvToRoom(Job.param);
		}break;

		default:
			break;
		}
	}

	// SessionMap�� ��ȸ�ϸ� ���۵� packet ó��
	for (auto iter = SessionMap.begin(); iter != SessionMap.end();)
	{
		// Session �˻�
		Session* pSession = iter->second;
		++iter;
		CalledChangeRoom = false;

		if (pSession->DisconnectFlag == 1)
		{
			SessionMap.erase(pSession->SessionID);
			OnSessionRelease(pSession->SessionID);
			Network->SessionUnLock(pSession);
			continue;
		}

		SerialRingBuffer* Packets[800];
		DWORD Size = pSession->MessageQ.GetUseSize();
		DWORD Count = static_cast<DWORD>(Size / 8);
		pSession->MessageQ.dequeue(Packets, Size);
		for (int iCnt = 0; iCnt < Count; ++iCnt)
		{
			CPacket Packet(Packets[iCnt]);
			Packets[iCnt]->SubRef();

			if (!CalledChangeRoom && !pSession->SuspendRecv)
				OnRecv(pSession->SessionID, Packet);
		}
	}

	// ������ ó��
	OnUpdate();
	InterlockedIncrement(&CurrentFrame);

	return;
}