#ifndef UTIL_FUNCTIONS_H
#define UTIL_FUNCTIONS_H

#include "data_structures.h"

void read_server_file(const char *filename, ServerConfig *server_config);
int read_controllers_file(const char *filename, ControllerInfo **controllers);
void print_format(int type, const char *s);
void print_server_config(ServerConfig *server_config);
void print_controller_info(ControllerInfo *controller);
void load_server_files(ProgramArgs *args, ServerConfig *server_config, ControllerInfo **controllers);
int create_udp_socket(int port);
int create_tcp_socket(int port);
int validate_sub_req(char *buffer, ControllerInfo *controllers, int num_controllers);
int validate_sub_info(char *buffer, ControllerInfo *controller);
char *create_pdu_subs_rej(ServerConfig server_config);
PackageTypeUDP get_type_package(char *buffer);
char *generate_random_number();
char *create_pdu_subs_ack(ServerConfig server_config, ControllerInfo controller);
int assign_udp_port();
void disconnect_controller(ControllerInfo *controller);
void write_to_buffer(char *buffer, char *value, int initial, int final);
char *read_from_buffer(char *buffer, int initial, int final);
int validate_hello(char *buffer, ControllerInfo *controller);
char *create_pdu_hello_rej(ServerConfig server_config);
char *create_pdu_hello(ServerConfig server_config, ControllerInfo controller);
char *state_to_str(ClientStates state);
char *create_pdu_info_ack(ServerConfig server_config, ControllerInfo controller);
void set_data_sockets_pid(ControllerInfo *controller, pid_t pid, int socket);
void avoid_sockets_pid(ControllerInfo *controller);
void set_state_controller(ControllerInfo *controller, int state);
int get_pos_for_pipe_controller(ControllerInfo pipe_controller, ControllerInfo *all_controllers, int num_controllers);
#endif
