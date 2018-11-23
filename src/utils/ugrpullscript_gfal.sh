#!/bin/bash

#
# This script performs a remote file copy using gfal-copy
#

echo "Starting a PULL TPC ckcheck:$1 cktype:$2 src:$3 dst:$4 delgproxy:$5 xferhdrauth:$6 usr1:$7 usr2:$8 usr3:$9"

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

# NOTE: if the parameter glb.filepullhook.usereplicaurl is false (default)
# this script has to be customized by inserting the URL prefix for Ugr
# before $4
# if the parameter glb.filepullhook.usereplicaurl is true then the commandline
# can be simplified as follows:
# cmd="gfal-copy -vv --just-copy --cert $5 --key $5 $3 $4"
# In the latter case, if any S3 bucket is federated, it will not be possible to
# pull files larger than 5GB (which is an S3 limit)
# 
cmd="gfal-copy -vv --just-copy --cert $5 --key $5 $3 https://fab-dynafed-dev1.cern.ch/$4"
echo "command: $cmd"
$cmd "-DHTTP PLUGIN:ENABLE_REMOTE_COPY=false" 2>&1
res=$?
echo "res: $res"

if [ "x$res" != "x0" ]; then
  echo "Pull TPC failed. Deleting local attempt '$4'"
fi

echo "finished"
exit $res
