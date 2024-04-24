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


//##################    GLOBAL VALUES    ########################

ControllerInfo *controllers; //Save all data relative to the client
ServerConfig server_config; //Server configuration
int udp_socket;
int tcp_socket;

//##################    PRINTS    ########################

// Controller -> Print state 
void show_state_client(ControllerInfo *controller){
    char msg[100];
    sprintf(msg, "Controlador: %s, passa a l'estat: %s", controller->name, state_to_str(controller->state));
    print_format(1,msg);
}

//List function print data formatted
void print_controllers_state() {
    printf("--NOM--- ------IP------- -----MAC---- --RNDM-- ----ESTAT--- --SITUACIÓ-- --ELEMENTS-------------------------------------------\n");
    for (int i = 0; i < server_config.num_controllers; i++) {
        printf("%s", controllers[i].name);
        printf("     ");

        // Print IP address
        if (controllers[i].state != DISCONNECTED)
            printf("127.0.0.1:%d", controllers[i].udp_port);
        else
            printf("%s", "       - ");

        printf(" ");

        // Print MAC address
        if (controllers[i].state != DISCONNECTED)
            printf("%s", controllers[i].mac);
        else
            printf("%s", "              - ");

        printf(" ");

        // Print Random number
        if (controllers[i].state != DISCONNECTED)
            printf("%s", controllers[i].random_num);
        else
            printf("%s", "        - ");

        printf(" ");

        // Print State
        printf("%s", state_to_str(controllers[i].state));

        printf("   ");

        // Print Situation
        if (controllers[i].state != DISCONNECTED)
            printf("%s", controllers[i].situation);
        else
            printf("%s", "             ");

        printf(" ");

        // Print Elements
        if (controllers[i].state != DISCONNECTED)
            printf("%s", controllers[i].elements_data);
        
        printf("\n");
    }
}

//##################    FUNCTIONS    ########################

//Close all opened socket and free memory
void disconnect_controllers() {
    for (int i = 0; i < server_config.num_controllers; i++) {
        close(controllers[i].socket_child); // close sokets
    }
    free(controllers);
    exit(0);
}

//##################    THREADS    ########################

//Thread used to handle commands
void *command_handler(void *arg) {
    pid_t parent_pid = *((pid_t*)arg); 

    char command[10];
    while (1) {
        scanf("%s", command);
        if (strcmp(command, "quit") == 0) {
            kill(parent_pid, SIGINT);
            pthread_exit(NULL);
        } else if (strcmp(command, "list") == 0) {
            print_controllers_state();
        }
    }
}

//Thread struct 
typedef struct {
    int controller_udp_socket;
    int pos;
    int debug;
} ControllerHandlerArgs;

//Used to handle with multiple clients
void *controller_handler(void *arg) {
    ControllerHandlerArgs *args = (ControllerHandlerArgs *)arg;

    //Get the controller from global array
    ControllerInfo *controller;
    controller = &controllers[args->pos];

    controller->state = SUBSCRIBED;

    struct sockaddr_in client_addr_child;
    socklen_t addr_len_child = sizeof(client_addr_child);


    //SUBS_INFO -> process
    int info_valid_received = 0;
    int s = 2; // retries
    while (s > 0 && !info_valid_received) {
        char info_buffer[MAX_UDP_MESSAGE_SIZE];
        int info_bytes_received = recvfrom(controller->socket_child, info_buffer, sizeof(info_buffer), 0, (struct sockaddr *)&client_addr_child, &addr_len_child);
        if (info_bytes_received == -1) {
            s--;
            continue;
        }
        info_buffer[info_bytes_received] = '\0';

        int type_package = get_type_package(info_buffer);
        //In case client response with a SUBS_INFO datagram
        if (type_package == SUBS_INFO) {
            if (validate_sub_info(info_buffer, controller)) {
                info_valid_received = 1;
                //it's valid
            } else {
                //failed send rejected
                char *sub_rej = create_pdu_subs_rej(server_config);
                sendto(controller->socket_child, sub_rej, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr_child, addr_len_child);
                //Close section
                disconnect_controller(controller);
                show_state_client(controller);
                close(controller->socket_child);
                return NULL;
            }
        }
        
        s--;
        sleep(1);
    }
    
    //In case that the SUBS_INFO is valid
    if (info_valid_received) {
        //sned info ack
        char *info_ack = create_pdu_info_ack(server_config, *controller);
        sendto(controller->socket_child, info_ack, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr_child, addr_len_child);
        set_state_controller(controller, SUBSCRIBED);
        show_state_client(controller);

        //Keep hello communications
        int x = 3; //attemps lost package
        int first = 1; //know if it's the first hello
        //While controller = SUBSCRIBED|SEND_HELLO
        while ((controller->state == SUBSCRIBED && x > 0) || (controller->state == SEND_HELLO && x > 0)){

            //READ HELLO 
            char hello_buffer[MAX_UDP_MESSAGE_SIZE];
            int hello_bytes_received = recvfrom(controller->socket_child, hello_buffer, sizeof(hello_buffer), 0, (struct sockaddr *)&client_addr_child, &addr_len_child);
            if (hello_bytes_received == -1) {
                x--;
                continue;
            }
            hello_buffer[hello_bytes_received] = '\0';
            int type_package = get_type_package(hello_buffer);
            
            if (type_package == HELLO) {
                
                if (validate_hello(hello_buffer, controller)) {
                    //In case to be valid and the first time we recive 
                    //hello, we must change the state to SEND_HELLO
                    if (first){
                        set_state_controller(controller, SEND_HELLO);
                        show_state_client(controller);
                        first = 0;
                    }

                    x = 3; //Reset retries cause we just recived a valid hello
                    char *hello = create_pdu_hello(server_config, *controller);
                    sendto(controller->socket_child, hello, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr_child, addr_len_child);
                    
                    if (args->debug){
                        char msg_hello_good[100];
                        sprintf(msg_hello_good, "Controlador %s ha enviat hello correctament", controller->name);
                        print_format(2,msg_hello_good);
                    }

                } else {
                    //REJECTED
                    char msg_helo_supl[100];
                    sprintf(msg_helo_supl, "Controlador %s [%s] ha sigut suplantat", controller->name, controller->mac);
                    print_format(1,msg_helo_supl);
                    //failed send rejected
                    char *hello_rej = create_pdu_hello_rej(server_config);
                    sendto(controller->socket_child, hello_rej, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr_child, addr_len_child);
                    //Close section
                    disconnect_controller(controller);
                    show_state_client(controller);
                    close(controller->socket_child);
                    return NULL;
                }
            }else if (type_package == HELLO_REJ) {
                //REJECTED
                char msg_hello_rej[100];
                sprintf(msg_hello_rej, "Controlador %s [%s] ha enviat un HELLO_REJ", controller->name, controller->mac);
                print_format(1,msg_hello_rej);
                //Close section
                disconnect_controller(controller);
                show_state_client(controller);
                close(controller->socket_child);
                return NULL;
            } else{
                x--;
            }
            sleep(1);
        }

        if (x <= 0){
            //REJECTED TIMEOUT
            char msg_hello_timeout[100];
            sprintf(msg_hello_timeout, "Controlador %s [%s] no ha rebut 3 HELLO consecutius", controller->name, controller->mac);
            print_format(1,msg_hello_timeout);
            //Close section
            disconnect_controller(controller);
            show_state_client(controller);
            close(controller->socket_child);
            return NULL;
        }else{
            //Close section
            close(controller->socket_child);
            return NULL;
        }
    }
    
    return NULL;
}

//##################    SIGNALS    ########################

// SIGINT (Ctrl+C) and Quit
void sigint_handler(int sig) {
    kill(0, SIGINT);
    disconnect_controllers(); //parent close sockets
}

//##################    MAIN    ########################

int main(int argc, char *argv[]) {

    //load arguments passed by user
    ProgramArgs args;
    parse_arguments(argc, argv, &args);
    load_server_files(&args, &server_config, &controllers);
    
    
    //THREAD to handle commands
    int main_pid = getpid();
    pthread_t commands_thread;
    if (pthread_create(&commands_thread, NULL, command_handler, (void*)&main_pid) != 0) {
        exit(1);
    }

    //Signals
    signal(SIGINT, sigint_handler);

    //SOCKETS INITIALIZATION
    udp_socket = create_udp_socket(server_config.udp_port);
    tcp_socket = create_tcp_socket(server_config.tcp_port);
    
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
            //check if controller exists and if it's valid
            int controller_pos = validate_sub_req(buffer, controllers, server_config.num_controllers);
            
            if (controller_pos != -1) {
                ControllerInfo *controller = &controllers[controller_pos];

                if (controller->state == DISCONNECTED){
                    
                    strcpy(controller->random_num, generate_random_number());

                    //Assign a new UDP port for the controller
                    //in case that the udp port is in use retry creating a socket
                    int controller_udp_socket = -1;
                    while (controller_udp_socket == -1){
                        controller->udp_port = assign_udp_port();
                        controller_udp_socket = create_udp_socket(controller->udp_port);
                        if (controller_udp_socket == -1) {
                            close(controller_udp_socket);
                        }
                    }


                    // Set up timeout
                    struct timeval timeout;
                    timeout.tv_sec = 2;
                    timeout.tv_usec = 0;
                    if (setsockopt(controller_udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == -1) {
                        disconnect_controller(controller);
                        close(controller_udp_socket);
                        exit(0);
                    }

                    //valid send subs_ack
                    char *sub_ack = create_pdu_subs_ack(server_config, *controller);
                    sendto(udp_socket, sub_ack, MAX_UDP_MESSAGE_SIZE, 0,
                        (struct sockaddr *)&client_addr, addr_len);

                    show_state_client(controller);

                    if (args.debug){
                        print_format(2,"Creació d'un nou fil (THREAD)");
                    }

                    //Save created socket 
                    controller->state=WAIT_INFO;
                    controller->socket_child = controller_udp_socket;
                    
                    //Creates new thread to handle with the client
                    ControllerHandlerArgs argsThread;
                    argsThread.debug = args.debug;
                    argsThread.pos = controller_pos;
                    pthread_t controller_thread;
                    if (pthread_create(&controller_thread, NULL, controller_handler, (void *)&argsThread) != 0) {
                        exit(1);
                    }                                        
                }
                else{
                    disconnect_controller(controller);
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
