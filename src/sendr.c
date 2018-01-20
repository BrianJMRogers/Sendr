//##############################################################################
// INCLUDES
//##############################################################################
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <dirent.h>

//##############################################################################
// DEFINES
//##############################################################################
//defaults
#define DEFAULTDOWNLOADLOCATION = "~/Downloads"

// errors
#define SENDFILEERROR "[!] SENDR Client send_file()"
#define MAINERROR "[!] SENDR main():"
#define RECEIVEFILEERROR "[!] SENDR Receiver receive_file()"

// parameters
#define PORTPARAMETERINDICATOR "-p"
#define RECEIVERIPHOSTINDICATOR "-h"
#define FILEPATHINDICATOR "-f"
#define SENDFILEINDICATOR "-s"
#define RECEIVEFILEINDICATOR "-r"

//##############################################################################
// FUNCTION PROTOTYPES
//##############################################################################
int send_file(int *argc, char *argv[]);
int receive_file(int *argc, char *argv[]);
int parse_args(char arg_to_look_for[2], int *argc, char *argv[]);


//##############################################################################
// function: main
// purpose: determine if the user wants to send a file or receive a file
// return value: int > 0 for success, int < 0 for failure
//##############################################################################

//##############################################################################
// FUNCTION: main
//##############################################################################
int main(int argc, char *argv[])
{
    int retval = 1;

    // parse arguments for -s or -r
    if (argc < 2)
    {
        fprintf(stderr, "%s: you must specify wether you want to send or receive a file by setting the first argument to [-s] for sending or [-r] for receiving\n", MAINERROR);
        fflush(stderr);
        goto SENDR_MAIN_ERROR;
    }

    // send to send_file
    if (strcmp(argv[1],"-s") == 0)
    {
        // run send_file
        send_file(&argc, &argv[0]);
    }
    
    // send to receive_file
    else if (strcmp(argv[1],"-r") == 0)
    {
        // recieve file
        receive_file(&argc, &argv[0]);
    }

    // else invalid, send to error
    else {
        fprintf(stderr, "%s: you must specify wether you want to send or receive a file by setting the first argument to [-s] for sending or [-r] for receiving\n", MAINERROR);
        fflush(stderr);
        goto SENDR_MAIN_ERROR;
    }

    if (retval == 1) { goto SENDR_MAIN_EXIT; }
SENDR_MAIN_ERROR:
    fprintf(stderr, "%s: exiting due to error\n", MAINERROR);
    retval = -1;
    
SENDR_MAIN_EXIT:

    return retval;
}

//##############################################################################
// FUNCTION: send_file
//##############################################################################
int
send_file(int *argc, char *argv[])
{
    int                 port = -1;
    int                 port_args_location = -1;
    int                 receiver_ip_args_location = -1;
    int                 file_args_location = -1;
    int                 socket_fd = -1;
    int                 file_fd = -1;
    int                 retval = 1;
    char                *receiver_ip = NULL;
    char                *path_to_file = NULL;
    struct              sockaddr_in receiver_addr;

    // get index location of file path in argv
    if ((file_args_location = parse_args("-f", argc, argv)) == -1)
    {
        fprintf(stderr, "%s: unable to find file argument\n", SENDFILEERROR);
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }
    
    // get index location of receiver_ip in argv
    if ((receiver_ip_args_location = parse_args("-i", argc, argv)) == -1)
    {
        fprintf(stderr, "%s: unable to find receiver IP \n", SENDFILEERROR);
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }

    // get index location of port number in argv
    if ((port_args_location = parse_args("-p", argc, argv)) == -1)
    {
        fprintf(stderr, "%s: unable to find port argument \n", SENDFILEERROR);
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }
    path_to_file = argv[file_args_location];
    receiver_ip= argv[receiver_ip_args_location];
    port = atoi(argv[port_args_location]);

    if (path_to_file == NULL)
    {
        fprintf(stderr, "%s: path to file error\n", SENDFILEERROR);
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }

    if (receiver_ip == NULL)
    {
        fprintf(stderr, "%s: receiver ip error\n", SENDFILEERROR);
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }

    if (port == 0 || port < 0)
    {
        fprintf(stderr, "%s: port error\n", SENDFILEERROR);
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }

    fprintf(stdout, "Path to file: [%s], receiver host ip: [%s]. port number: [%d]\n", path_to_file, receiver_ip, port);
    exit(0);
    // ensure the file path is valid
    if ((file_fd = fopen(path_to_file, "r") == NULL))
    {
        fprintf(stderr, "%s: unable to open file indicated by path. errno [%d]-[%s] \n", SENDFILEERROR, errno, strerror(errno));
    }

    
    // now that we have the path, ip, and port, we are ready to send the file
    // open socket an check for sucess
    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "%s: unable to open socket. errno [%d]-[%s]\n", SENDFILEERROR, errno, strerror(errno));
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }

    // populate sockadder_in receiver_addr
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);

    // convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, receiver_ip, &receiver_addr.sin_addr) <= 0)  
    {
        fprintf(stderr, "%s: unable to convert ip address. errno [%d]-[%s] \n", SENDFILEERROR, errno, strerror(errno));
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }

    // send file to socket
    if (connect(socket_fd, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) < 0)
    {
        fprintf(stderr, "%s: unable to connect to receiver socket. errno [%d]-[%s] \n", SENDFILEERROR, errno, strerror(errno));
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }

    if ((sendfile(file_fd, socket_fd, 0, 0, NULL, 0)) < 0)
    {
        // TODO: fall back to read/write if sendfile fails with EINVAL or ENOSYS (verify we need to do this) 
        fprintf(stderr, "%s: sendfile() failed. errno [%d]-[%s]\n", SENDFILEERROR, errno, strerror(errno));
        fflush(stderr);
        goto SEND_FILE_ERROR;
    }
    
    if (retval != -1) { goto SEND_FILE_EXIT; }
SEND_FILE_ERROR:
    fprintf(stderr, "%s:  exiting due to terminal error\n", SENDFILEERROR);
    fflush(stderr);
    retval = -1;
    
SEND_FILE_EXIT:
    // clean up
    if (socket_fd != -1) { close(socket_fd); }
    if (file_fd != -1) { close(file_fd); }
    return -1;
}

//##############################################################################
// FUNCTION: receive_file
//##############################################################################
int 
receive_file(int *argc, char *argv[])
{
    int             retval = 1;
    int             destination_args_location = -1;
    int             port_args_location = -1;

    // TODO: finish receiving port args
    fprintf(stdout, "receive_file()\n");
    if ((destination_args_location = (parse_args("-d", argc, argv))) == -1)
    {
        // error statement
        // RECEIVEFILEERROR
    }

RECEIVE_FILE_ERROR:

RECEIVE_FILE_EXIT:
    return retval;
}

// -f file -i ip -p port
// -d destination -p port

//##############################################################################
// FUNCTION: parse_args
//##############################################################################
int
parse_args(char arg_to_look_for[2], int *argc, char *argv[])
{
    int retval = -1;

    for (int i = 1; i < *argc - 1; i++)
    {
        fprintf(stdout, "found: [%s]\n", argv[i]);
        if (strcmp(arg_to_look_for, argv[i]) == 0)
        {
            // TODO: verify that we aren't passing back an argument starting with a '-' char 
            retval = i+1;
            goto PARSE_ARGS_EXIT;
        }
    }
    
PARSE_ARGS_EXIT:
    return retval;
}

