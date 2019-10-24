#!/bin/ash

source /root/.profile

#PID=$(ps w | grep -e "[s]sh -i /root/dragino" | head -n 1 | awk '{print $1}')
PID=$(pgrep  -f "[s]sh -i /root/dragino")
if [ -z "$PID" ]
then
  ssh -i /root/dragino -f -N -R "$SSH_PORT":localhost:22 dragino@"$SSH_HOST" -K 60 -I 577 -y -y
  logger "SSH tunel is offline"
else
  logger "SSH tunel is online"
fi

# kate: replace-tabs on; indent-width 2; mixedindent off; indent-mode cstyle; syntax bash;
