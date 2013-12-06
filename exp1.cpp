#include "sysinclude.h"
#include <deque>
#include <algorithm>

using namespace std;

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

typedef unsigned int UI;
typedef unsigned char UC;

typedef enum {data, ack, nak} frame_kind;
typedef struct frame_head
{
	frame_kind kind;
	UI seq;
	UI ack;
	UC data[100];
};
typedef struct frame
{
	frame_head head;
	UI size;
};

deque<frame> onebit_queue;    //ͣ��Э��֡����
deque<frame> back_n_queue;        //����n֡Э��֡����
bool empty = true;           //�����Ƿ��п���
UI acks;
UI window_size = 0;
UI seqs;
frame temp;

/*
* ͣ��Э����Ժ���
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	if (messageType == MSG_TYPE_SEND)
	{
		temp = *(frame*)pBuffer;
		temp.size = bufferSize;
		onebit_queue.push_back(temp);      //����֡�Ӻ�����
		if (empty)       //�����пռ䣬���Է���
		{
			temp = onebit_queue.front();
			SendFRAMEPacket((UC*)&temp, temp.size);
			empty = false;
		}
	}
	if (messageType == MSG_TYPE_RECEIVE)
	{
		acks = ntohl(((frame*)pBuffer)->head.ack);
		temp = onebit_queue.front();
		if (onebit_queue.size() == 0)
			return 0;
		if (acks == ntohl(temp.head.seq))
		{
			onebit_queue.pop_front();
			if (onebit_queue.size() != 0)       //�����зǿ�����
			{
				temp = onebit_queue.front();
				SendFRAMEPacket((UC*)&temp, temp.size);
			}
			else	empty = true;        //����򿪴���
		}
	}
	if (messageType == MSG_TYPE_TIMEOUT)
	{
		seqs = ntohl(*(UINT32*)pBuffer);      //�����ֽ���ת�����ֽ���
		temp = onebit_queue.front();
		if (seqs == temp.head.seq)      //��֤���к�
			SendFRAMEPacket((UC*)&temp, temp.size);
	}
	return 0;
}

/*
* ����n֡���Ժ���
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	if (messageType == MSG_TYPE_SEND)
	{
		cout << "!send!" <<endl;
		temp = *(frame*)pBuffer;
		temp.size = bufferSize;
		back_n_queue.push_back(temp);      //����֡�Ӻ�����
		if (window_size < WINDOW_SIZE_BACK_N_FRAME && back_n_queue.size() > 0)   //�����пռ䣬���Է���
		{
			SendFRAMEPacket((UC*)&temp, temp.size);
			cout << temp.head.seq << endl;
			window_size++;
		}
	}
	if (messageType == MSG_TYPE_RECEIVE)
	{
		cout << "@receive@" <<endl;
		acks = ntohl(((frame*)pBuffer)->head.ack);
		UI bound = min(WINDOW_SIZE_BACK_N_FRAME, (int)back_n_queue.size());
		for (int i = 0;i < bound;i++) //Ѱ�ұ�ȷ�ϵ�֡
		{
			temp = back_n_queue[i];
			if (acks == ntohl(temp.head.seq))
			{
				cout<<"!!"<<i<<"!!"<<endl;
				for (int j = 0;j <= i;j++)  //����֡����֮ǰ��֡�Ӵ����Ƴ�
				{
					window_size--;
					back_n_queue.pop_front();
				}
				bound = min(WINDOW_SIZE_BACK_N_FRAME, (int)back_n_queue.size());
				i++;
				while (i < bound)  //����ǰ��  
				{
					temp = back_n_queue[i];
					SendFRAMEPacket((UC*)&temp, temp.size);
					window_size++;
					i++;
				}
				break;
			}
		}
	}
	if (messageType == MSG_TYPE_TIMEOUT)
	{
		cout << "?timeout?" <<endl;
		cout << pBuffer <<endl;
		seqs = ntohl(*(UI*)pBuffer);      //�����ֽ���ת�����ֽ���
		UI bound = min(WINDOW_SIZE_BACK_N_FRAME, (int)back_n_queue.size());
		for (int i = bound - window_size;i < bound;i++) //Ѱ����Ҫ�ش���֡
		{
			//temp = back_n_queue[i];
			//if (seqs == temp.head.seq)
			//{
				//while (i < bound)      //�ҵ����ش��������֡
				//{
					temp = back_n_queue[i];
					SendFRAMEPacket((UC*)&temp, temp.size);
					//cout << temp.head.seq << endl;
					//bound = min(WINDOW_SIZE_BACK_N_FRAME, (int)back_n_queue.size());
					//i++;
				//}
				//break;
			//}
		}
	}
	return 0;
}

/*
* ѡ�����ش����Ժ���
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	return 0;
}
