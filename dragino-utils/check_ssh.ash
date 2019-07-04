source /root/.profile
PID=$(ps w | grep -e "[s]sh -i /root/dragino" | head -n 1 | awk '{print $1}')
if [ -z "$PID" ]
then
  ssh -i /root/dragino -f -N -R $SSH_PORT:localhost:22 dragino@$SSH_HOST -K 60 -I 587 -yy
  logger "SSH tunel is offline"
else
  logger "SSH tunel is online"
fi
