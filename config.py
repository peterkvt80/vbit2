import os
import json

# absolute path of installed services
SERVICESDIR = os.path.join(os.getenv('HOME'), ".teletext-services")

if not os.path.exists(SERVICESDIR):
    os.makedirs(SERVICESDIR)
    with open(os.path.join(SERVICESDIR,"IMPORTANT"), 'w') as importantFile:
        importantFile.write("IMPORTANT:\nThese directories were created by vbit-config.\nIf a service is uninstalled the directory will be deleted.")

if not os.path.exists(os.path.join(SERVICESDIR, "custom_services")):
    os.makedirs(os.path.join(SERVICESDIR, "custom_services"))

def load():
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
    
    # Default to raspi-teletext output
    if not configData["settings"].get("output"):
        configData["settings"]["output"] = "raspi-teletext"
    
    return configData

def save(configData):
    try:
        with open(os.path.join(SERVICESDIR,"config.json"), 'w') as configFile:
            json.dump(configData, configFile, indent=2)
    except:
        raise RuntimeError("updating config file failed")

def getSelectedService(configData):
    selected = configData["settings"].get("selected")
    if not selected:
        raise RuntimeError("No service is selected")
    
    for service in configData["installed"]:
        name = service.get("name")
        path = service.get("path")
        if name == selected:
            return service
        
    raise RuntimeError("selected service not found")
