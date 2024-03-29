#!/usr/bin/env python3
# Update the active service and any ancillary sub-services

import config
import os
import subprocess

try:
    service = config.getSelectedService()
except Exception as e:
    print(e)
    quit()

def updateService(service):
    # update the service
    if service["type"] == "svn":
        process = subprocess.run(["svn", "up", service["path"]], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        
        if process.returncode:
            print("svn update failed with: "+process.stdout.decode("utf8"))
        
        process = subprocess.run(["svn", "cleanup", service["path"]], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if process.returncode:
            print("svn cleanup failed with: "+process.stdout.decode("utf8"))
        
        
    elif service["type"] == "git":
        process = subprocess.run(["git", "-C", service["path"], "pull"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        
        if process.returncode:
            print("git pull failed with: "+process.stdout.decode("utf8"))
    
    subservices = service.get("subservices")
    if subservices:
        for subservice in subservices:
            updateService(subservice) # recurse into subservices

updateService(service)

