#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data_structures.h"
#include "util_functions.h"

// Function to parse program arguments
void parse_arguments(int argc, char *argv[], ProgramArgs *args) {

    strcpy(args->server_file, CONFIG_FILE_DEFAULT);
    args->debug = 0;
    strcpy(args->controllers_file, CONTROLLERS_FILE_DEFAULT);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            args->debug = 1;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            strcpy(args->server_file, argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            strcpy(args->controllers_file, argv[i + 1]);
            i++;
        }
    }
}

int main(int argc, char *argv[]) {
    ProgramArgs args;
    parse_arguments(argc, argv, &args);

    ServerConfig server_config;
    ControllerInfo *controllers;

    initialize_server(&args, &server_config, &controllers);

    
    return 0;
}
