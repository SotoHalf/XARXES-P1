#!/usr/bin/env python3


###########
##!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
##!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#recorda posar python y no python3

import sys
import time
import threading
import signal
from pprint import pprint

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


def exit_server(socket_udp, socket_tcp):
    common.run_main = False
    common.hello_thread = False
    if socket_udp:
        socket_udp.close()
    sys.exit(0)

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
    

    while common.run_main:
        time.sleep(1)
        #if go_subscription:
        if client.get_current_state() == "NOT_SUBSCRIBED":
            
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
            #per reinciar la suscripcio pensa en modificar l'estat del client y el go_subscription
        else:
            pass
            #if client.get_current_state != ""
            #Commands
            """
            user_input = input("")
            if user_input in common.COMMANDS:
                if user_input == "quit":
                    exit_server(socket_udp, socket_tcp)
            """
            
            

if __name__ == "__main__":
    main()