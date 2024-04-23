#!/usr/bin/env python

import sys
import time
import threading
import signal
import select
#-------------------------------
import common
from common import print_format
from class_manager import Client, SocketSetup, SocketType
from util_functions import (
    load_args, 
    load_config_file,
    subscription_start,
    keep_communication
)

socket_udp = None
socket_tcp = None

def signal_handler(sig, frame):
    global socket_tcp, socket_udp
    exit_server(socket_udp, socket_tcp)

# COMMANDS
def exit_server(socket_udp, socket_tcp):
    try:
        common.run_main = False
        common.hello_thread = False
        common.command_thread = False
        if socket_udp:
            socket_udp.close()
        if socket_tcp:
            socket_tcp.close()
        sys.exit(0)
    except:
        pass

def stat_client(client):
    print(client.get_stat())

#When the client is in an allowed state, commands are executed
def command_manager(client):
    global socket_tcp, socket_udp
    valid_states = ["SEND_HELLO"]
    while common.command_thread:
        try:
            user_input = None
            if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                user_input = input("")# Reed only if there is data to read
                # to be sure client stil in a valid state
                if client.get_current_state() in valid_states:
                    if user_input in common.COMMANDS:
                        if user_input == "quit":
                            exit_server(socket_udp, socket_tcp)
                        elif user_input == "stat":
                            stat_client(client)
        except KeyboardInterrupt:
            signal_handler(signal.SIGINT, None)
        time.sleep(1)


def main():
    global socket_udp, socket_tcp
    #Signal to kill correctly the client
    signal.signal(signal.SIGINT, signal_handler)
    #Get args by user
    load_args()
    #Load config client file
    client = load_config_file()
    #Create Socket
    socket_udp = SocketSetup(SocketType.UDP, client)

    #new thread for commands
    thread_commands = threading.Thread(target=command_manager, args=(client,))
    thread_commands.start()
    

    while common.run_main:
        time.sleep(1)

        if client.get_current_state() == "DISCONNECTED":
            client.set_current_state("NOT_SUBSCRIBED")
        #if go_subscription:
        if client.get_current_state() == "NOT_SUBSCRIBED":
            #close socket_tcp
            if socket_tcp:
                socket_tcp.close()
                socket_tcp = None

            #Subscription
            success, n_process = subscription_start(client, socket_udp)
            if not success:
                print_format(f"Superat el nombre de processos de subscripció ( {n_process} )")
                exit_server(socket_udp, socket_tcp)
                sys.exit(0)

            #Keep Communication with hello
            #new thread
            thread_hello = threading.Thread(target=keep_communication, args=(client, socket_udp))
            thread_hello.start()
        else:

            if socket_tcp is None and client.get_current_state()=="SEND_HELLO":
                #Open socket_tcp
                socket_tcp = SocketSetup(SocketType.TCP, client)
                if socket_tcp.get_connected():
                    print_format(f"Obert port TCP {socket_tcp.get_port()} per la comunicació amb el servidor")
            
                

if __name__ == "__main__":
    main()