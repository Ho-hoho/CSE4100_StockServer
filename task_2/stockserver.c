/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#include <string.h>
#define MAXID 8192
void echo(int connfd);

struct item{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
    sem_t r_lock;
}items[MAXID+1],temp;

sem_t thread_lock;
int thread_cnt;


void show_tree(int idx,char buf[]){
    char pbuf[MAXLINE];
    if(items[idx].ID == -1)
        return;
    P(&items[idx].r_lock);
    items[idx].readcnt++;
    if(items[idx].readcnt == 1)
        P(&items[idx].mutex);
    V(&items[idx].r_lock);
    
    sprintf(pbuf,"%d %d %d\n",items[idx].ID,items[idx].left_stock,items[idx].price);
    strcat(buf,pbuf);
    
    
    P(&items[idx].r_lock);
    items[idx].readcnt--;
    if(items[idx].readcnt == 0)
        V(&items[idx].mutex);
    V(&items[idx].r_lock);
    


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

void recieve_request(int connfd) 
{
    int n,N,M; 
    char buf[MAXLINE],res[MAXLINE],cmd[20],c2[20],c3[20]; 
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        //printf("server received %s\n", buf);
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
                P(&items[idx].mutex);
                if(items[idx].left_stock >= M){
                    items[idx].left_stock -= M;
                    strcpy(res,"[buy] success\n");
                }
                else
                    strcpy(res,"Not enough left stock\n");
                V(&items[idx].mutex);
            }
            else if(!strcmp(cmd,"sell")){
                P(&items[idx].mutex);
                items[idx].left_stock += M;
                strcpy(res,"[sell] success\n");
                V(&items[idx].mutex);

            }
        }

        //while(cmd != NULL){

        //}
        //printf("%s\n",res);
        Rio_writen(connfd, res, MAXLINE);
    }
}

void *thread(void *vargp){
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    
    P(&thread_lock);
    thread_cnt++;
    V(&thread_lock);

    recieve_request(connfd);
    
    P(&thread_lock);
    thread_cnt--;
    if(thread_cnt == 0){
        char buf[MAXLINE];
        memset(buf,0,sizeof(buf));
        FILE* fp = fopen("stock.txt","w");
        show_tree(1,buf);
        
        Fputs(buf,fp);
        fclose(fp);
    }
    V(&thread_lock);


    Close(connfd);    
    return NULL;
}

int main(int argc, char **argv) 
{
    FILE* fp;
    char str[100],*result;
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;
    

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    thread_cnt=0;
    sem_init(&thread_lock,0,1);
    for(int i=0; i<=MAXID;i++){
        items[i].ID = -1;
        items[i].readcnt = 0;
        sem_init(&items[i].mutex,0,1);
        sem_init(&items[i].r_lock,0,1);
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
    while (1) {
        clientlen = sizeof(struct sockaddr_storage); 
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        
        pthread_create(&tid,NULL,thread,connfdp);    
            
        //echo(connfd);
        ///Close(connfd);
        
    }
    exit(0);
}
/* $end echoserverimain */
