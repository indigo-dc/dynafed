#!/bin/bash

#
# This script performs a remote file copy using gfal-copy
#

echo "Starting a PUSH TPC ckcheck:$1 cktype:$2 src:$3 dst:$4 delgproxy:$5 xferhdrauth:$6 usr1:$7 usr2:$8 usr3:$9"

#sleep 5 

# Some debug output
#mount
#df
#ls /

# Note this may breaks gfal, depending on the version
# If the bearer token field comes from the TransferHeaderAuthorization header field
# then we splat it into this envvar. This is plain wrong on the side of gfal,
# as it assumes that there is only one token, while a transfer may need two
if [ "x$6" != "x" ]; then
  echo "Setting BEARER_TOKEN to '$6'"
  export BEARER_TOKEN="$6"
fi

if [ "x$5" == "x" ]
then
  cmd="gfal-copy -vv --just-copy $3 $4"
else
  cmd="gfal-copy -vv --just-copy --cert $5 --key $5 $3 $4"
fi

echo "command: $cmd"
$cmd "-DHTTP PLUGIN:ENABLE_REMOTE_COPY=false" 2>&1
res=$?
echo "res: $res"

if [ "x$res" != "x0" ]; then
  echo "Push TPC failed. Deleting local attempt '$4'"
fi

echo "finished"
exit $res
