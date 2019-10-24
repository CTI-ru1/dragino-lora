#!/bin/ash

LIMIT="30"
COUNT=$(/bin/ls -lhr /mnt/sda1 | /usr/bin/wc -l)
if [ "$COUNT" -gt "$LIMIT" ]; then
  logger "Count [$COUNT] over $LIMIT"
  FILE=$(/bin/ls -hr /mnt/sda1 | /usr/bin/tail -n 1)
  logger "Will delete $FILE"
  rm /mnt/sda1/"$FILE"
fi

# kate: replace-tabs on; indent-width 2; mixedindent off; indent-mode cstyle; syntax bash;
