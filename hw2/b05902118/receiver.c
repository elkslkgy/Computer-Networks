#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
} header;

typedef struct {
	header head;
	char data[1000];
} segment;

void setIP(char *dst, char *src) {
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0 || strcmp(src, "localhost"))
        sscanf("127.0.0.1", "%s", dst);
    else
        sscanf(src, "%s", dst);
}

int main(int argc, char* argv[]) {
    FILE *output;
    int receiversocket, portNum, nBytes;
    segment s_tmp, g_tmp;

    char write_data[32][1000];
    int write_length[32];
    int flush_data = 0;
    int flush = 0;

    struct sockaddr_in agent, receiver, tmp_addr;
    socklen_t agent_size, recv_size, tmp_size;
    char ip[3][50];
    int port[3];
    
    if(argc != 6) {
        fprintf(stderr,"用法: %s <agent IP> <recv IP> <agent port> <recv port> <result filename>\n", argv[0]);
        fprintf(stderr, "例如: ./receiver local local 8888 8889 result.txt\n");
        exit(1);
    } else {
        setIP(ip[0], argv[1]);
        setIP(ip[1], argv[2]);

        sscanf(argv[3], "%d", &port[0]);
        sscanf(argv[4], "%d", &port[1]);

        output = fopen(argv[5], "w"); 
    }

    /*Create UDP socket*/
    if ((receiversocket = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Can't create UDP socket");
        exit(-1);
    } 

    /*Configure settings in agent struct*/
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[0]);    //agent port
    agent.sin_addr.s_addr = inet_addr(ip[0]);   //agent IP --> 本地
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

    /*Configure settings in receiver struct*/
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port[1]); //recv port
    receiver.sin_addr.s_addr = inet_addr(ip[1]);    //recv IP
    memset(receiver.sin_zero, '\0', sizeof(receiver.sin_zero));

    /*bind socket*/
    if ((bind(receiversocket, (struct sockaddr *)&receiver, sizeof(receiver))) == -1) {
        close(receiversocket);
        fprintf(stderr, "Can't bind\n");
        exit(-1);
    }
 
    /*Initialize size variable to be used later on*/
    agent_size = sizeof(agent);
    recv_size = sizeof(receiver);
    tmp_size = sizeof(tmp_addr);

    printf("可以開始測囉^Q^\n");
    printf("receiver info: ip = %s port = %d\n",ip[0], port[0]);
    printf("agent info: ip = %s port = %d\n", ip[1], port[1]);

    int now_data = 0;   //現在收到第幾個data
    int segment_size, index;
    while(1) {

        /*Receive message from receiver and sender*/
        memset(&g_tmp, 0, sizeof(g_tmp));
        segment_size = recvfrom(receiversocket, &g_tmp, sizeof(g_tmp), 0, (struct sockaddr *)&tmp_addr, &tmp_size);
        if(segment_size > 0) {

            /*segment from receiver, ack*/
            if(g_tmp.head.ack == 1) {
                fprintf(stderr, "收到來自 sender 的 ack\n");
                exit(1);
            }
            if(g_tmp.head.fin == 1) {
                printf("recv    fin\n");
                g_tmp.head.ack = 1;
                sendto(receiversocket, &g_tmp, segment_size, 0, (struct sockaddr *)&agent, agent_size);
                printf("send    finack\n");
                printf("flush\n");
                for (int i = 0; i < flush_data; i++) {
                    fwrite(write_data[i], 1, write_length[i], output);
                }
                break;
            } else {
                if (g_tmp.head.seqNumber == now_data + 1) {
                    now_data++;
                    index = g_tmp.head.ackNumber = g_tmp.head.seqNumber;
                    if (flush_data + 1 > 32) {
                        printf("drop    data    #%d\n", g_tmp.head.seqNumber);
                        now_data--;
                        index = g_tmp.head.ackNumber = now_data;
                        flush = 1;

                    } else {
                        printf("recv    data    #%d\n", index);
                        memcpy(write_data[flush_data], g_tmp.data, g_tmp.head.length);
                        write_length[flush_data] = g_tmp.head.length;
                        flush_data++;
                    }
                } else {
                    printf("drop    data    #%d\n", g_tmp.head.seqNumber);
                    index = g_tmp.head.ackNumber = now_data;
                }
                g_tmp.head.ack = 1;
                sendto(receiversocket, &g_tmp, segment_size, 0, (struct sockaddr *)&agent, agent_size);
                printf("send    ack     #%d\n", index);
                if (flush) {
                    printf("flush\n");
                    for (int i = 0; i < flush_data; i++) {
                        fwrite(write_data[i], 1, write_length[i], output);
                    }
                    memset(write_data, 0, sizeof(write_data));
                    memset(write_length, 0, sizeof(write_length));
                    flush_data = 0;
                    flush = 0;
                }
            }
        }
    }
    return 0;
}