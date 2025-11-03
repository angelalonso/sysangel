#!/usr/bin/bash

FILE="/etc/resolv.conf"
TMPFILE="resolv.conf.tmp"
NSRV="nameserver 8.8.8.8"

ALLGOOD=$(grep "$NSRV" ${FILE} | wc -l)
if [[ ${ALLGOOD} -gt 0 ]]; then
  exit 0
else
  echo -n Modifying ${FILE}...
  cp ${FILE} ${TMPFILE}
  echo ${NSRV} | sudo tee ${FILE} > /dev/null
  cat ${TMPFILE} | sudo tee -a ${FILE} > /dev/null
  echo DONE
fi
rm ${TMPFILE}
exit 0
