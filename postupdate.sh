#!/bin/bash

# Perform any script actions which need to happen after switching to the latest
# tagged release

main(){
  # ensure we are in the vbit2 source directory
  cd `dirname "$(readlink -f "$0")"`
  
  # if it's an old install without auto deps we should do a complete recompile
  if [ ! -f vbit2.d ]; then
    # hope that presence of vbit2.d means all dep files are present
    make clean
  fi

  # offer to upgrade old installs to new scripts (runs getvbit2)
  migrate
  
  # recompile vbit2
  make
  
  sudo apt -qq -y install python3-dialog
  
  # remove old symlink
  rm $HOME/.local/bin/runvbit2.sh 2>/dev/null
  
  # create links
  mkdir -p $HOME/.local/bin
  ln -s -f `pwd`/vbit2 $HOME/.local/bin/
  ln -s -f `pwd`/scripts/runvbit2.py $HOME/.local/bin/runvbit2
  ln -s -f `pwd`/scripts/teletext-update.py $HOME/.local/bin/teletext-update
  ln -s -f `pwd`/scripts/vbit-config.py $HOME/.local/bin/vbit-config
  
  # install systemd user scripts
  mkdir -p $HOME/.local/share/systemd/user
  cp vbit2.service $HOME/.local/share/systemd/user
  cp teletext-update.timer $HOME/.local/share/systemd/user
  cp teletext-update.service $HOME/.local/share/systemd/user
  
  systemctl --user daemon-reload
  
  cleanoldunits
  
  # warn about removing old services
  migratejson
  
  # restart vbit if service is active
  if [[ `systemctl --user is-active vbit2.service` == "active" ]]; then
    systemctl --user restart vbit2.service
  fi
}

cleanoldunits(){
  # clean up older systemd files
  if [ -f $HOME/.config/systemd/user/vbit2.service ]; then
    systemctl --user disable vbit2.service --now # removes old link
  fi

  if [ -f $HOME/.config/systemd/user/teletext-update.service ]; then
    systemctl --user disable teletext-update.service --now # removes old link
  fi

  if [ -f $HOME/.config/systemd/user/teletext-update.timer ]; then
    systemctl --user disable teletext-update.timer --now # removes old link
  fi
}

migrate(){
  FOUND=()
  if [ -f $HOME/vb2 ]; then FOUND+=("$HOME/vb2"); fi
  if [ -f $HOME/vbit2.sh ]; then FOUND+=("$HOME/vbit2.sh"); fi
  if [ -f $HOME/updatePages.sh ]; then FOUND+=("$HOME/updatePages.sh"); fi
  if [ -d $HOME/raspi-teletext-master ]; then FOUND+=("$HOME/raspi-teletext-master"); fi
  if [ -f /etc/systemd/system/vbit2.service ]; then FOUND+=("/etc/systemd/system/vbit2.service"); fi
  if [ ! ${#FOUND[@]} -eq 0 ]; then
    printf 'The following files were found which relate to an old version of vbit2:' | fold -s -w `tput cols`
    printf '\n%s' "${FOUND[@]}" | fold -s -w `tput cols`
    printf '\n\nIt is recommended to upgrade to the new system which includes the interactive vbit-config utility.\nDo you wish to attempt to reinstall vbit2 automatically?\n\033[1mCaution: this will remove the files and directories listed above losing any local changes you have made.\033[0m\n' | fold -s -w `tput cols`
    read -p "(y)es (n)o" -n 1 -s
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
      printf "leaving start scripts unchanged.\n"
    else
      # Here Be Dragons!
      # remove any updatePages.sh cron job
      sudo crontab -l | grep -v 'updatePages.sh' | sudo crontab -
      crontab -l | grep -v 'updatePages.sh' | crontab -
      # delete the old files and directories
      sudo systemctl disable vbit2 --now
      sudo rm -rf ${FOUND[@]}
      # run the new installer
      ./getvbit2
      
      if [ -d $HOME/Pages ]; then
        printf 'The directory %s is no longer required.\n' "$HOME/Pages"
      fi
      
      if [ -d $HOME/teletext ]; then
        printf 'The directory %s is no longer required.\n' "$HOME/teletext"
      fi
    fi
    exit
  fi
}

migratejson(){
  if [ -f $HOME/.teletext-services/config ]; then
    printf 'This version of VBIT2 uses a new config format and directory scheme.\nServices will not be automatically migrated so must be reinstalled.\nThe following services were found:\n'
    ls -d1 ~/.teletext-services/*/
    
    read -n 1 -s -r -p "Do you wish to create a backup of these services? (y/N)"
    if [[ $REPLY =~ ^[Yy]$ ]]; then
      printf '\nbacking up old services to %s\n' "$HOME/teletext-services.bak"
      mv $HOME/.teletext-services $HOME/teletext-services.bak
    else
      rm -rf $HOME/.teletext-services/ 2>/dev/null
    fi
    
    vbit-config
  fi
}

main; exit
