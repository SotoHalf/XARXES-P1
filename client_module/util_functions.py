import sys
import os
import random
import time
#-------------------------------
import common
from common import print_format
from class_manager import Client, Element, SocketSetup, PDU

#Change state to client and print if it's a new state
def change_state_client(client, state, reason = ""):
    if client.get_current_state() != state:
        client.set_current_state(state)
        print_format(f"Controlador passa a l'estat: {client.get_current_state()} {reason}")

#Creates a pdu objects and send it
def send_pdu_datagram(client:Client, socket:SocketSetup, typePdu:str, time_wait:int):
    if common.debug:
        print_format(f"send pdu type: {typePdu}",flag=2)
    datagram = PDU(**{
        "typePdu" : typePdu,
        "client" : client
    })
    socket.sendto(datagram, timeout = time_wait)
    data_recived = socket.recvData() #get response from the server
    return data_recived, datagram

#Create a pdu objects using the response from the server
def read_pdu_datagram(data_recived:bytes):
    if data_recived:
        pdu_recv = PDU.pdu_from_datagram(data_recived)
        pdu_kwargs = pdu_recv.get_attrs()
        return pdu_kwargs
    return {}

#Hello thread method
def keep_communication(client, socket):
    v = 2
    r = 2
    s = 3

    common.hello_thread = True
    first = True
    while common.hello_thread:

        if  (
            client.get_current_state() == "SEND_HELLO" or 
            (first and client.get_current_state() == "SUBSCRIBED")
        ): #Valid states to keep the hello communication

            data_recived_hello, hello_sended = send_pdu_datagram(client, socket, "HELLO", r) #send hello
            pdu_kwargs = read_pdu_datagram(data_recived_hello) #read hello response

            if not pdu_kwargs:
                s-=1
                #Disconnect cause more than 3 datagrams were lost
                if s <= 0:
                    change_state_client(client,"DISCONNECTED", reason = "(Sense resposta a 3 ALIVES)")

            #Disconnect cause hello rejected
            elif pdu_kwargs.get('pdu_type','') == "HELLO_REJ":
                change_state_client(client,"DISCONNECTED", reason = "(HELLO_REJ rebut)")
            else:
                if PDU.check_hello_recived(client, pdu_kwargs, hello_sended.get_attrs()):
                    s = 3 #reset lost datagrams
                    if first:
                        change_state_client(client,"SEND_HELLO")
                        first = False
                else:
                    #Disconnect cause data do not match
                    send_pdu_datagram(client, socket, "HELLO_REJ", 1)
                    change_state_client(client,"DISCONNECTED", reason = "(Suplantació d'identitat detectada)")

        else:
            common.hello_thread = False
            break
        time.sleep(v)

#Subscription
def subscription_process(client:Client, socket:SocketSetup, t:int ,p:int , q:int, n:int):
    
    count = 0
    time_wait = t
    while count < n:
        if common.debug:
            print('----------------------')
            print_format(f"t={t} p={p} q={q} n={n} count={count} time_wait={time_wait}", flag=2)
            print('----------------------')
            
        #we increase 't' until we send 'p' packets up to reach 'q'
        if count >= p and time_wait < q+t:
            time_wait += 1
        
        change_state_client(client, "WAIT_ACK_SUBS")
        data_recived_req, _ = send_pdu_datagram(client,socket,"SUBS_REQ", time_wait)
        pdu_kwargs = read_pdu_datagram(data_recived_req)

        # The client must act depending on the server's response
        if pdu_kwargs.get('pdu_type','') == "SUBS_ACK":
            #set new data to the client
            new_udp_port = int(pdu_kwargs.get('port_udp',client.get_srvUDP()))
            client.set_server_data(
                pdu_kwargs.get('random_num',client.get_random_num()),
                pdu_kwargs.get('mac',client.get_random_num()),
                new_udp_port,
            )
            #new port for udp
            socket.set_port(new_udp_port)

            #send subs_info
            change_state_client(client, "WAIT_ACK_INFO")
            data_recived, _ = send_pdu_datagram(client,socket,"SUBS_INFO", t) #t = 1 sec
            pdu_kwargs = read_pdu_datagram(data_recived)
            
            correct = False
            if pdu_kwargs.get('pdu_type','') == "INFO_ACK":
                #check data from info and set the new port to tcp
                correct = client.check_info_ack_data(
                    pdu_kwargs.get('random_num',client.get_random_num()),
                    pdu_kwargs.get('mac',client.get_random_num()),
                    int(pdu_kwargs.get('port_tcp',client.get_localTCP())),
                )
            
            socket.set_port(client.get_srvUDP())
            if correct:
                change_state_client(client, "SUBSCRIBED")
                return False

        count+=1
        pdu_type = pdu_kwargs.get('pdu_type','')
        # Server denies and errors
        if pdu_type in ["SUBS_NACK", "SUBS_REJ"]:
            change_state_client(client, "NOT_SUBSCRIBED")
            reason = pdu_kwargs.get('reason','')
            print_format(f"Controlador ha rebut un paquet tipus {pdu_type} \nMotiu del servidor: {reason}")
            if pdu_type == "SUBS_REJ":
                print_format("El procediment tornarà a començar")
                return True
        #in case to recive a different pdu from expected
        elif (
                (pdu_type == "SUBS_ACK" and client.get_current_state == "WAIT_ACK_INFO") or
                (pdu_type == "INFO_ACK" and client.get_current_state == "WAIT_ACK_SUBS") or
                (pdu_type not in ["SUBS_ACK", "INFO_ACK",""])
            ):
            change_state_client(client, "NOT_SUBSCRIBED")
            print_format(u"Controlador ha rebut un paquet diferent a l'esperat el procediment tornarà a començar")
            return True
    client.set_current_state("NOT_SUBSCRIBED")
    return False
        

#Subscription
def subscription_start(client:Client, socket:SocketSetup):

    MIN_PACKAGES = 1
    MAX_PACKAGES = 3
    MIN_RESTART = 1
    MAX_RESTART = 3
    MAX_SUBCRIPTIONS = 3

    t = 1 #time
    p = random.randint(MIN_PACKAGES,MAX_PACKAGES+1) #packages
    q = random.randint(MIN_PACKAGES,MAX_PACKAGES+1) # q to increment t
    n = p + q + random.randint(MIN_PACKAGES,MAX_PACKAGES+1) #in case to fail p + q 
    u = random.randint(MIN_RESTART,MAX_RESTART+1) #time wait to restart process
    o = MAX_SUBCRIPTIONS #max subcription_process 

    if common.debug:
        print('----------------------')
        print_format(f"Initial Subscription Values", flag=2)
        print_format(f"t={t} p={p} q={q} n={n} u={u} o={o}", flag=2)
        print('----------------------')

    while client.get_current_state() == "NOT_SUBSCRIBED" and o > 0:
        print_format(f"Controlador en l'estat: {client.get_current_state()}, procés de subscripció: {MAX_SUBCRIPTIONS-o+1}")
        restart = subscription_process(client,socket,t,p,q,n)
        if restart:
            o = MAX_SUBCRIPTIONS 
        else:
            o-=1
        
        if client.get_current_state() != "SUBSCRIBED":
            time.sleep(u) #wait u sec to try again

    return client.get_current_state() == "SUBSCRIBED", MAX_SUBCRIPTIONS
#Load config client file
def load_config_file():
    #Check if file exists
    if not os.path.exists(common.config_client):
        print_format(f"No es pot obrir l'arxiu de configuració: {os.path.basename(common.config_client)}", flag=1)
        sys.exit(1)
    
    client = None

    #Read file
    with open(common.config_client,'r') as file:
        lines = file.readlines()
        kwargs = {}
        #Each line of the file represents a parameter of the class
        for line in lines:
            arg, value = line.split("=") 
            arg = arg.strip()
            value = value.strip()
        
            #Elements is the only one that needs a separate class
            if arg == "Elements": 
                elements = []
                for element in value.split(";"):
                    data = element.split("-")
                    if len(data) == 3:
                        elements.append(Element(**{
                            "magnitud" : data[0],
                            "ordinal" : data[1],
                            "typeIo" : data[2]
                        }))
                kwargs[arg] = elements
            else:
                kwargs[arg] = value
        client = Client(**kwargs)
    return client

            

#Parse the args from user
def load_args():
    #remove the file name
    _args = sys.argv[1:] 
    for arg in _args:
        #if the argument it's valid
        if arg in common.COMMAND_ARGUMENTS: 
            j = 1
            # Get the function that must be run
            # Get the number of args that the function needs
            _func = common.COMMAND_ARGUMENTS[arg]["_func"] 
            num_args = common.COMMAND_ARGUMENTS[arg]["num_args"]
            if num_args > 0:
                #Build a generic dict
                kwargs = {}
                while num_args > 0:
                    kwargs[f"arg{j}"] = _args[j] if j < len(_args) else " "
                    num_args -= 1
                    j+=1
                #Run the function
                _func(**kwargs)
            else:
                _func()
            #Remove the actual taken args
            _args = _args[j:]