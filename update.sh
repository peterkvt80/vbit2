#!/bin/sh

echo Updating to latest stable version of vbit2
git fetch --tags
latestTag=`curl --silent "https://api.github.com/repos/peterkvt80/vbit2/releases/latest" | grep -Po '"tag_name": "\K.*?(?=")'`
git checkout $latestTag # switch to latest 

# new version might need to perform tasks we don't know about, so run its postupdate script
./postupdate.sh
