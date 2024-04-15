import os
import sys
from datetime import datetime

 #--------- FINAL VALUES ---------
BASE_PATH = os.getcwd()
CONFIG_PATH = os.path.join(BASE_PATH, "./../config_files") #TEMPORAL REMOVE ./../
DEFAULT_CONFIG_CLIENT = "client.cfg"

#--------- VALUES ---------
debug = False
config_client = os.path.join(CONFIG_PATH, DEFAULT_CONFIG_CLIENT)

# --------- MAPPING ---------

#UDP DATAGRAM
#PACKAGE TYPES SUBSCRIPTION PHASE
PACKAGE_TYPE_SUB = {
    0x00: "SUBS_REQ",  #Petició de subscripció
    0x01: "SUBS_ACK",  #Acceptació de paquet de subscripció
    0x02: "SUBS_REJ",  #Rebuig de subscripció
    0x03: "SUBS_INFO", #Paquet addicional de subscripció
    0x04: "INFO_ACK",  #Acceptació del paquet addicional de subscripció
    0x05: "SUBS_NACK", #Error de subscripció
}

PACKAGE_TYPE_SUB_REVERSE = {
    "SUBS_REQ"  : 0x00,   #Petició de subscripció
    "SUBS_ACK"  : 0x01,   #Acceptació de paquet de subscripció
    "SUBS_REJ"  : 0x02,   #Rebuig de subscripció
    "SUBS_INFO" : 0x03,   #Paquet addicional de subscripció
    "INFO_ACK"  : 0x04,   #Acceptació del paquet addicional de subscripció
    "SUBS_NACK" : 0x05,   #Error de subscripció
}

CLIENT_STATES = {
    0xa0: "DISCONNECTED"   , # Controlador desconnectat
    0xa1: "NOT_SUBSCRIBED" , # Controlador connectat i no subscrit
    0xa2: "WAIT_ACK_SUBS"  , # Espera confirmació primer paquet subscripció
    0xa3: "WAIT_INFO"      , # Servidor esperant paquet SUBS_INFO
    0xa4: "WAIT_ACK_INFO"  , # Espera confirmació segon paquet subscripció
    0xa5: "SUBSCRIBED"     , # Controlador subscrit, sense enviar HELLO
    0xa6: "SEND_HELLO"     , # Controlador enviant paquets de HELLO
}

CLIENT_STATES_REVERSE = {
    "DISCONNECTED"   : 0xa0, # Controlador desconnectat
    "NOT_SUBSCRIBED" : 0xa1, # Controlador connectat i no subscrit
    "WAIT_ACK_SUBS"  : 0xa2, # Espera confirmació primer paquet subscripció
    "WAIT_INFO"      : 0xa3, # Servidor esperant paquet SUBS_INFO
    "WAIT_ACK_INFO"  : 0xa4, # Espera confirmació segon paquet subscripció
    "SUBSCRIBED"     : 0xa5, # Controlador subscrit, sense enviar HELLO
    "SEND_HELLO"     : 0xa6, # Controlador enviant paquets de HELLO
}

FLAG_PRINT = {
    0 : "MSG",
    1 : "ERROR",
    2 : "DEBUG"
}

# --------- FUNCTIONS ---------
def arguments_c(**kwargs):
    global config_client
    path_config_client = kwargs.get("arg1", "")
    config_client = os.path.join(CONFIG_PATH, path_config_client)
    
def arguments_d():
    global debug
    debug = True

def print_format(s, flag = 0):
    now = datetime.now().strftime("%H:%M:%S")
    flag_p = FLAG_PRINT.get(flag, FLAG_PRINT.get(0,"MSG"))
    print(f"{now}: {flag_p}. => {s}")

# --------- STATIC ARGUMENTS ---------
COMMAND_ARGUMENTS = {
    "-c" : {
        "num_args" : 1,
        "_func" : arguments_c
    },
    "-d" : {
        "num_args" : 0,
        "_func" : arguments_d
    }
}
