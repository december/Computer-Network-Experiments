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

deque<frame> onebit_queue;    //停等协议帧队列
deque<frame> back_n_queue;        //回退n帧协议帧队列
bool empty = true;           //窗口是否有空闲
UI acks;
UI window_size = 0;
UI seqs;
frame temp;

/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	if (messageType == MSG_TYPE_SEND)
	{
		temp = *(frame*)pBuffer;
		temp.size = bufferSize;
		onebit_queue.push_back(temp);      //发送帧从后端入队
		if (empty)       //窗口有空间，可以发送
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
			if (onebit_queue.size() != 0)       //若队列非空则发送
			{
				temp = onebit_queue.front();
				SendFRAMEPacket((UC*)&temp, temp.size);
			}
			else	empty = true;        //否则打开窗口
		}
	}
	if (messageType == MSG_TYPE_TIMEOUT)
	{
		seqs = ntohl(*(UINT32*)pBuffer);      //网络字节序转主机字节序
		temp = onebit_queue.front();
		if (seqs == temp.head.seq)      //验证序列号
			SendFRAMEPacket((UC*)&temp, temp.size);
	}
	return 0;
}

/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	if (messageType == MSG_TYPE_SEND)
	{
		cout << "!send!" <<endl;
		temp = *(frame*)pBuffer;
		temp.size = bufferSize;
		back_n_queue.push_back(temp);      //发送帧从后端入队
		if (window_size < WINDOW_SIZE_BACK_N_FRAME && back_n_queue.size() > 0)   //窗口有空间，可以发送
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
		for (int i = 0;i < bound;i++) //寻找被确认的帧
		{
			temp = back_n_queue[i];
			if (acks == ntohl(temp.head.seq))
			{
				cout<<"!!"<<i<<"!!"<<endl;
				for (int j = 0;j <= i;j++)  //将该帧及其之前的帧从窗口移除
				{
					window_size--;
					back_n_queue.pop_front();
				}
				bound = min(WINDOW_SIZE_BACK_N_FRAME, (int)back_n_queue.size());
				i++;
				while (i < bound)  //窗口前移  
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
		seqs = ntohl(*(UI*)pBuffer);      //网络字节序转主机字节序
		UI bound = min(WINDOW_SIZE_BACK_N_FRAME, (int)back_n_queue.size());
		for (int i = bound - window_size;i < bound;i++) //寻找需要重传的帧
		{
			//temp = back_n_queue[i];
			//if (seqs == temp.head.seq)
			//{
				//while (i < bound)      //找到后重传其后所有帧
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
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	return 0;
}
