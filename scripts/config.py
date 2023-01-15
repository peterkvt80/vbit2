import os
import json
import subprocess
import shutil

# absolute path of installed services
SERVICESDIR = os.path.join(os.getenv('HOME'), ".teletext-services")

def load():
    if not os.path.exists(SERVICESDIR):
        os.makedirs(SERVICESDIR)
        with open(os.path.join(SERVICESDIR,"IMPORTANT"), 'w') as importantFile:
            importantFile.write("IMPORTANT:\nThese directories were created by vbit-config.\nIf a service is uninstalled the directory will be deleted.")
    
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

def getInstalledServices():
    configData = load()
    return configData["installed"]

def getSelectedService():
    configData = load()
    selected = configData["settings"].get("selected")
    if not selected:
        return {}
    
    for service in configData["installed"]:
        name = service.get("name")
        path = service.get("path")
        if name == selected:
            return service
        
    return {}

def selectService(name):
    configData = load()
    configData["settings"]["selected"] = name
    save(configData)
    
    if subprocess.run(["systemctl", "--user", "is-active", "vbit2.service"], capture_output=True, text=True).stdout == "active\n":
        # service is running so restart it
        subprocess.run(["systemctl", "--user", "restart", "vbit2.service"])

def uninstallService(name):
    configData = load()
    
    for service in configData["installed"]:
        if service.get("name") == name:
            type = service.get("type")
            
            if dict(configData["settings"]).get("selected") == name:
                # stop vbit2 before deleting an active service
                subprocess.run(["systemctl", "--user", "stop", "vbit2.service"])
                # deselect the service
                configData["settings"].pop("selected")
            if os.path.commonpath([os.path.abspath(SERVICESDIR)]) == os.path.commonpath([os.path.abspath(SERVICESDIR), os.path.abspath(service["path"])]) and type != "dir":
                # only delete service if installed under SERVICESDIR and not custom dir type
                shutil.rmtree(service["path"], ignore_errors=True) # delete directory
            
            configData["installed"].remove(service) # remove from installed list
            save(configData)
            break

def installService(service):
    configData = load()
    
    if not service.get("name") or not service.get("type") or not service.get("path"):
        raise RuntimeError("invalid service configuration data\n{0}".format(service))
    
    installedNames = []
    for s in configData.get("installed"):
        installedNames += [s.get("name")]
    
    if service["name"] in installedNames:
        raise RuntimeError("Service name already in use")
    
    if service["type"] == "dir":
        # service in an arbitrary directory
        if not os.path.exists(service["path"]):
            raise RuntimeError("Directory does not exist")
    else:
        if not service.get("url"):
            raise RuntimeError("invalid service configuration data\n{0}".format(service))
        doServiceInstall(service["type"], service["path"], service["url"])
    
    serviceConfigObject = {"name":service["name"], "type":service["type"], "path":service["path"]}
    
    subservices = service.get("subservices")
    
    try:
        if subservices:
            serviceConfigObject["subservices"] = []
            for subservice in subservices:
                if not subservice.get("type") or not subservice.get("path") or not subservice.get("url"):
                    raise RuntimeError("invalid subservice configuration data\n{0}".format(subservice))
                subservicePath = os.path.join(service["path"],subservice.get("path"))
                doServiceInstall(subservice.get("type"), subservicePath, subservice.get("url"))
                serviceConfigObject["subservices"] += [{"name":subservice["name"], "type":subservice["type"], "path":subservicePath, "required":subservice.get("required") or False}]
        
        configData["installed"] += [serviceConfigObject]
        configData["installed"].sort(key=lambda x: x["name"])
        # if no service is selected, select it
        if not configData["settings"].get("selected"):
            configData["settings"]["selected"] = service["name"]
        
        save(configData)
    except Exception as e:
        shutil.rmtree(service["path"], ignore_errors=True) # delete directory
        raise Exception(e)

def doServiceInstall(type, path, url):
    if type == "git" or type == "svn":
        if os.path.commonpath([os.path.abspath(SERVICESDIR)]) != os.path.commonpath([os.path.abspath(SERVICESDIR), os.path.abspath(path)]):
            raise RuntimeError("Tried to install outside SERVICESDIR")
            
        os.mkdir(path)
        
        if type == "svn":
            process = subprocess.run(["svn", "checkout", "--quiet", url, path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        
            if process.returncode:
                text = "Subversion checkout failed with the following error:\n"
                text += process.stdout.decode("utf8")
                raise RuntimeError(text)
                
        elif type == "git":
            process = subprocess.run(["git", "clone", "--quiet", "--depth", "1", url, path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            
            if process.returncode:
                text = "Git clone failed with the following error:\n"
                text += process.stdout.decode("utf8")
                raise RuntimeError(text)
    else:
        raise RuntimeError("unknown service type")