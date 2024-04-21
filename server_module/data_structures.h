#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#define CONFIG_PATH "./config_files/"
#define CONFIG_FILE_DEFAULT "server.cfg"
#define CONTROLLERS_FILE_DEFAULT "controllers.dat"
#define MAC_ADDRESS_LENGTH 13 
#define RANDOM_NUM_LENGTH 9
#define NAME_LENGTH 9
#define DATA_UDP_LENGTH 80
#define PORT_LENGTH 6
#define MAX_UDP_MESSAGE_SIZE 103 //Max UDP packet

#define MAX_UDP_PORT 40000 
#define MIN_UDP_PORT 50000 //10k ports for udp
#define MAX_TCP_PORT 50001
#define MIN_TCP_PORT 60000 //10k ports for tcp

#include <sys/types.h>
#include <sys/socket.h>

typedef struct {
    char name[NAME_LENGTH];   // Server Name
    char mac[MAC_ADDRESS_LENGTH];   // Mac Server
    int udp_port;   // UDP port
    int tcp_port;   // TCP port
    int num_controllers;
} ServerConfig;

typedef enum {
    DISCONNECTED,
    NOT_SUBSCRIBED,
    WAIT_ACK_SUBS,
    WAIT_INFO,
    WAIT_ACK_INFO,
    SUBSCRIBED,
    SEND_HELLO,
    ERORR_STATE
} ClientStates;

typedef struct {
    char name[NAME_LENGTH]; // Controller /client Name
    char situation[DATA_UDP_LENGTH]; //situation
    char mac[MAC_ADDRESS_LENGTH]; // MAC /client 
    char random_num[RANDOM_NUM_LENGTH]; //random number
    int udp_port; //udp port for the client
    int tcp_port; //port tcp from the client
    char elements_data[DATA_UDP_LENGTH]; //controller data from the client
    char data_hello[DATA_UDP_LENGTH];
    ClientStates state;
    pid_t pid_child;
    int socket_child;
} ControllerInfo;

// Structure for arguments
typedef struct {
    char server_file[256];       // Path to server.cfg
    char debug;                  // Debug mode
    char controllers_file[256];  // Path to controllers.dat
} ProgramArgs;


//DATAGRAM TYPE
typedef enum {
    SUBS_REQ,
    SUBS_ACK,
    SUBS_REJ,
    SUBS_INFO,
    INFO_ACK,
    SUBS_NACK,
    HELLO,
    HELLO_REJ,
    ERROR_UDP
} PackageTypeUDP;

#endif
