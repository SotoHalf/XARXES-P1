#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h> 
#include <pthread.h> 
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

void show_state_client(ControllerInfo *controller){
    char msg[100];
    sprintf(msg, "Controlador: %s, passa a l'estat: %s", controller->name, state_to_str(controller->state));
    print_format(1,msg);
}

ControllerInfo *controllers;
ControllerInfo controller;
ServerConfig server_config;
int run_command_thread = 1;


void print_controllers_state() {
    printf("%s\t", controller.name);
    
    // Print IP address
    printf("127.0.0.1:%d\t", controller.udp_port);
    // Print MAC address
    printf("%s\t", controller.mac);
    
    // Print Random number
    printf("%s\t", controller.random_num);

    // Print State
    printf("%s\t", state_to_str(controller.state));

    // Print Situation
    printf("%s\t", controller.situation);

    // Print Elements
    printf("%s\t", controller.elements_data);
    printf("\n");
    
}

void disconnect_controllers() {
    for (int i = 0; i < server_config.num_controllers; i++) {
        if (controllers[i].socket_child != 0) {
            close(controllers[i].socket_child); // close sokets
        }
        
    }
    free(controllers);
    exit(0);
}


void *command_handler(void *arg) {
    pid_t parent_pid = *((pid_t*)arg); 
    char command[10];

    while (1) {
        if (!run_command_thread) {
            break;
        }
        scanf("%s", command);
        if (strcmp(command, "quit") == 0) {
            kill(parent_pid, SIGINT);
            pthread_exit(NULL);
        } else if (strcmp(command, "list") == 0) {
            kill(0,SIGUSR1);
            printf("\n");
        }
    }
    return NULL;
}

// Función para manejar la señal SIGINT (Ctrl+C)
void sigint_handler(int sig) {
    if (run_command_thread) {
        kill(0, SIGINT);
        disconnect_controllers(); //parent close sockets
    }else{
        kill(getpid(), SIGUSR1); //child kill
    }
}

void signint_print_state(int sig){
    if (run_command_thread) {
        printf("--NOM--- ------IP------- -----MAC---- --RNDM-- ----ESTAT--- --SITUACIÓ-- --ELEMENTS-------------------------------------------\n");
    }else{
        sleep(1);
        print_controllers_state(); //child print
    }
}

int main(int argc, char *argv[]) {
    ProgramArgs args;
    parse_arguments(argc, argv, &args);

    load_server_files(&args, &server_config, &controllers);

    int main_pid = getpid();

    pthread_t commands_thread;
    if (pthread_create(&commands_thread, NULL, command_handler, (void*)&main_pid) != 0) {
        exit(1);
    }

    signal(SIGINT, sigint_handler); //terminate
    signal(SIGUSR1, signint_print_state); //stat

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
        buffer[bytes_received] = '\0';
        
        //SUB_REQ -> Recived
        int type_package = get_type_package(buffer);

        if (type_package == SUBS_REQ) {
            int controller_pos = validate_sub_req(buffer, controllers, server_config.num_controllers);
            //check if controller exists
            if (controller_pos != -1) {
                //ControllerInfo controller = controllers[controller_pos];
                controller = controllers[controller_pos];

                if (controller.state == DISCONNECTED){
                    controller.state=WAIT_INFO;
                    strcpy(controller.random_num, generate_random_number());

                    //Assign a new UDP port for the controller
                    //in case that the udp port is in use retry creating a socket
                    int controller_udp_socket = -1;
                    while (controller_udp_socket == -1){
                        controller.udp_port = assign_udp_port();
                        controller_udp_socket = create_udp_socket(controller.udp_port);
                        if (controller_udp_socket == -1) {
                            close(controller_udp_socket);
                        }
                    }


                    // Set up timeout
                    struct timeval timeout;
                    timeout.tv_sec = 2;
                    timeout.tv_usec = 0;
                    if (setsockopt(controller_udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == -1) {
                        disconnect_controller(&controller);
                        close(controller_udp_socket);
                        exit(0);
                    }
                    controller.socket_child = controller_udp_socket;

                    //valid send subs_ack
                    char *sub_ack = create_pdu_subs_ack(server_config, controller);
                    sendto(udp_socket, sub_ack, MAX_UDP_MESSAGE_SIZE, 0,
                        (struct sockaddr *)&client_addr, addr_len);

                    show_state_client(&controller);
                    if (args.debug){
                        print_format(2,"Creació d'un subprocés (FORK)");
                    }

                    //avoid client to have multiple subs
                    avoid_sockets_pid(&controller);
            
                    //int status;
                    pid_t pid = fork();
                    if (pid == -1) { //error
                        continue;
                    } else if (pid == 0) { //child
                        run_command_thread = 0;
                        int info_valid_received = 0;
                        int s = 2; // retries
                        while (s > 0 && !info_valid_received) {
                            char info_buffer[MAX_UDP_MESSAGE_SIZE];
                            int info_bytes_received = recvfrom(controller_udp_socket, info_buffer, sizeof(info_buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
                            if (info_bytes_received == -1) {
                                s--;
                                continue;
                            }
                            info_buffer[info_bytes_received] = '\0';
                            type_package = get_type_package(info_buffer);
                            
                            if (type_package == SUBS_INFO) {
                                if (validate_sub_info(info_buffer, &controller)) {
                                    info_valid_received = 1;
                                } else {
                                    //failed send rejected
                                    char *sub_rej = create_pdu_subs_rej(server_config);
                                    sendto(controller_udp_socket, sub_rej, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr, addr_len);
                                    disconnect_controller(&controller);
                                    show_state_client(&controller);
                                    close(controller_udp_socket);
                                    exit(0);
                                }
                            }
                            
                            s--;
                            sleep(1);
                        }
                        
                        if (info_valid_received) {
                            //sned info ack
                            char *info_ack = create_pdu_info_ack(server_config, controller);
                            sendto(controller_udp_socket, info_ack, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr, addr_len);
                            controller.state=SUBSCRIBED;
                            show_state_client(&controller);

                            //keep hello here
                            int x = 3; //attemps lost package
                            int first = 1;
                            while ((controller.state == SUBSCRIBED && x > 0) || (controller.state == SEND_HELLO && x > 0)){
                                char hello_buffer[MAX_UDP_MESSAGE_SIZE];
                                int hello_bytes_received = recvfrom(controller_udp_socket, hello_buffer, sizeof(hello_buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
                                if (hello_bytes_received == -1) {
                                    x--;
                                    continue;
                                }
                                hello_buffer[hello_bytes_received] = '\0';
                                type_package = get_type_package(hello_buffer);
                                
                                if (type_package == HELLO) {
                                    
                                    if (validate_hello(hello_buffer, &controller)) {
                                        if (first){
                                            controller.state = SEND_HELLO;
                                            show_state_client(&controller);
                                            first = 0;
                                        }

                                        x = 3;
                                        char *hello = create_pdu_hello(server_config, controller);
                                        sendto(controller_udp_socket, hello, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr, addr_len);
                                        if (args.debug){
                                            char msg_hello_good[100];
                                            sprintf(msg_hello_good, "Controlador %s ha enviat hello correctament", controller.name);
                                            print_format(2,msg_hello_good);
                                        }
                                        //print_controller_info(&controller);
                                        
                                    } else {
                                        char msg_helo_supl[100];
                                        sprintf(msg_helo_supl, "Controlador %s [%s] ha sigut suplantat", controller.name, controller.mac);
                                        print_format(1,msg_helo_supl);
                                        //failed send rejected
                                        char *hello_rej = create_pdu_hello_rej(server_config);
                                        sendto(controller_udp_socket, hello_rej, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr, addr_len);
                                        disconnect_controller(&controller);
                                        show_state_client(&controller);
                                        close(controller_udp_socket);
                                        exit(0);
                                    }
                                }else if (type_package == HELLO_REJ) {
                                    
                                    char msg_hello_rej[100];
                                    sprintf(msg_hello_rej, "Controlador %s [%s] ha enviat un HELLO_REJ", controller.name, controller.mac);
                                    print_format(1,msg_hello_rej);
                                    disconnect_controller(&controller);
                                    show_state_client(&controller);
                                    close(controller_udp_socket);
                                    exit(0);
                                } else{
                                    x--;
                                }
                                sleep(1);
                            }

                            if (x <= 0){
                                char msg_hello_timeout[100];
                                sprintf(msg_hello_timeout, "Controlador %s [%s] no ha rebut 3 HELLO consecutius", controller.name, controller.mac);
                                print_format(1,msg_hello_timeout);
                                disconnect_controller(&controller);
                                show_state_client(&controller);
                                close(controller_udp_socket);
                                exit(0);
                            }else{
                                close(controller_udp_socket);
                                exit(0);
                            }
                        }
                        
                    } else {
                        // Parent
                        //waitpid(pid, &status, 0);
                    }
                }
                
            } else {
                //no valid
                char *sub_rej = create_pdu_subs_rej(server_config);
                sendto(udp_socket, sub_rej, MAX_UDP_MESSAGE_SIZE, 0,
                       (struct sockaddr *)&client_addr, addr_len);
            }
        }
    }

    
    close(udp_socket);
    close(tcp_socket);

    return 0;
}
