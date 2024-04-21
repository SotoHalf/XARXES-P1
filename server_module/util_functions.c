#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include "util_functions.h"
#include "data_structures.h"
#include "util_functions.h"

void print_format(int type, const char *s) {
    const char *type_str;
    
    switch (type) {
        case 1:
            type_str = "MSG";
            break;
        case 2:
            type_str = "DEBUG";
            break;
        case 3:
            type_str = "ERROR";
            break;
    }
    time_t now;
    struct tm *info;
    char time_str[9];
    time(&now);
    info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", info);
    printf("%s: %s => %s\n", time_str, type_str, s);
}

void disconnect_controller(ControllerInfo controller){
    controller.state=DISCONNECTED;
    strcpy(controller.random_num, "00000000");
}

int assign_udp_port() {
    srand(time(NULL));
    int udp_port = rand() % (MAX_UDP_PORT - MIN_UDP_PORT + 1) + MIN_UDP_PORT;
    return udp_port;
}

char *state_to_str(ClientStates state) {
    switch (state) {
        case DISCONNECTED:
            return "DISCONNECTED";
        case NOT_SUBSCRIBED:
            return "NOT_SUBSCRIBED";
        case WAIT_ACK_SUBS:
            return "WAIT_ACK_SUBS";
        case WAIT_INFO:
            return "WAIT_INFO";
        case WAIT_ACK_INFO:
            return "WAIT_ACK_INFO";
        case SUBSCRIBED:
            return "SUBSCRIBED";
        case SEND_HELLO:
            return "SEND_HELLO";
        case ERORR_STATE:
            return "ERROR_STATE";
        default:
            return "UNKNOWN";
    }
}

//Method for test the controller
void print_controller_info(ControllerInfo *controller) {
    printf("Controller Info:\n");
    printf("MAC Address: %s\n", controller->mac);
    printf("Random Number: %s\n", controller->random_num);
    printf("TCP Port: %d\n", controller->tcp_port);
    printf("Elements Data: %s\n", controller->elements_data);
    printf("Hello Data: %s\n", controller->data_hello);   
}

//Method for test the creation of packages
void print_pdu_test(char *pack) {
    for (int i = 0; i < MAX_UDP_MESSAGE_SIZE; i++) {
        printf("%02X ", (unsigned char)pack[i]);
    }
    printf("\n");
}

//Method for test the values
void print_val(char *val, int limit) {
    for (int i = 0; i < limit; i++) {
        printf("%d%c\n", i,val[i]);
    }
    printf("\n");
}

void fill_empty_to_buffer(char *buffer, int initial) {
    for (int i = initial; i < MAX_UDP_MESSAGE_SIZE; i++) {
        buffer[i] = '\0';
    }
}

void write_to_buffer(char *buffer, char *value, int initial, int final) {
    int value_length = final - initial;
    for (int i = 0; i < value_length; i++) {
        if (i < strlen(value)) {
            buffer[initial + i] = value[i];
        } else {
            buffer[initial + i] = '\0';
        }
    }
}

char *read_from_buffer(char *buffer, int initial, int final) {
    char *res = (char *)malloc(final);
    int i = 0;
    for (i = 0; i < final; i++) {
        res[i] = buffer[initial + i];
    }
    res[i] = '\0';
    return res;
}



// check HELLO
int validate_hello(char *buffer, ControllerInfo *controller){
    int total_bytes = 1; //already writen
    char *mac_to_check = read_from_buffer(buffer, total_bytes, MAC_ADDRESS_LENGTH);    
    
    total_bytes += MAC_ADDRESS_LENGTH;  //number of readed bytes
    char *number_to_check = read_from_buffer(buffer, total_bytes, RANDOM_NUM_LENGTH);
    
    //Data from hello
    total_bytes += RANDOM_NUM_LENGTH;  //number of readed bytes
    char *data_hello_temp = read_from_buffer(buffer, total_bytes, DATA_UDP_LENGTH);
    
    // check if the mac and random_number match with the controller
    if ((strcmp(mac_to_check, controller->mac) == 0) && (strcmp(number_to_check, controller->random_num) == 0)) {
        //valid Hello
        strncpy(controller->data_hello, data_hello_temp, DATA_UDP_LENGTH);
        //controller.data_hello[DATA_UDP_LENGTH] = '\0';
        return 1;
    }
    
    //invalid Hello
    return 0;
}



// check INFO_ACK
int validate_sub_info(char *buffer, ControllerInfo *controller){

    int total_bytes = 1; //already writen
    //char mac_to_check[MAC_ADDRESS_LENGTH];
    //read_from_buffer(buffer, total_bytes, MAC_ADDRESS_LENGTH, mac_to_check);
    char *mac_to_check = read_from_buffer(buffer, total_bytes, MAC_ADDRESS_LENGTH);
        
    total_bytes += MAC_ADDRESS_LENGTH;  //number of readed bytes
    //char number_to_check[RANDOM_NUM_LENGTH];
    //read_from_buffer(buffer, total_bytes, RANDOM_NUM_LENGTH, number_to_check);
    char *number_to_check = read_from_buffer(buffer, total_bytes, RANDOM_NUM_LENGTH);    
    
    //TCP PORT is before a ',' so read until reach one the rest go for elements
    int i = 0;
    int c = 0;
    int len_ele = 0;
    int skip = 1;
    total_bytes += RANDOM_NUM_LENGTH;  //number of readed bytes

    char *port_tcp_client = (char *)malloc(PORT_LENGTH);
    char *elements_client = (char *)malloc(DATA_UDP_LENGTH);
    //&& i < (total_bytes + DATA_UDP_LENGTH) 
    
    while (buffer[total_bytes + i] != '\0') {
        if (buffer[total_bytes + i] == ',') skip = 0;
        if (i < PORT_LENGTH && skip){
            port_tcp_client[i] = buffer[total_bytes + i];
            c++;
        }else{
            elements_client[i-c-1] = buffer[total_bytes + i]; //skip , cause we don't do -1
            //controller.elements_data[i-c-1] = buffer[total_bytes + i];
            len_ele++;
        }
        i++;
    }
    

    port_tcp_client[PORT_LENGTH] = '\0';
    //elements_client[i-c] = '\0';
    elements_client[len_ele] = '\0';
    
    // check if the mac and random_number match with the controller
    if ((strcmp(mac_to_check, controller->mac) == 0) && (strcmp(number_to_check, controller->random_num) == 0)) {
        //valid INFO_ACK
        //client sent his own tcp port to communicate
        controller->tcp_port = atoi(port_tcp_client); //to int
        strcpy(controller->elements_data, elements_client);
        
        //strncpy(controller.elements_data, elements_client, DATA_UDP_LENGTH);
        //controller.elements_data[DATA_UDP_LENGTH] = '\0';
        //print_val(controller.elements_data,DATA_UDP_LENGTH);
        return 1;
    }
    
    
    //invalid INFO_ACK
    return 0;
}

// check SUB_REQ package
int validate_sub_req(char *buffer, ControllerInfo *controllers, int num_controllers) {

    //MAC
    int total_bytes = 1; //already writen
    char mac_to_check[MAC_ADDRESS_LENGTH];
    strncpy(mac_to_check, buffer + total_bytes, MAC_ADDRESS_LENGTH);
    mac_to_check[MAC_ADDRESS_LENGTH] = '\0';

    //NAME
    total_bytes += MAC_ADDRESS_LENGTH; 
    char name_to_check[NAME_LENGTH];

    //Name is before a ',' so read until reach one
    int i = 0;
     total_bytes += RANDOM_NUM_LENGTH;  //number of readed bytes
    while (buffer[total_bytes + i] != ',' && i < 8) {
        name_to_check[i] = buffer[total_bytes + i];
        i++;
    }
    name_to_check[i] = '\0';

    // check if the mac and name match any controller
    for (int i = 0; i < num_controllers; i++) {
        if (strcmp(mac_to_check, controllers[i].mac) == 0 && strcmp(name_to_check, controllers[i].name) == 0) {
            //valid SUB_REQ
            return i;
        }
    }

    //invalid SUB_REQ    
    return -1;
}

//create hello_rej
char *create_pdu_hello_rej(ServerConfig server_config){
    char *hello_rej = (char *)malloc(MAX_UDP_MESSAGE_SIZE * sizeof(char));
    // Type
    hello_rej[0] = 0x11;
    //fill with empty
    int total_bytes = 1;
    fill_empty_to_buffer(hello_rej, total_bytes);
    return hello_rej;
}

//create hello
char *create_pdu_hello(ServerConfig server_config, ControllerInfo controller){
    char *hello = (char *)malloc(MAX_UDP_MESSAGE_SIZE * sizeof(char));
    
    // Type
    hello[0] = 0x10;
    //MAC Address
    int total_bytes = 1; //already writen
    write_to_buffer(hello, server_config.mac, total_bytes, total_bytes+MAC_ADDRESS_LENGTH);
    //Random number
    char *r_num = controller.random_num;
    total_bytes += MAC_ADDRESS_LENGTH;
    write_to_buffer(hello, r_num, total_bytes, total_bytes+RANDOM_NUM_LENGTH);
    //data
    total_bytes += RANDOM_NUM_LENGTH;
    write_to_buffer(hello, controller.data_hello, total_bytes, total_bytes+strlen(controller.data_hello));
    //fill with empty
    total_bytes += strlen(controller.data_hello);
    fill_empty_to_buffer(hello, total_bytes);

    return hello;
}

//Create info_ack
char *create_pdu_info_ack(ServerConfig server_config, ControllerInfo controller){
    char *info_ack = (char *)malloc(MAX_UDP_MESSAGE_SIZE * sizeof(char));
    
    // Type
    info_ack[0] = 0x04;
    //MAC Address
    int total_bytes = 1; //already writen
    write_to_buffer(info_ack, server_config.mac, total_bytes, total_bytes+MAC_ADDRESS_LENGTH);
    //Random number
    char *r_num = controller.random_num;
    total_bytes += MAC_ADDRESS_LENGTH;
    write_to_buffer(info_ack, r_num, total_bytes, total_bytes+RANDOM_NUM_LENGTH);
    //PortTCP
    char new_port_udp[PORT_LENGTH];
    snprintf(new_port_udp, PORT_LENGTH, "%d", controller.tcp_port);
    total_bytes += RANDOM_NUM_LENGTH;
    write_to_buffer(info_ack, new_port_udp, total_bytes, total_bytes+PORT_LENGTH);
    //fill with empty
    total_bytes += PORT_LENGTH;
    fill_empty_to_buffer(info_ack, total_bytes);

    return info_ack;
}

//Create SUBS_ACK
char *create_pdu_subs_ack(ServerConfig server_config, ControllerInfo controller){
    char *sub_ack = (char *)malloc(MAX_UDP_MESSAGE_SIZE * sizeof(char));
    
    // Type
    sub_ack[0] = 0x01;
    //MAC Address
    int total_bytes = 1; //already writen
    write_to_buffer(sub_ack, server_config.mac, total_bytes, total_bytes+MAC_ADDRESS_LENGTH);
    //Random number
    char *r_num = controller.random_num;
    total_bytes += MAC_ADDRESS_LENGTH;
    write_to_buffer(sub_ack, r_num, total_bytes, total_bytes+RANDOM_NUM_LENGTH);
    //PortUdp
    char new_port_udp[PORT_LENGTH];
    snprintf(new_port_udp, PORT_LENGTH, "%d", controller.udp_port);
    total_bytes += RANDOM_NUM_LENGTH;
    write_to_buffer(sub_ack, new_port_udp, total_bytes, total_bytes+PORT_LENGTH);
    //fill with empty
    total_bytes += PORT_LENGTH;
    fill_empty_to_buffer(sub_ack, total_bytes);

    return sub_ack;
}

// create SUBS_REJ package
char *create_pdu_subs_rej(ServerConfig server_config) {
    char *sub_rej = (char *)malloc(MAX_UDP_MESSAGE_SIZE * sizeof(char));
    
    // Type
    sub_rej[0] = 0x02;
    //MAC Address
    int total_bytes = 1; //already writen
    write_to_buffer(sub_rej, server_config.mac, total_bytes, total_bytes+MAC_ADDRESS_LENGTH);
    //Random number
    char *r_num = "00000000";
    total_bytes += MAC_ADDRESS_LENGTH;
    write_to_buffer(sub_rej, r_num, total_bytes, total_bytes+RANDOM_NUM_LENGTH);
    //Reason
    char *reason = "Rebuig de subscripció dades incorrectes";
    total_bytes += RANDOM_NUM_LENGTH;
    write_to_buffer(sub_rej, reason, total_bytes, total_bytes+strlen(reason));
    //fill with empty
    total_bytes += strlen(reason);
    fill_empty_to_buffer(sub_rej, total_bytes);

    return sub_rej;
}

//random num for clients
char *generate_random_number() {
    srand(time(NULL));
    int random_num = rand() % 100000000;
    char *random_str = (char *)malloc((RANDOM_NUM_LENGTH + 1) * sizeof(char));

    //pass num to str only 8 digits
    sprintf(random_str, "%08d", random_num);
    return random_str;
}

PackageTypeUDP get_type_package(char *buffer) {
    //check first byte and retorn the type of the datagram
    switch (buffer[0]) {
        case 0x00: return SUBS_REQ;
        case 0x01: return SUBS_ACK;
        case 0x02: return SUBS_REJ;
        case 0x03: return SUBS_INFO;
        case 0x04: return INFO_ACK;
        case 0x05: return SUBS_NACK;
        case 0x10: return HELLO;
        case 0x11: return HELLO_REJ;
        default: return ERROR_UDP;
    }
}

int create_udp_socket(int port) {
    int udp_socket;

    // UDP 
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        print_format(3,"No ha sigut possible crear un socket UDP");
        exit(1);
    }

    // Server address structure
    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_server.sin_port = htons(port);

    // Bind socket to the server address
    if (bind(udp_socket, (struct sockaddr *)&addr_server, sizeof(addr_server)) == -1) {
        //print_format(3,"No ha sigut possible fer un bind del socket UDP");
        //exit(1);
        return -1;
    }

    return udp_socket;
}

// Function to create a TCP socket
int create_tcp_socket(int port) {
    int tcp_socket;

    // Create TCP socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1) {
        print_format(3,"No ha sigut possible crear un socket TCP");
        exit(1);
    }

    // Prepare server address structure
    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_server.sin_port = htons(port);

    // Bind socket to the server address
    if (bind(tcp_socket, (struct sockaddr *)&addr_server, sizeof(addr_server)) == -1) {
        print_format(3,"No ha sigut possible fer un bind del socket TCP");
        exit(1);
    }

    // Queue wait | Max 5 connections
    if (listen(tcp_socket, 5) == -1) {
        print_format(3,"No ha sigut possible fer un listen del socket TCP");
        exit(1);
    }

    return tcp_socket;
}

// Print ServerConfig (test)
void print_server_config(ServerConfig *server_config) {
    printf("Server Name: %s\n", server_config->name);
    printf("Server MAC: %s\n", server_config->mac);
    printf("UDP Port: %d\n", server_config->udp_port);
    printf("TCP Port: %d\n", server_config->tcp_port);
}

//load server file
void read_server_file(const char *filename, ServerConfig *server_config) {
    // Open the file
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        char error_message[100] = "No es pot obrir l'arxiu de configuració:";
        strcat(error_message, filename);
        print_format(3, error_message);
        exit(1);
    }

    // Set server configuration parameters
    fscanf(file, "Name = %8s\n", server_config->name);
    fscanf(file, "MAC = %12s\n", server_config->mac);
    fscanf(file, "UDP-port = %d\n", &server_config->udp_port);
    fscanf(file, "TCP-port = %d\n", &server_config->tcp_port);
    fclose(file);
}

//load controller file
int read_controllers_file(const char *filename, ControllerInfo **controllers) {
    // Open the file
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        print_format(3, "No es pot obrir la base de dades de controladors");
        exit(1);
    }

    // Count the number of controllers in the file
    int num_controllers = 0;
    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        num_controllers++;
    }

    // Save memory for the array of controllers
    *controllers = malloc(num_controllers * sizeof(ControllerInfo));
    rewind(file); //Go back at the start of the file

    for (int i = 0; i < num_controllers; i++) {
        // Set controller information from the file
        //%[^,] stop reading when reaching a ,
        //also then %s read string after ,
        fscanf(file, "%[^,],%s\n", (*controllers)[i].name, (*controllers)[i].mac);
        (*controllers)[i].state = DISCONNECTED; //disconnected
        strcpy((*controllers)[i].random_num, "00000000"); // set the random num     
    }

    fclose(file);

    return num_controllers;
}


void load_server_files(ProgramArgs *args, ServerConfig *server_config, ControllerInfo **controllers) {
    //Get config path
    char file_path[100];
    strcpy(file_path, CONFIG_PATH);

    //Get Server Path
    char file_path_server[100];
    strcpy(file_path_server, file_path);
    strcat(file_path_server, args->server_file);

    //Get Controllers Path
    char file_path_controllers[100];
    strcpy(file_path_controllers, file_path);
    strcat(file_path_controllers, args->controllers_file);

    //Read server config
    read_server_file(file_path_server, server_config);

    // Read controllers
    int num_controllers = read_controllers_file(file_path_controllers, controllers);
    server_config->num_controllers=num_controllers;

    //print_server_config(server_config);
    //print_controller_info(*controllers, num_controllers);
}