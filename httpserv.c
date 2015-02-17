#include <stdio.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include <unistd.h>
#include <pthread.h>


int serversocket;


struct addrinfo *addressinfo(){
    struct addrinfo hints, *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo("127.0.0.1", "3490", &hints, &result) != 0)
    {
        perror ("error");
        exit(1);
    }
    return result;

}


void  settingupServer()
{
    struct addrinfo *res, *p;
    res = addressinfo();

    for (p = res; p!=NULL; p=p->ai_next)
    {
        serversocket = socket (p->ai_family, p->ai_socktype, 0);
        if (serversocket == -1)
            continue;
        if (bind(serversocket, p->ai_addr, p->ai_addrlen) == 0)
            break;
    }

    freeaddrinfo(res);


    if ( listen (serversocket, 50) != 0 )
    {
        perror("Listen error\n");
        exit(1);
    }

}

int acceptConnection()
{
    struct sockaddr_storage their_addr;

    socklen_t addr_size = sizeof their_addr;
    int new_fd;

    if((new_fd = accept(serversocket, (struct sockaddr *)&their_addr, &addr_size)) == -1)
    {
           perror("Can't accept request\n");
    }

    return new_fd;
}

void templateReturn(int client)
{
    char buffer[1024];

    sprintf(buffer, "test answer httpserv!!!!");
    send(client, buffer, strlen(buffer), 0);
}


void request(int clientsocket)
{

    char message[99999];
    int recieved;

    memset( (void*)message, (int)'\0', 99999 );

    recieved=recv(clientsocket, message, 99999, 0);

    if (recieved<0)
        fprintf(stderr,("recvieve error\n"));
    else if (recieved==0)
        fprintf(stderr,"Client disconnected.\n");
    else
    {

        printf("recieved \n");
        templateReturn(clientsocket);

    }

    close(clientsocket);
}


void serverRun()
{
    int clientsocket;
    pthread_t thread;

    printf("httpserv is running \n");
    while(1)
    {
        clientsocket = acceptConnection();

            if (pthread_create(&thread , NULL, request, clientsocket) != 0){
                perror("Create thread error \n");
            }


    }
}

int main(int argc, const char * argv[]) {

    printf("Welcome to httpserv\n");
    settingupServer();
    serverRun();

    return 0;
}
