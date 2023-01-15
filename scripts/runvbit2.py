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

output = configData["settings"].get("output")
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

vbit = subprocess.Popen([os.path.join(os.getenv('HOME'), ".local/bin/vbit2"), "--dir", service["path"]], stdout=subprocess.PIPE)

signal.signal(signal.SIGTERM, signalHandler)
signal.signal(signal.SIGINT, signalHandler)

subprocess.Popen(destproc, stdin=vbit.stdout)
vbit.wait()

if postrun:
    subprocess.run(postrun)