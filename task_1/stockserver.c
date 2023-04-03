/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#include <string.h>
#define MAXID 8192

struct item{
    int ID;
    int left_stock;
    int price;
    int readcnt;
}items[MAXID+1],temp;

typedef struct { 
    int maxfd; 
    fd_set read_set; 
    fd_set ready_set; 
    int nready; 
    int maxi; 
    int clientfd[FD_SETSIZE]; 
    rio_t clientrio[FD_SETSIZE]; 
} pool;

int fd_cnt = 0;


void init_pool(int listenfd, pool *p){
    int i;
    p->maxi = -1;
    for (i=0; i< FD_SETSIZE; i++)
        p->clientfd[i] = -1;
    
    
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p){
    int i;
    p->nready--;
    fd_cnt++;
    for (i = 0; i < FD_SETSIZE; i++)
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);
            
            if (connfd > p->maxfd)
                p->maxfd = connfd;
            if (i > p->maxi)
                p->maxi = i;
            break;
        }
    if (i == FD_SETSIZE) 
        app_error("add_client error: Too many clients");
}

void show_tree(int idx,char buf[]){
    char pbuf[MAXLINE];
    if(items[idx].ID == -1)
        return;
    
    sprintf(pbuf,"%d %d %d\n",items[idx].ID,items[idx].left_stock,items[idx].price);
    strcat(buf,pbuf);

    if(idx*2 +1 <= MAXID){
        show_tree(idx*2,buf);
        show_tree(idx*2+1,buf);
    }

}

int find_tree_idx(int id){
    int idx=1;
    while(1){
        if(items[idx].ID == id)
            return idx;
        idx *= 2;
        if(items[idx].ID < id)
            idx +=1;
    }
}
void check_clients(pool *p){
    int i, connfd, n,N,M;
    char buf[MAXLINE],res[MAXLINE],cmd[20],c2[20],c3[20];
    rio_t rio;

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                memset(res,0,strlen(res));
                sscanf(buf,"%s",cmd);
                if(!strcmp(cmd,"show")){
                    show_tree(1,res);
                    res[strlen(res)-1] = '\n';
                    //printf("%s\n",res);
                }
                else if(!strcmp(cmd,"exit")){
                    strcpy(res,"exit\n");
                    //return;
                }
                else{
                    sscanf(buf,"%s %s %s",cmd,c2,c3);
                    N= (int)atoi(c2); M= (int)atoi(c3);
                    int idx = find_tree_idx(N);
                    if(!strcmp(cmd,"buy")){
                        if(items[idx].left_stock >= M){
                            items[idx].left_stock -= M;
                            strcpy(res,"[buy] success\n");
                        }
                        else
                            strcpy(res,"Not enough left stock\n");
                    }
                    else if(!strcmp(cmd,"sell")){
                        items[idx].left_stock += M;
                        strcpy(res,"[sell] success\n");
                    }
                }

                //printf("%s\n",res);
                Rio_writen(connfd, res, MAXLINE);
            
            }
            else {
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
                fd_cnt--;
                if(fd_cnt == 0){
                    char buf[MAXLINE];
                    memset(buf,0,sizeof(buf));
                    FILE* fp = fopen("stock.txt","w");
                    show_tree(1,buf);
                    
                    Fputs(buf,fp);
                    fclose(fp);

                }
            }
        }
    }
}

int main(int argc, char **argv) 
{
    FILE* fp;
    char str[100],*result;
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    
    for(int i=0; i<=MAXID;i++){
        items[i].ID = -1;
        items[i].readcnt = 0;
    }

    fp = fopen("stock.txt","r");
    while((result = fgets(str,100,fp))!= NULL){
        int idx=1;
        sscanf(str,"%d %d %d",&temp.ID,&temp.left_stock,&temp.price);
        while(items[idx].ID != -1){
            idx *= 2;
            if(items[idx].ID < temp.ID)
                idx +=1;
        }
        items[idx].ID = temp.ID;
        items[idx].left_stock = temp.left_stock;
        items[idx].price = temp.price;
    }
    
    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd,&pool);
    fd_cnt = 0;

    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);
        
        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            add_client(connfd, &pool);
            
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            
        }
        check_clients(&pool);
    }
    exit(0);
}
/* $end echoserverimain */
