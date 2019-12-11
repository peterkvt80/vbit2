#!/bin/bash

# Script to update the active service and any ancillary sub-services

main(){
  CONFIG=$HOME/.teletext-services/config

  if [ -f "$CONFIG" ]; then
    source $CONFIG
  else
    echo no config found, please run vbit-config
    exit 1
  fi

  if [ -z $SELECTED ]; then
    echo no service selected, please run select_service.sh
    exit 1
  fi

  SERVICES=()
  get_services $SELECTED

  for i in ${!SERVICES[@]}; do
    IFS=',' read -r -a SERVICEARRAY <<< "${SERVICES[i]}"
    echo ${SERVICEARRAY[@]}

    if [[ "${SERVICEARRAY[3]}" == "svn" ]]; then
      svn up "${SERVICEARRAY[1]}"
    elif [[ "${SERVICEARRAY[3]}" == "git" ]]; then
      git -C "${SERVICEARRAY[1]}" pull
    fi
  done
}

get_services(){
  for i in ${!INSTALLED[@]}; do
    IFS=',' read -r -a INSTALLEDARRAY <<< "${INSTALLED[i]}"
    if [[ "${INSTALLEDARRAY[0]}" == "$1" ]]; then
      # the service itself
      SERVICES+=("${INSTALLED[i]}")
    fi
    if [[ "${INSTALLEDARRAY[2]}" == "$1" ]]; then
      # recurse down into any sub-services
      get_services "${INSTALLEDARRAY[0]}"
    fi
  done
}

main; exit

