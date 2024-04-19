#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
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


// Print ServerConfig (test)
void print_server_config(ServerConfig *server_config) {
    printf("Server Name: %s\n", server_config->name);
    printf("Server MAC: %s\n", server_config->mac);
    printf("UDP Port: %d\n", server_config->udp_port);
    printf("TCP Port: %d\n", server_config->tcp_port);
}

// Print ControllerInfo (test)
void print_controller_info(ControllerInfo *controllers, int num_controllers) {
    for (int i = 0; i < num_controllers; i++) {
        printf("Controller Name: %s\n", controllers[i].name);
        printf("Controller MAC: %s\n", controllers[i].mac);
    }
}

//load server file
void read_server_file(const char *filename, ServerConfig *server_config) {
    // Open the file
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        char error_message[100] = "No es pot obrir l'arxiu de configuració:";
        strcat(error_message, filename);
        print_format(2, error_message);
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
        print_format(2, "No es pot obrir la base de dades de controladors");
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
    }

    fclose(file);

    return num_controllers;
}


void initialize_server(ProgramArgs *args, ServerConfig *server_config, ControllerInfo **controllers) {
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