#ifndef UTIL_FUNCTIONS_H
#define UTIL_FUNCTIONS_H

#include "data_structures.h"

void read_server_file(const char *filename, ServerConfig *server_config);
int read_controllers_file(const char *filename, ControllerInfo **controllers);
void print_format(int type, const char *s);
void print_server_config(ServerConfig *server_config);
void print_controller_info(ControllerInfo *controllers, int num_controllers);
void initialize_server(ProgramArgs *args, ServerConfig *server_config, ControllerInfo **controllers);

#endif