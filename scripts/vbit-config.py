#!/usr/bin/env python3

import config
import json
import sys
import os
import shutil
import subprocess
import re
from dialog import Dialog
from dialog import DialogBackendVersion

# get absolute path of vbit2 scripts directory
SCRIPTDIR = os.path.dirname(os.path.realpath(__file__))

# absolute path of known services file
KNOWNSERVICES = os.path.join(SCRIPTDIR,"known_services.json")

# absolute path of installed services
SERVICESDIR = os.path.join(os.getenv('HOME'), ".teletext-services")

d = Dialog(dialog="dialog", autowidgetsize=True)
d.set_background_title("VBIT2 Config")

def selectServiceMenu():
    choices = []
    i = 0
    
    for service in config.getInstalledServices():
        choices += [(str(i), service["name"])]
        i += 1
    
    code, tag = d.menu("Choose service to generate",choices=choices,title="Select",cancel_label="Back")
    
    if code == "ok":
        config.selectService(dict(choices)[tag])

def installServiceMenu(servicesData, isGroup=False):
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
            customServiceMenu()
        
        else:
            service = installable[int(tag)]
            if service["type"] == "group":
                # iterate into subgroup with new menu
                installServiceMenu(service["services"], True)
            else:
                name = service.get("name")
                path = service.get("path")
                
                # check if already installed
                installedNames = []
                for s in config.getInstalledServices():
                    installedNames += [s["name"]]
                if service["name"] in installedNames:
                    n = 2
                    while service["name"]+"-"+str(n) in installedNames:
                        n += 1
                    
                    name += "-"+str(n)
                    path += "-"+str(n)
                    
                    text = "Service is already installed.\nInstall anyway as "+name+"?\n"
                    code = d.yesno(text)
                    if code != "ok":
                        return
                
                serviceConfigObject = {"name":name, "type":service.get("type"), "path":os.path.join(SERVICESDIR,path), "url":service.get("url")}
                
                if service.get("subservices"):
                    selectedSubservices = chooseSubservicesMenu(service["subservices"])
                    if selectedSubservices:
                        serviceConfigObject["subservices"] = selectedSubservices
                
                try:
                    # selected a name so add service to config
                    d.infobox("Installing. Please wait...")
                    config.installService(serviceConfigObject)
                except Exception as e:
                    d.msgbox("Installation failed: {0}".format(e), cancel_label="Back")

def chooseSubservicesMenu(subservices):
    selectedSubservices = []
    optionalSubservices = []
    choices = []
    i = 0
    for subservice in subservices:
        if subservice.get("required"):
            selectedSubservices += [subservice]
        else:
            # optional ancillary services
            optionalSubservices += [subservice]
            choices += [(str(i), subservice["name"], "off",)]
            i += 1
    
    if choices:
        choices = [("none", "None", "on",)] + choices
        code, tag = d.radiolist("",choices=choices, title="Select optional sub-service to install", no_tags=True, no_cancel=True)
        
        if code == "ok" and tag != "none":
            selectedSubservices += [optionalSubservices[int(tag)]]
    
    return selectedSubservices

def customServiceMenu():
    choices = [("S", "Subversion repository"),("G", "Git repository"),("D", "Directory")]
    
    code, tag = d.menu("Select service type",choices=choices,title="Install custom service",cancel_label="Back")
    
    if code == "ok":
        if tag == "D":
            # local directory
            if DialogBackendVersion.fromstring(d.backend_version()) < DialogBackendVersion.fromstring("1.3-20201126"):
                h = 10
            else:
                h = 20

            while True: # select directory loop
                code, string = d.dselect(os.getenv('HOME')+"/", title="Enter teletext service directory:", height=h)
                if code == "ok":
                    path = os.path.normpath(string)
                    code = d.yesno("Selected directory "+string, yes_label="OK", no_label="Back")
                    if code == "ok":
                        # confirmed directory
                        while True: # name input loop
                            code, string = d.inputbox("Enter service name:", cancel_label="Back")
                            if code == "ok":
                                try:
                                    # selected a name so add service to config
                                    config.installService({"name":string, "type":"dir", "path":path})
                                except Exception as e:
                                    d.msgbox("Installation failed: {0}".format(e), cancel_label="Back")
                                break
                        break
                else:
                    break # aborted so break out of select directory loop
        
        else:
            # remote service
            if tag == "S":
                # subversion
                type = "svn"
                
            if tag == "G":
                # git
                type = "git"
            
            code, string = d.inputbox("Enter repository URL:")
            
            if code == "ok":
                url = string
                
                while True: # name input loop
                    code, string = d.inputbox("Enter service name:")
                    if code == "ok":
                        # strip whitespace and illegal characters
                        name = re.sub('(^\s*)|([./\\\"\'])|(\s*$)', '', string)
                        
                        installedNames = []
                        for s in config.getInstalledServices():
                            installedNames += [s["name"]]
                            
                        if name in installedNames:
                            d.msgbox("Service name already in use", cancel_label="Back")
                            
                        else:
                            if not os.path.exists(os.path.join(SERVICESDIR, "custom_services")):
                                os.makedirs(os.path.join(SERVICESDIR, "custom_services"))
                            
                            fullpath = os.path.join(SERVICESDIR, "custom_services", name) # create a path from the service name
                            
                            try:
                                # selected a name so add service to config
                                d.infobox("Installing. Please wait...")
                                config.installService({"name":name, "type":type, "path":fullpath, "url":url})
                            except Exception as e:
                                d.msgbox("Installation failed: {0}".format(e), cancel_label="Back")
                            break
                    else:
                        break

def uninstallService():
    choices = []
    i = 0
    
    for service in config.getInstalledServices():
        choices += [(str(i), service["name"])]
        i += 1
    
    code, tag = d.menu("Choose service to uninstall",choices=choices,title="Uninstall",cancel_label="Back")
    
    if code == "ok":
        name = dict(choices)[tag]
        text = "Are you sure you want to remove this service:\n "+name
        code = d.yesno(text)
        
        if code == "ok":
            d.infobox("Uninstalling. Please wait...")
            
            config.uninstallService(name)
def optionsMenu():
    configData = config.load()
    updateEnabled = subprocess.run(["systemctl", "--user", "is-enabled", "teletext-update.timer"], capture_output=True, text=True).stdout == "enabled\n"
    bootEnabled = subprocess.run(["systemctl", "--user", "is-enabled", "vbit2.service"], capture_output=True, text=True).stdout == "enabled\n"
    serverEnabled = configData["settings"].get("packetServer")
    interfaceEnabled = configData["settings"].get("interfaceServer")
    
    options = [("U", "Automatically update selected service")]
    
    if updateEnabled:
        options[0] += ("on",)
    else:
        options[0] += ("off",)
    
    options += [("B", "Run VBIT2 automatically at boot")]
    
    if bootEnabled:
        options[1] += ("on",)
    else:
        options[1] += ("off",)
    
    options += [("S", "Enable teletext packet server")]
    
    if serverEnabled:
        options[2] += ("on",)
    else:
        options[2] += ("off",)
    
    options += [("I", "Enable control interface server")]
    
    if interfaceEnabled:
        options[3] += ("on",)
    else:
        options[3] += ("off",)
    
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
        if not "S" in tags and serverEnabled:
            # server was enabled, now clear
            configData["settings"]["packetServer"] = False
            config.save(configData)
        if "S" in tags and not serverEnabled:
            # server was clear, now enabled
            configData["settings"]["packetServer"] = True
            config.save(configData)
        if not "I" in tags and interfaceEnabled:
            # server was enabled, now clear
            configData["settings"]["interfaceServer"] = False
            config.save(configData)
        if "I" in tags and not interfaceEnabled:
            # server was clear, now enabled
            configData["settings"]["interfaceServer"] = True
            config.save(configData)
        

def mainMenu():
    while True:
        options = [("I","Install service")]
        
        if config.getInstalledServices():
            options += [("S","Select service"), ("R","Remove service"), ("O","Options"), ("U","Update VBIT2")]
        
            if subprocess.run(["systemctl", "--user", "is-active", "vbit2.service"], capture_output=True, text=True).stdout == "active\n":
                options += [("V","Stop VBIT2")]
                command = "stop"
            elif config.getSelectedService():
                options += [("V","Start VBIT2")]
                command = "start"
        
        code, tag = d.menu("",choices=options,title="Main menu",cancel_label="Exit")
        
        if code == "ok":
            if tag == "S":
                selectServiceMenu()
            elif tag == "I":
                # reload known services
                with open(KNOWNSERVICES) as servicesFile:
                    data = json.load(servicesFile)
                    servicesData = data["services"]
                
                for service in servicesData:
                    if not service.get("name") or not service.get("type"):
                        d.msgbox("Fatal error: invalid service configuration data in {0}\n{1}".format(KNOWNSERVICES, service), cancel_label="Back")
                installServiceMenu(servicesData)
            elif tag == "R":
                uninstallService()
            elif tag == "O":
                optionsMenu()
            elif tag == "V":
                subprocess.run(["systemctl", "--user", command, "vbit2.service"])
            elif tag == "U":
                os.system('clear')
                subprocess.run(os.path.join(SCRIPTDIR,"../update.sh")) # run the update script
                break # exit vbit-config
        
        else:
            break

if __name__ == "__main__":
    mainMenu()
    os.system('clear')
