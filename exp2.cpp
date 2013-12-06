#include "sysinclude.h"

extern void rip_sendIpPkt(unsigned char *pData, UINT16 len,unsigned short dstPort,UINT8 iNo);

extern struct stud_rip_route_node *g_rip_route_table;

typedef unsigned int UI;
typedef unsigned char UC;
typedef unsigned short US;

struct ripbody
{
       US AFI;
       US RouteTag;
       UI ipAdd;
       UI SubMask;
       UI NextHop;
       UI metric;
       ripbody():RouteTag(0), AFI(2) {}
};

struct ripchart
{
       char command;
       char version;
       US MBZ;
       ripbody data[10000];
       ripchart():version(2), MBZ(0) {}
};

void senddata(ripchart *rc, UI iNo)
{
     rc->command = 2;
     stud_rip_route_node *temp = g_rip_route_table;
     int length = 4;
     int number = 0;
     while (temp)
     {
           if (temp->if_no != iNo && number < 25)
           {
               rc->data[number].AFI = htons(2);
               rc->data[number].RouteTag = htons(0);
               rc->data[number].ipAdd = htonl(temp->dest);
               rc->data[number].SubMask = htonl(temp->mask);
               rc->data[number].NextHop = htonl(temp->nexthop);
               rc->data[number].metric = htonl(temp->metric);
               number++;
               length += 20;
           }
           temp = temp->next;
     }
     rip_sendIpPkt((UC*)rc, length, 520, iNo);
}
       

int stud_rip_packet_recv(char *pBuffer,int bufferSize,UINT8 iNo,UINT32 srcAdd)
{	
	ripchart *chart = (ripchart*)pBuffer;
	if (chart->version != 2)
	{
        ip_DiscardPkt(pBuffer, STUD_RIP_TEST_VERSION_ERROR);
        return -1;
    }
    if (chart->command != 1 && chart->command != 2)
    {
        ip_DiscardPkt(pBuffer, STUD_RIP_TEST_COMMAND_ERROR);
        return -1;
    }
    if (chart->command == 1)
    {
        ripchart tobesent;
        senddata(&tobesent, iNo);
    }
    else
    {
        int number = (bufferSize - 4) / 20;
        for (int i = 0;i < number;i++)
        {
            stud_rip_route_node *temp = g_rip_route_table;
            ripbody *rb = &(chart->data[i]);
            bool newitem = true;
            while (temp)
            {
                  if (temp->dest == ntohl(rb->ipAdd) && temp->mask == ntohl(rb->SubMask))
                  {
                      newitem = false;
                      if (temp->nexthop == srcAdd)
                      {
                          temp->metric = ntohl(rb->metric) + 1;
                          temp->if_no = iNo;
                      }
                      else
                      {
                          if (ntohl(rb->metric) < temp->metric)
                          {
                              temp->metric = ntohl(rb->metric)+1;
                              temp->nexthop = srcAdd;
                              temp->if_no = iNo;
                          }
                      }
                      if (temp->metric > 16)
                          temp->metric = 16;
                      break;
                  }
                  temp = temp->next;
            }
            if (newitem && ntohl(rb->metric) < 15)
            {
                stud_rip_route_node *node = new stud_rip_route_node;
                node->dest = ntohl(rb->ipAdd);
                node->mask = ntohl(rb->SubMask);
                node->nexthop = srcAdd;
                node->metric = ntohl(rb->metric) + 1;
                node->if_no = iNo;
                node->next = g_rip_route_table->next;
                g_rip_route_table->next = node;
            }
        }
    }
	return 0;
}

void stud_rip_route_timeout(UINT32 destAdd, UINT32 mask, unsigned char msgType)
{
	if (msgType == RIP_MSG_SEND_ROUTE)
	{
		ripchart chartone, charttwo;
		senddata(&chartone, 1);
		senddata(&charttwo, 2);
	}
	if (msgType == RIP_MSG_DELE_ROUTE)
	{
		stud_rip_route_node *temp = g_rip_route_table;
		while (temp)
		{
			if (temp->dest == destAdd && temp->mask == mask)
			{
				temp->metric = 16;
				break;
			}
			temp = temp->next;
		}
	}
}
