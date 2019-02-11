logger "reporting to base"
source /root/.profile
/usr/bin/gaia "status: dragino connected $(/sbin/ifconfig eth1 | /bin/grep 'Link\|inet addr:'  | /bin/grep 'Bcast' | /usr/bin/awk '{print $2}')"

