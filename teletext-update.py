#!/usr/bin/env python3
# Update the active service and any ancillary sub-services

import config
import os
import subprocess

configData = config.load()

try:
    service = config.getSelectedService(configData)
except Exception as e:
    print(e)
    quit()

if service["type"] == "svn":
    process = subprocess.run(["svn", "up", service["path"]], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    
    if process.returncode:
        print("svn update failed with: "+process.stdout.decode("utf8"))
    
elif service["type"] == "git":
    process = subprocess.run(["git", "-C", service["path"], "pull"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    
    if process.returncode:
        print("git pull failed with: "+process.stdout.decode("utf8"))
