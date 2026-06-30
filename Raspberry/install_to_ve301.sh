#!/bin/bash
echo "ssh root@192.168.178.25 'systemctl stop ve301'"
ssh root@192.168.178.25 'systemctl stop ve301'
echo "ssh root@192.168.178.25 'cp /usr/bin/ve301 /usr/bin/ve301.`date +"%Y%m%d%H%M%S"`'"
ssh root@192.168.178.25 'cp /usr/bin/ve301 /usr/bin/ve301.'`date +"%Y%m%d%H%M%S"` 
echo "scp ve301 root@192.168.178.25:/usr/bin"
scp ve301 root@192.168.178.25:/usr/bin
echo "ssh root@192.168.178.25 'systemctl start ve301'"
ssh root@192.168.178.25 'systemctl start ve301'
