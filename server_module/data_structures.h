#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#define CONFIG_PATH "./config_files/"
#define CONFIG_FILE_DEFAULT "server.cfg"
#define CONTROLLERS_FILE_DEFAULT "controllers.dat"
#define MAC_ADDRESS_LENGTH 13 
#define RANDOM_NUM_LENGTH 9
#define REASON_LENGTH 80
#define MAX_UDP_MESSAGE_SIZE 118 //Max UDP packet

typedef struct {
    char name[9];   // Server Name
    char mac[MAC_ADDRESS_LENGTH];   // Mac Server
    int udp_port;   // UDP port
    int tcp_port;   // TCP port
    int num_controllers;
} ServerConfig;

typedef struct {
    char name[9]; // Controller /client Name
    char mac[MAC_ADDRESS_LENGTH]; // MAC /client 
    int state;
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
