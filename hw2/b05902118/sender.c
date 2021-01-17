#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

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

FILE *input;
int already_send = 0;   //已經送出過的最大的data Number
int now_data = 0;   //現在送到第幾個data
int recv_data = 0;  //現在收到第幾個data的ack
int get_data = 0;   //現在winSize裡面有幾個data成功送給receiver了
int winSize = 1;
int threshold = 16;
int finish = 0; //該結束了嗎

int sendersocket;
struct sockaddr_in sender, agent, tmp_addr;
socklen_t sender_size, agent_size, tmp_size;

void setIP(char *dst, char *src) {
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0 || strcmp(src, "localhost"))
        sscanf("127.0.0.1", "%s", dst);
    else
        sscanf(src, "%s", dst);
}

void sigalrm_fn(int sig) {
    segment s_tmp;

    if ((winSize / 2) > 1) threshold = winSize / 2;
    else threshold = 1;
    printf("time    out,           threshold = %d\n", threshold);
    if (now_data != recv_data) {
        winSize = 1;
        get_data = 0;
        finish = 0;
        fseek(input, 1000 * (recv_data), SEEK_SET);
        s_tmp.head.length = fread(s_tmp.data, 1, 1000, input);
        s_tmp.head.seqNumber = recv_data + 1;
        s_tmp.head.ackNumber = 0;
        s_tmp.head.fin = 0;
        s_tmp.head.syn = 0;
        s_tmp.head.ack = 0;
        now_data = recv_data + 1;
        if (now_data > already_send) already_send = now_data;

        if ((sendto(sendersocket, &s_tmp, sizeof(s_tmp), 0, (struct sockaddr *)&agent, agent_size)) == -1)
            fprintf(stderr, "Send error\n");
        printf("resnd    data    #%d,    winSize = %d\n", now_data, winSize);
    }
    alarm(1);
    return;
}

int main(int argc, char* argv[]) {
    segment s_tmp, g_tmp;
    char ip[2][50];
    int port[2];
    
    if(argc != 6) {
        fprintf(stderr,"用法: %s <sender IP> <agent IP> <sender port> <agent port> <test filename>\n", argv[0]);
        fprintf(stderr, "例如: ./sender local local 8887 8888 test.txt\n");
        exit(1);
    } else {
        setIP(ip[0], argv[1]);
        setIP(ip[1], argv[2]);

        sscanf(argv[3], "%d", &port[0]);
        sscanf(argv[4], "%d", &port[1]);

        input = fopen(argv[5], "r");
    }

    /*Create UDP socket*/
    if ((sendersocket = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Can't create UDP socket");
        exit(-1);
    }

    /*Configure settings in sender struct*/
    sender.sin_family = AF_INET;
    sender.sin_port = htons(port[0]);  //sender port 
    sender.sin_addr.s_addr = inet_addr(ip[0]);  //sender IP
    memset(sender.sin_zero, '\0', sizeof(sender.sin_zero));  

    /*Configure settings in agent struct*/
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[1]);    //agent port
    agent.sin_addr.s_addr = inet_addr(ip[1]);   //agent IP --> 本地
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

    /*bind socket*/
    if ((bind(sendersocket, (struct sockaddr *)&sender, sizeof(sender))) == -1) {
        close(sendersocket);
        fprintf(stderr, "Can't bind\n");
        exit(-1);
    }

    /*Initialize size variable to be used later on*/
    sender_size = sizeof(sender);
    agent_size = sizeof(agent);
    tmp_size = sizeof(tmp_addr);

    printf("可以開始測囉^Q^\n");
    printf("sender info: ip = %s port = %d\n",ip[0], port[0]);
    printf("agent info: ip = %s port = %d\n", ip[1], port[1]);

    int segment_size, index;

    s_tmp.head.length = fread(s_tmp.data, 1, 1000, input);
    s_tmp.head.seqNumber = now_data + 1;
    s_tmp.head.ackNumber = 0;
    s_tmp.head.fin = 0;
    s_tmp.head.syn = 0;
    s_tmp.head.ack = 0;
    now_data++;

    if ((sendto(sendersocket, &s_tmp, sizeof(s_tmp), 0, (struct sockaddr *)&agent, agent_size)) == -1)
        fprintf(stderr, "Send error\n");

    printf("send    data    #%d,    winSize = %d\n", now_data, winSize);
    while(1){

        /*Receive message from agent*/
        memset(&g_tmp, 0, sizeof(g_tmp));

        signal(SIGALRM, sigalrm_fn);
        alarm(1);
        segment_size = recvfrom(sendersocket, &g_tmp, sizeof(g_tmp), 0, (struct sockaddr *)&tmp_addr, &tmp_size);
        if(segment_size > 0){

            /*segment from sender, not ack*/
            if(g_tmp.head.fin == 1) {
                printf("recv    finack\n");
                break;
            } else if (g_tmp.head.ack == 1) {
                index = g_tmp.head.ackNumber;
                printf("recv    ack     #%d\n", index);
                recv_data = index;
                if (finish && (now_data == index)) {
                    s_tmp.head.seqNumber = 0;
                    s_tmp.head.ackNumber = 0;
                    s_tmp.head.fin = 1;
                    s_tmp.head.syn = 0;
                    s_tmp.head.ack = 0;
                    if ((sendto(sendersocket, &s_tmp, sizeof(s_tmp), 0, (struct sockaddr *)&agent, agent_size)) == -1)
                        fprintf(stderr, "Send error\n");
                    printf("send    fin\n");
                } else if (index == g_tmp.head.seqNumber) {
                    get_data++;
                    if (get_data == winSize) {
                        get_data = 0;
                        if (winSize < threshold) winSize *= 2;
                        else winSize++;
                        for (int i = 0; i < winSize; i++) {
                            memset(&s_tmp, 0, sizeof(s_tmp));
                            s_tmp.head.length = fread(s_tmp.data, 1, 1000, input);

                            if (s_tmp.head.length < 1000) finish = 1;
                            s_tmp.head.seqNumber = now_data + 1;
                            s_tmp.head.ackNumber = 0;
                            s_tmp.head.fin = 0;
                            s_tmp.head.syn = 0;
                            s_tmp.head.ack = 0;
                            now_data++;

                            if ((sendto(sendersocket, &s_tmp, sizeof(s_tmp), 0, (struct sockaddr *)&agent, agent_size)) == -1)
                                fprintf(stderr, "Send error\n");
                            if (now_data > already_send) {
                                already_send = now_data;
                                printf("send    data    #%d,    winSize = %d\n", now_data, winSize);
                            } else
                                printf("resnd   data    #%d,    winSize = %d\n", now_data, winSize);

                            if (finish) break;
                        }
                    }
                }
            }
        }
    }

    return 0;
}