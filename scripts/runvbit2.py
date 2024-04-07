#!/usr/bin/env python3
# run vbit2 using configuration from config.json

import config
import os
import subprocess
import signal

def signalHandler(_signo, _stack_frame):
    # send TERM signal to vbit2 process
    vbit.terminate()

configData = config.load()

service = config.getSelectedService()

if not service:
    print("No service selected")
    quit()

linesPerField = 16 # vbit2 defaults to 16 lpf

# try to get lines_per_field setting from config files
for conffile in ["vbit.conf", "vbit.conf.override"]:
    confpath = os.path.join(service["path"], conffile)
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

cmdline = [os.path.join(os.getenv('HOME'), ".local/bin/vbit2"), "--dir", service["path"]]
prerun = []
postrun = []

output = configData["settings"].get("output")
if output == "none":
    # disables piped output!
    cmdline.append("--format")
    cmdline.append("none")
elif output == "raspi-teletext":
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

packetServerPort = configData["settings"].get("packetServerPort")
if configData["settings"].get("packetServer") and packetServerPort:
    cmdline.append("--packetserver")
    cmdline.append(str(packetServerPort))

vbit = subprocess.Popen(cmdline, stdout=subprocess.PIPE)

signal.signal(signal.SIGTERM, signalHandler)
signal.signal(signal.SIGINT, signalHandler)

if not output == "none":
    subprocess.Popen(destproc, stdin=vbit.stdout)

vbit.wait()

if postrun:
    subprocess.run(postrun)
