#ifndef UTIL_FUNCTIONS_H
#define UTIL_FUNCTIONS_H

#include "data_structures.h"


//##################    LOAD DATA FROM FILES    ########################
void parse_arguments(int argc, char *argv[], ProgramArgs *args);
void load_server_files(ProgramArgs *args, ServerConfig *server_config, ControllerInfo **controllers);
int read_controllers_file(const char *filename, ControllerInfo **controllers);
void read_server_file(const char *filename, ServerConfig *server_config);


int create_udp_socket(int port);
int create_tcp_socket(int port);
char *generate_random_number();
int assign_udp_port();

//##################    PRINTS    ########################
void print_format(int type, const char *s);
void print_server_config(ServerConfig *server_config);
void print_controller_info(ControllerInfo *controller);


//##################   PDU    ########################
PackageTypeUDP get_type_package(char *buffer);

//##################    VALIDATE PDU    ########################
int validate_sub_req(char *buffer, ControllerInfo *controllers, int num_controllers);
int validate_sub_info(char *buffer, ControllerInfo *controller);
int validate_hello(char *buffer, ControllerInfo *controller);

//##################    CREATE PDU    ########################

char *create_pdu_subs_rej(ServerConfig server_config);
char *create_pdu_subs_ack(ServerConfig server_config, ControllerInfo controller);
char *create_pdu_hello_rej(ServerConfig server_config);
char *create_pdu_hello(ServerConfig server_config, ControllerInfo controller);
char *create_pdu_info_ack(ServerConfig server_config, ControllerInfo controller);

//##################    CONTROLLERS    ########################
void disconnect_controller(ControllerInfo *controller);
char *state_to_str(ClientStates state);
void set_state_controller(ControllerInfo *controller, int state);

//##################    OTHERS    ########################
void write_to_buffer(char *buffer, char *value, int initial, int final);
char *read_from_buffer(char *buffer, int initial, int final);
void fill_empty_to_buffer(char *buffer, int initial);


#endif
