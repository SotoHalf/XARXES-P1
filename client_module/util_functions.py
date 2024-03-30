import sys
import os
#-------------------------------
import common
from class_manager import Client, Element


#Load config client file
def load_config_file():
    #Check if file exists
    if not os.path.exists(common.config_client):
        print(f"No es pot obrir l'arxiu de configuració: {os.path.basename(common.config_client)}")
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