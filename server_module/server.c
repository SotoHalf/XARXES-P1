#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
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

    load_server_files(&args, &server_config, &controllers);

    int udp_socket = create_udp_socket(server_config.udp_port);
    int tcp_socket = create_tcp_socket(server_config.tcp_port);

    
    //MAIN LOOP SUBSCRIPTION
    while (1) {
        //UDP
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        char buffer[MAX_UDP_MESSAGE_SIZE];
        int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer), 0,(struct sockaddr *)&client_addr, &addr_len);
        if (bytes_received == -1) {
            //ERROR RECIVED
            continue;
        }
        //buffer[bytes_received] = '\0';
        
        //SUB_REQ -> Recived
        int type_package = get_type_package(buffer);

        if (type_package == SUBS_REQ) {
            int controller_pos = validate_sub_req(buffer, controllers, server_config.num_controllers);
            //check if controller exists
            if (controller_pos != -1) {
                ControllerInfo controller = controllers[controller_pos];

                if (controller.state == DISCONNECTED){
                    controller.state=WAIT_ACK_INFO;
                    strcpy(controller.random_num, generate_random_number());

                    //Assign a new UDP port for the controller
                    int controller_udp_socket = -1;
                    while (controller_udp_socket == -1){
                        controller.udp_port = assign_udp_port();
                        controller_udp_socket = create_udp_socket(controller.udp_port);
                    }
                    printf("%d\n",controller.udp_port);

                    //valid send subs_ack
                    char *sub_ack = create_pdu_subs_ack(server_config, controller);
                    sendto(udp_socket, sub_ack, strlen(sub_ack), 0,
                        (struct sockaddr *)&client_addr, addr_len);
                    
                    //modificar aquet print
                    print_format(1,"Controlador pasa a l'estat WAIT_ACK_INFO");
                    if (args.debug){
                        print_format(2,"Creació d'un subprocés (FORK)");
                    }
                    pid_t pid = fork();
                    if (pid == -1) { //error
                        continue;
                    } else if (pid == 0) { //child
                        
                        //SUBS_INFO -> Recived
                        char info_buffer[MAX_UDP_MESSAGE_SIZE];
                        int info_bytes_received = recvfrom(controller_udp_socket, info_buffer, sizeof(info_buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
                        if (info_bytes_received == -1) {
                            //ERROR RECIVED
                            continue;
                        }
                        printf("Received SUBS_INFO package content: %s\n", info_buffer);

                        //valid send info_ack

                        //keep comunication hello
                        //When the client is disconnected we must terminate the socket that we opened previously    
                        disconnect_controller(controller);
                        close(controller_udp_socket);
                        exit(0);
                    } else {
                        // Parent
                        print_format(1,"Es valid"); //modificar el print
                        wait(NULL); // no se si el haure de treure al fer un bucle infinit sobre el fill
                    }
                }
                
            } else {
                //no valid
                char *sub_rej = create_pdu_subs_rej(server_config);
                sendto(udp_socket, sub_rej, strlen(sub_rej), 0,
                       (struct sockaddr *)&client_addr, addr_len);
            }
        }
    }

    
    close(udp_socket);
    close(tcp_socket);

    return 0;
}
