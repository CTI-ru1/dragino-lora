#!/bin/ash
COUNT=$(/bin/ls -lhr /mnt/sda1 | /usr/bin/wc -l)
if [ "$COUNT" -gt "30" ]; then
  logger "Count [$COUNT] over 30"
  FILE=$(/bin/ls -hr /mnt/sda1 | /usr/bin/tail -n 1)
  logger "Will delete $FILE"
  rm /mnt/sda1/$FILE
fi

