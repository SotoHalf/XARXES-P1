import sys
import os
import random
import time
#-------------------------------
import common
from common import print_format
from class_manager import Client, Element, SocketSetup

#subscription
def subscription_send(client:Client, socket:SocketSetup):
    socket.sendto("test")
    #socket.close()

def subscription_process(client:Client, socket:SocketSetup, t:int ,p:int , q:int, n:int):
    client.set_current_state("WAIT_ACK_SUBS")
    print_format(f"Controlador passa a l'estat: {client.get_current_state()}")

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
        subscription_send(client,socket)
        #crec que els time.sleep no funcionara perque enviara 
        #el ack i es perdra, provar i modificar si fos el cas #borrar
        time.sleep(time_wait)
        count += 1

#despres fer els estats
def subscription_start(client:Client, socket:SocketSetup):

    MIN_PACKAGES = 3
    MAX_PACKAGES = 5
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
        subscription_process(client,socket,t,p,q,n)
        o-=1
        
        client.set_current_state("NOT_SUBSCRIBED")
        print_format(f"Controlador passa a l'estat: {client.get_current_state()}")
        #Aqui igual que adalt #borrar
        time.sleep(u) #wait u sec to try again


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