#!/bin/ash
cd /root/
rm -rf delete_old_logs.ash check_ssh.ash gaia

wget https://raw.githubusercontent.com/CTI-ru1/dragino-lora/master/dragino-utils/check_ssh.ash
chmod a+x checkssh.ash

wget https://raw.githubusercontent.com/CTI-ru1/dragino-lora/master/dragino-utils/delete_old_logs.ash
chmod a+x delete_old_logs.ash

wget https://raw.githubusercontent.com/CTI-ru1/dragino-lora/master/dragino-utils/gaia
chmod a+x gaia
mv gaia /usr/bin/gaia 

echo "*/5 * * * * /root/check_ssh.ash" > /etc/crontabs/root
echo "* * * * * /root/delete_old_logs.ash" >> /etc/crontabs/root
