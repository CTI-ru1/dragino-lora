#!/bin/ash

source /root/.profile

logger "reporting to base"
/usr/bin/gaia "status: dragino connected $(/sbin/ifconfig eth1 | /bin/grep 'Link\|inet addr:'  | /bin/grep 'Bcast' | /usr/bin/awk '{print $2}')"

# kate: replace-tabs on; indent-width 2; mixedindent off; indent-mode cstyle; syntax bash;
