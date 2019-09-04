#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "dir.h"
#include "usage.h"

// from myFTP.cs
#include "netbuffer.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

// from server

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.
static void handle_client(int fd);

#define MAX_LINE_LENGTH 1024

static void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    else
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

    // This is some sample code feel free to delete it
    // This is the main program for the thread version of nc

    int i;

    // Check the command line arguments
    if (argc != 2) {
        usage(argv[0]);
        return -1;
    }

    run_server(argv[1], handle_client);
    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor
    // returned for the ftp server's data connection

    printf("Printed %d directory entries\n", listFiles(1, "."));
    return 0;

}

// set follow-fork-mode child set detach-on-fork off
//

void handle_client(int fd) {

    // TODO To be implemented

    net_buffer_t recvBuf = nb_create(fd, MAX_LINE_LENGTH);
    char buf[MAX_LINE_LENGTH + 1] = {0};
    int loggedIn = 0;
    send_string(fd, "220 Service ready for new user.\r\n");
    char ftpRoot[128] = {};
    getcwd(ftpRoot, 128);
    int sockfd;


    while(1) {
        nb_read_line(recvBuf, buf);
//        send_string(fd, buf);
        char dup[1025];
        strcpy(dup, buf);
        char *firstArg;
        char *extraArg;
        firstArg = strtok(dup, " \r\n");
        extraArg = firstArg;
        int count = 0;
        while (extraArg != NULL) {
            printf("%s\n", extraArg);
            extraArg = strtok(NULL, " \r\n");
            count++;
        }


        if (!loggedIn) {
            if (strcasecmp(firstarg, "user") == 0 && count == 2) {
                // this recopies the buf into dup as the previous strtok has modified it
                strcpy(dup, buf);
                strtok(dup, " \r\n");
                char *user = strtok(null, " \r\n");

                if (strcmp(user, "cs317") == 0) {
                    loggedIn = 1;
                    send_string(fd, "230 user logged in, proceed.\r\n");
                } else {
                    send_string(fd, "530 not logged in. only supports cs317 as user.\r\n");
                }
            } else {
                send_string(fd,
                            "530 not logged in. only supports cs317 as user or incorrect number of args given.\r\n");
            }
        } else if (strcasecmp(firstarg, "quit") == 0 && count == 1) {
            //do we need to check for this
//            if file transfer is not
//            in progress, the server closes the control connection.  if
//            file transfer is in progress, the connection will remain
//            open for result response and the server will then close it.
            send_string(fd, "goodbye.\r\n");
            nb_destroy(recvbuf);
            break;
        } else if (strcasecmp(firstarg, "quit") == 0 && count != 1) {
            send_string(fd, "500 syntax error in parameters or arguments.\r\n");
        } else if (strcasecmp(firstarg, "cwd") == 0) {
            strcpy(dup, buf);
            strtok(dup, " \r\n"); // this removes the command (cwd) again


//            cwd "test folder"
//            you can try checking for quotation marks. here's another way:
//
//            cd test\ folder
            // need to count args not including spaces

            //resets dup to after one strtok, now we can get the directory to change into, we need to change
            // delimiters as a directory can have a space it its name /test space etc.
            char *passeddirectory = strtok(null, "\\\"\r\n");
            if ((strstr(passeddirectory, "./") != null) || (strstr(passeddirectory, "../") != null) ||
                strtok(null, "\\\"\r\n") != null) {
                send_string(fd, "501 syntax error in parameters or arguments.\r\n");
            } else {
                if (chdir(passeddirectory) == 0) {
                    send_string(fd, "250 directory successfully changed.\r\n");
                } else {
                    send_string(fd, "550 Failed to change directory.\r\n");
                }
            }

        } else if (strcasecmp(firstArg, "CDUP") == 0) {
            char cwd[128] = {};
            getcwd(cwd, 128);

            if (strcmp(cwd, ftpRoot) == 0) {
                send_string(fd, "550 Failed to change directory.\r\n");
            } else {
                if ((chdir("../") == 0)) {
                    send_string(fd, "250 Directory successfully changed.\r\n");
                } else {
                    send_string(fd, "550 Failed to change directory.\r\n");
                }
            }

        } else if (strcasecmp(firstArg, "SYST") == 0 || strcasecmp(firstArg, "FEAT") == 0 ||
                   strcasecmp(firstArg, "PWD") == 0) {
            // SILENTLY IGNORE AS STATED https://piazza.com/class/jq71qu0b3sj2pu?cid=607, if we dont \r\n, it ftp linux stalls
            send_string(fd, "\r\n");
        } else if (strcasecmp(firstArg, "PASV") == 0 && count == 1) {
//            run_passive(fd, handle_client);
            struct addrinfo hints, *servinfo, *p;
            struct sockaddr_storage initial_sock; // connector's address information
            socklen_t sockSize = sizeof(struct sockaddr_storage);
            struct sigaction sa;
            int yes = 1;
            char s[INET6_ADDRSTRLEN];
            int rv;

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;   // use IPv4 or IPv6, whichever is available
            hints.ai_socktype = SOCK_STREAM; // create a stream (TCP) socket server
            hints.ai_flags = AI_PASSIVE;  // use any available connection

            time_t t;
            srand((unsigned) time(&t));
            int portNum = (rand() % 64512) + 1024;
            printf("%d", portNum);
            // do we need to add a check to make sure it's not the original port passed in?

            char portString[8];
            sprintf(portString, "%d", portNum);
            getsockname(fd, (struct sockaddr *) &initial_sock, &sockSize);
            char ipString[INET6_ADDRSTRLEN];
            inet_ntop(initial_sock.ss_family, get_in_addr((struct sockaddr *) &initial_sock), ipString, sizeof(s));

            printf("this is port: %s\n", portString);
            printf("this is ip: %s\n", ipString);


            // Gets information about available socket types and protocols
            if ((rv = getaddrinfo(NULL, portString, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                exit(1);
            }

            // loop through all the results and bind to the first we can
            for (p = servinfo; p != NULL; p = p->ai_next) {

                // create socket object
                if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    perror("server: socket");
                    continue;
                }

                // specify that, once the program finishes, the port can be reused by other processes
                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                    perror("setsockopt");
                    exit(1);
                }

                // bind to the specified port number
                if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    close(sockfd);
                    perror("server: bind");
                    continue;
                }

                // if the code reaches this point, the socket was properly created and bound
                break;
            }

            // all done with this structure
            freeaddrinfo(servinfo);

            // if p is null, the loop above could create a socket for any given address
            if (p == NULL) {
                fprintf(stderr, "server: failed to bind\n");
                exit(1);
            }

            // sets up a queue of incoming connections to be received by the server
            if (listen(sockfd, 10) == -1) {
                perror("listen");
                exit(1);
            }

            char *ipDot = strtok(ipString, ".");


            int a5Int = portNum / 256;
            int a6Int = portNum - a5Int * 256;
            char a5[5];
            char a6[5];
            sprintf(a5, "%d", a5Int);
            sprintf(a6, "%d", a6Int);
            char output[256] = "227 Entering Passive Mode (";

            while (ipDot != NULL) {
                strcat(output, ipDot);
                ipDot = strtok(NULL, ".");
                strcat(output, ",");
            }

            strcat(output, a5);
            strcat(output, ",");
            strcat(output, a6);
            strcat(output, ").");
            send_string(fd, "%s\r\n", output);

        }else if (strcasecmp(firstArg, "PASV") == 0 && count != 1) {
            send_string(fd, "501 Syntax error in parameters or arguments.\r\n");
        } else if (strcasecmp(firstArg, "NLST") == 0 && (count == 1)) {

            struct sockaddr_storage their_addr; // connector's address information
            socklen_t size = sizeof(struct sockaddr_storage);
            int new_fd;
            char s[INET6_ADDRSTRLEN];

            while(1) {
                // wait for new client to connect
                new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &size);
                if (new_fd == -1) {
                    perror("accept");
                    continue;
                }

                inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr),
                          s, sizeof(s));
                printf("server: got connection from %s\n", s);

                char currentDir[128] = {};
                getcwd(currentDir, 128);

                send_string(new_fd, "150 File status okay; about to open data connection.\r\n");


                listFiles(new_fd, currentDir);
                if (send(new_fd,currentDir,sizeof(currentDir), 0) == -1){
                    perror("couldn't send");
                };
                break;

            }

            } else if (strcasecmp(firstArg, "NLST") == 0 && (count != 1)) {
            send_string(fd, "501 Syntax error in parameters or arguments.\r\n");
        } else {
            send_string(fd, "500 Syntax error, command unrecognized. This may include errors such as command line too long.\r\n");
        }


    }
}

