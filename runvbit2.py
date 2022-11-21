#!/usr/bin/env python3
# run vbit2 using configuration from config.json

import os
import json
import subprocess
import signal

def sigterm_handler(_signo, _stack_frame):
    # forward TERM signal to vbit2 process
    vbit.terminate()

# absolute path of installed services
SERVICESDIR = os.path.join(os.getenv('HOME'), ".teletext-services")

if not os.path.exists(SERVICESDIR):
    quit()

try:
    with open(os.path.join(SERVICESDIR,"config.json")) as configFile:
        configData = json.load(configFile)
except:
    # opening file failed
    print("Failed to open config.json in services directory")
    quit()

installed = configData.get("installed")

if not installed:
    installed = []

settings = configData.get("settings")

if not settings:
    settings = {}

selected = settings.get("selected")

if not selected:
    print("No service is selected")
    quit()
    
for service in installed:
    name = service.get("name")
    path = service.get("path")
    if name == selected:
        servicePath = path
        break

if not servicePath:
    print("error: selected service not found")
    quit()

linesPerField = 16 # vbit2 defaults to 16 lpf

# try to get lines_per_field setting from config files
for conffile in ["vbit.conf", "vbit.conf.override"]:
    confpath = os.path.join(servicePath, conffile)
    if os.path.exists(confpath):
        with open(confpath, "r") as conf:
            for line in conf:
                if line.startswith("lines_per_field="):
                    try:
                        linesPerField = int(line[16:])
                        if linesPerField < 1:
                            raise ValueError
                    except ValueError:
                        print("invalid lines_per_field in "+conffile)

# could check for overrides in the json config here too if vbit2 gains a command argument to override lines per field

output = settings.get("output")
if not output:
    print("no output type selected")
    quit()

if output == "raspi-teletext":
    prerun = ["sudo", os.path.join(os.getenv('HOME'), "raspi-teletext/tvctl"), "on"]
    postrun = ["sudo", os.path.join(os.getenv('HOME'), "raspi-teletext/tvctl"), "off"]
    
    if linesPerField > 16:
        print("full field operation not currently supported")
        quit()
    
    mask = 0xffff
    for i in range(linesPerField):
        mask = (mask << 1) & 0xffff
    
    destproc = [os.path.join(os.getenv('HOME'), "raspi-teletext/teletext"), "-m", "0x{:04x}".format(mask), "-"]

if prerun:
    subprocess.run(prerun)

vbit = subprocess.Popen([os.path.join(os.getenv('HOME'), ".local/bin/vbit2"), "--dir", servicePath], stdout=subprocess.PIPE)

signal.signal(signal.SIGTERM, sigterm_handler)

subprocess.Popen(destproc, stdin=vbit.stdout)
vbit.wait()

if postrun:
    subprocess.run(postrun)