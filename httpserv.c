#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h> /* phtread*/

#include <string.h> /* memset */
#include <unistd.h> /* close */
#include <stdlib.h> /* exit */
#include <fcntl.h>
#include <sys/types.h>

#define SERVERVERSION "SteinernerMo"
#define BUFFERSIZE 4096

char *fileextensions [11][2] = {
  {"html","text/html" },
  {"htm", "text/html" },
  {"ico", "image/x-icon"},
  {"gif", "image/gif" },
  {"jpg", "image/jpg" },
  {"jpeg","image/jpeg"},
  {"png", "image/png" },
  {"ico", "image/ico" },
  {"zip", "image/zip" },
  {"gz",  "image/gz"  },
  {"tar", "image/tar" }
};


int serversocket;
char rootpath[] = "/VOLUMES/Data/webroot";
char ip[] = "127.0.0.1";
char port[] = "3490";

struct addrinfo *addressinfo(){
    struct addrinfo hints, *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(ip, port, &hints, &result) != 0)
    {
        perror ("error");
        exit(1);
    }
    return result;

}


void  settingupServer()
{
    struct addrinfo *serveraddr;
    serveraddr = addressinfo();

        if((serversocket = socket(serveraddr->ai_family, serveraddr->ai_socktype, 0)) < 0)
        {
          perror("Socket error\n");
          exit(1);
        }
        if (bind(serversocket, serveraddr->ai_addr, serveraddr->ai_addrlen) < 0)
        {
          perror("Bind error\n");
          exit(1);
        }

    freeaddrinfo(serveraddr);


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

char *timeCalc(int size, char buf[]){
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, size-1, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    return buf;
}

void unimplementedrequest(int client)
{
    char buffer[BUFFERSIZE+1];
    memset(buffer, 0, sizeof(buffer));

    sprintf(buffer, "HTTP/1.0 501  Not Implemented\n");
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "Server: httpserv %s \n", SERVERVERSION);
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "Date: %s \n", timeCalc(30, buffer));
    write(client, buffer, strlen(buffer));
    strcpy(buffer, "Content-Type: text/html\n");
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "\n\n");
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "<html><head></head><body><h1>501 - NOT IMPLEMENTED</h1></body></html>\n");
    write(client, buffer, strlen(buffer));
}


void filenotfoundrequest(int client)
{
    char buffer[BUFFERSIZE+1];
    memset(buffer, 0, sizeof(buffer));

    sprintf(buffer, "HTTP/1.0 404 NOT FOUND\n");
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "Server: httpserv %s \n", SERVERVERSION);
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "Date: %s \n", timeCalc(30, buffer));
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html\n");
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "\n\n");
    write(client, buffer, strlen(buffer));
    sprintf(buffer, "<html><head></head><body><h1>404 - FILE NOT FOUND</h1></body></html>\n");
    write(client, buffer, strlen(buffer));

}

int getFileextension(char filepath[])
{
    for(int i= 0; i<11; i++){

        int len = strlen(fileextensions[i][0]);
        int bufflen = strlen(filepath);
        if( !strncmp(&filepath[bufflen-len], fileextensions[i][0], len)) {
            printf("%s \n", fileextensions[i][1]);
            return i;
        }
    }

    return -1;
}

void implementedrequest(int clientsocket, char *path)
{
    int fd;
    char filepath[2048] = "";
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));


    if (strcmp(path, "/") == 0)
        path = "/index.html";

    strcat(filepath, rootpath);
    strcat(filepath, path);

   int extensionnumber =   getFileextension(filepath);
    printf("path:  %s \r\n", filepath);

    fd = open(filepath, O_RDONLY);

    if( fd != -1)
    {
        long len = (long)lseek(fd, (off_t)0, SEEK_END);
        (void)lseek(fd, (off_t)0, SEEK_SET);

        sprintf(buffer, "HTTP/1.1 200 OK\n");
        write(clientsocket, buffer, strlen(buffer));
        sprintf(buffer, "Server: httpserv %s \n", SERVERVERSION);
        write(clientsocket, buffer, strlen(buffer));
        sprintf(buffer, "Date: %s \n", timeCalc(30, buffer));
        write(clientsocket, buffer, strlen(buffer));
        sprintf(buffer, "Content-Length: %ld\n", len);
        write(clientsocket, buffer, strlen(buffer));
        sprintf(buffer, "Content-Type: %s \n\n", fileextensions[extensionnumber][1]);
        write(clientsocket, buffer, strlen(buffer));

        long ret;
        char buff[BUFFERSIZE+1];
        while (	(ret = read(fd, buff, BUFFERSIZE)) > 0 ) {
            (void)write(clientsocket,buff,ret);
        }

    }

    else
    {
        perror("Error while opening the file.\n");
        filenotfoundrequest(clientsocket);
    }
    close(fd);
}

void request(int clientsocket)
{

    char message[BUFFERSIZE+1],  *requestpart[3], *requestline;
    int recieved;

    memset(message, 0, BUFFERSIZE+1);

    recieved=recv(clientsocket, message, BUFFERSIZE, 0);

    if (recieved>0){

        //parse requestline
        requestline = strtok(message, "\r\n");
        printf("requestline: %s \r\n", requestline);
        requestpart[0] = strtok(requestline, " ");
        requestpart[1] = strtok(NULL, " ");
        requestpart[2] = strtok(NULL, " ");

        if((strcasecmp(requestpart[0], "GET") == 0) &&
              ((strcasecmp(requestpart[2], "HTTP/1.0") == 0) || (strcasecmp(requestpart[2], "HTTP/1.1") == 0))  ){
                      implementedrequest(clientsocket, requestpart[1]);
            }
        else{
            unimplementedrequest(clientsocket);
        }
    }

    close(clientsocket);
}



void serverRun()
{
    int clientsocket;
    pthread_t thread;

    printf("Running: %s:%s \n", ip, port);
    while(1)
    {
        clientsocket = acceptConnection();

        if (pthread_create(&thread , NULL, request, clientsocket) != 0){
            perror("Create thread error \n");
        }


    }
}

int main(int argc, const char * argv[]) {

    printf("Welcome to httpserv - Version: %s \n", SERVERVERSION);
    settingupServer();
    serverRun();

    return 0;
}
