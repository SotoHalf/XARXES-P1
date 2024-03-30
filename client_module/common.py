import os
import sys

#FINAL VALUES
BASE_PATH = os.getcwd()
CONFIG_PATH = os.path.join(BASE_PATH, "../config_files") #TEMPORAL REMOVE ./../
DEFAULT_CONFIG_CLIENT = "client.cfg"

#VALUES
debug = False
config_client = os.path.join(CONFIG_PATH, DEFAULT_CONFIG_CLIENT)

#FUNCTIONS
def arguments_c(**kwargs):
    global config_client
    path_config_client = kwargs.get("arg1", "")
    config_client = os.path.join(CONFIG_PATH, path_config_client)
    

def arguments_d():
    global debug
    debug = True

#STATIC ARGUMENTS
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