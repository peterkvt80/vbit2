#!/usr/bin/env python3

import sys
import json
import os
import shutil
import subprocess
from dialog import Dialog

# get absolute path of vbit2 install
SCRIPTDIR = os.path.dirname(os.path.realpath(__file__))

# absolute path of installed services
SERVICESDIR = os.path.join(os.getenv('HOME'), ".teletext-services")

if not os.path.exists(SERVICESDIR):
    os.makedirs(SERVICESDIR)
    with open(os.path.join(SERVICESDIR,"IMPORTANT"), 'w') as importantFile:
        importantFile.write("IMPORTANT:\nThese directories were created by vbit-config.\nIf a service is uninstalled the directory will be deleted.")

d = Dialog(dialog="dialog", autowidgetsize=True)
d.set_background_title("VBIT2 Config")

with open(os.path.join(SCRIPTDIR,"known_services.json")) as servicesFile:
    data = json.load(servicesFile)
    servicesData = data["services"]

def selectService(configData):
    choices = []
    i = 0
    
    for service in configData["installed"]:
        choices += [(str(i), service["name"])]
        i += 1
    
    code, tag = d.menu("Choose service to generate",choices=choices,title="Select",cancel_label="Back")
    
    if code == "ok":
        configData["settings"]["selected"] = dict(choices)[tag]
        
        writeConfig(configData)
        
        if subprocess.run(["systemctl", "--user", "is-active", "vbit2.service"], capture_output=True, text=True).stdout == "active\n":
            # service is running so restart it
            subprocess.run(["systemctl", "--user", "restart", "vbit2.service"])

def installService(configData, servicesData, isGroup=False):
    installed = []
    for s in configData["installed"]:
        installed += [s["name"]]

    installable = []
    choices = []
    i = 0
    
    for service in sorted(servicesData,key=lambda x: x["name"]):
        installable += [service]
        choices += [(str(i), service["name"])]
        i += 1
    
    if not isGroup:
        choices += [("C", "Custom service")]
    
    code, tag = d.menu("Choose service to install",choices=choices,title="Install",cancel_label="Back")
    
    if code == "ok":
        if tag == "C":
            # custom service
            customMenu()
            
        else:
            service = installable[int(tag)]
            if service["type"] == "group":
                installService(configData, service["services"], True)
            else:
                name = service["name"]
                path = service["path"]
                
                subservices = service.get("subservices")
                if not subservices:
                    subservices = []

                options = []
                for subservice in subservices:
                    if not subservice.get("required"):
                        options += subservice
                
                # check if already installed
                if service["name"] in installed:
                    n = 2
                    while service["name"]+"-"+str(n) in installed:
                        n += 1
                    
                    name += "-"+str(n)
                    path += "-"+str(n)
                    
                    if options:
                        choices = [("A","Select ancillary service"),("N","Install again as "+name)]
                        code, tag = d.menu("Service is already installed.",choices=choices,title="Install")
                        if code == "ok":
                            if tag == "A":
                                chooseSubservices(subservices, os.path.join(SERVICESDIR,path))
                                return
                        else:
                            return
                    else:
                        text = "Service is already installed.\nInstall anyway as "+name+"?\n"
                        code = d.yesno(text)
                        if code != "ok":
                            return
                
                fullpath = os.path.join(SERVICESDIR,path)
                
                try:
                    doServiceInstall(service["type"],fullpath, service["url"])
                
                    # add service to config
                    configData["installed"] += [{"name":name, "type":service["type"], "path":fullpath}]
                    
                    configData["installed"].sort(key=lambda x: x["name"])
                    writeConfig(configData)
                    
                    chooseSubservices(subservices, fullpath)
                except:
                    d.msgbox("Installing service \""+service["name"]+"\" failed")
                    if os.path.commonpath([os.path.abspath(SERVICESDIR)]) == os.path.commonpath([os.path.abspath(SERVICESDIR), os.path.abspath(fullpath)]): # only delete service if installed under SERVICESDIR
                        shutil.rmtree(fullpath, ignore_errors=True) # delete directory

def chooseSubservices(subservices, fullpath):
    choices = []
    i = 0
    for subservice in subservices:
        if subservice.get("required"):
            doServiceInstall(subservice["type"], os.path.join(fullpath,subservice["path"]), subservice["url"])
        else:
            # optional ancillary services
            choices += [(str(i), subservice["name"], "off",)]
            i += 1
    
    if choices:
        code, tags = d.checklist("",choices=choices, title="Select optional sub-services to install", no_tags=True)
        
        if code == "ok" and tags:
            d.msgbox("Optional subservices not yet implemented")

def customMenu():
    choices = [("S", "Subversion repository"),("G", "Git repository"),("D", "Directory")]
    
    code, tag = d.menu("Select service type",choices=choices,title="Install custom service",cancel_label="Back")
    
    d.msgbox("Custom services not yet implemented")

def doServiceInstall(type,path,url):
    # todo actually install service
    os.mkdir(path)
    
    if type == "svn":
        process = subprocess.run(["svn", "checkout", "--quiet", url, path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        
        if process.returncode:
            text = "Subversion checkout failed with the following error:\n"
            text += process.stdout.decode("utf8")
            d.msgbox(text)
            raise RuntimeError("svn checkout failed")
    elif type == "git":
        process = subprocess.run(["git", "clone", "--quiet", "--depth", "1", url, path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        
        if process.returncode:
            text = "Git clone failed with the following error:\n"
            text += process.stdout.decode("utf8")
            d.msgbox(text)
            raise RuntimeError("git clone failed")
    else:
        print ("TODO: other types")

def uninstallService(configData):
    choices = []
    i = 0
    
    for service in configData["installed"]:
        choices += [(str(i), service["name"])]
        i += 1
    
    code, tag = d.menu("Choose service to uninstall",choices=choices,title="Uninstall",cancel_label="Back")
    
    if code == "ok":
        for service in configData["installed"]:
            name = service.get("name")
            
            if name == dict(choices)[tag]:
                text = "Are you sure you want to remove this service:\n "+name
                
                code = d.yesno(text)
                
                if code == "ok":
                    d.infobox("Uninstalling. Please wait...")
                    if dict(configData["settings"]).get("selected") == dict(choices)[tag]:
                        # stop vbit2 before deleting an active service
                        subprocess.run(["systemctl", "--user", "stop", "vbit2.service"])
                        configData["settings"].pop("selected")
                    
                    if os.path.commonpath([os.path.abspath(SERVICESDIR)]) == os.path.commonpath([os.path.abspath(SERVICESDIR), os.path.abspath(service["path"])]): # only delete service if installed under SERVICESDIR
                        shutil.rmtree(service["path"], ignore_errors=True) # delete directory
                        
                    configData["installed"].remove(service) # remove from list
                    writeConfig(configData)
                
                break

def importConfig():
    try:
        with open(os.path.join(SERVICESDIR,"config.json")) as configFile:
            configData = json.load(configFile)
    except:
        # opening file failed
        configData = {}
    
    if not configData.get("installed"):
        configData["installed"] = []
        
    if not configData.get("settings"):
        configData["settings"] = {}
    
    # no ui to set desired output yet so tell runvbit2 to use raspi-teletext
    if not configData["settings"].get("output"):
        configData["settings"]["output"] = "raspi-teletext"
    
    return configData

def writeConfig(configData):
    try:
        with open(os.path.join(SERVICESDIR,"config.json"), 'w') as configFile:
            json.dump(configData, configFile, indent=2)
    except:
        print("ERROR: updating config file failed")
        quit()

def optionsMenu():
    updateEnabled = subprocess.run(["systemctl", "--user", "is-enabled", "teletext-update.timer"], capture_output=True, text=True).stdout == "enabled\n"
    bootEnabled = subprocess.run(["systemctl", "--user", "is-enabled", "vbit2.service"], capture_output=True, text=True).stdout == "enabled\n"
    
    options = [("U", "Update services")]
    
    if updateEnabled:
        options[0] += ("on",)
    else:
        options[0] += ("off",)
    
    options += [("B", "Run VBIT2 at boot")]
    
    if bootEnabled:
        options[1] += ("on",)
    else:
        options[1] += ("off",)
    
    code, tags = d.checklist("",choices=options, title="Options", no_tags=True, no_cancel=True)

    if code == "ok":
        if not "U" in tags and updateEnabled:
            # update was enabled, now clear
            subprocess.run(["systemctl", "--user", "disable", "teletext-update.timer", "--now"], stderr=subprocess.DEVNULL)
        if "U" in tags and not updateEnabled:
            # update was clear, now enabled
            subprocess.run(["systemctl", "--user", "enable", "teletext-update.timer", "--now"], stderr=subprocess.DEVNULL)
        if not "B" in tags and bootEnabled:
            # run at boot was enabled, now clear
            subprocess.run(["systemctl", "--user", "disable", "vbit2.service"], stderr=subprocess.DEVNULL)
        if "B" in tags and not bootEnabled:
            # run at boot was clear, now enabled
            subprocess.run(["systemctl", "--user", "enable", "vbit2.service"], stderr=subprocess.DEVNULL)

def mainMenu():
    while True:
        configData = importConfig()
        
        options = [("I","Install service")]
        
        if configData["installed"]:
            options += [("S","Select service"), ("R","Remove service"), ("O","Options"), ("U","Update VBIT2")]
        
            if subprocess.run(["systemctl", "--user", "is-active", "vbit2.service"], capture_output=True, text=True).stdout == "active\n":
                options += [("V","Stop VBIT2")]
                command = "stop"
            else:
                options += [("V","Start VBIT2")]
                command = "start"
        
        code, tag = d.menu("",choices=options,title="Main menu",cancel_label="Exit")
        
        if code == "ok":
            if tag == "S":
                selectService(configData)
            elif tag == "I":
                installService(configData, servicesData)
            elif tag == "R":
                uninstallService(configData)
            elif tag == "O":
                optionsMenu()
            elif tag == "V":
                subprocess.run(["systemctl", "--user", command, "vbit2.service"])
            elif tag == "U":
                os.system('clear')
                subprocess.run(os.path.join(SCRIPTDIR,"update.sh")) # run the update script
                break # exit vbit-config
        
        else:
            break

if __name__ == "__main__":
    mainMenu()
    os.system('clear')