#!/usr/bin/env python3

import sys
from pprint import pprint

#-------------------------------
import common
from util_functions import load_args, load_config_file
from class_manager import Client

 
    
def main():
    #Get args by user
    load_args()
    #Load config client file
    client = load_config_file()

    print(str(client))

    

if __name__ == "__main__":
    main()