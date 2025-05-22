#!/bin/bash

COMMIT_ID='88617c7765e90837e5ff8f1a9d16984e977675a3'
current_path=$(pwd)
SONIC_BUILD_PATH=$(dirname "$current_path")
cd $SONIC_BUILD_PATH/
current_commit_id=$(git rev-parse HEAD)
cd $current_path

if [ "${current_commit_id}" != "$COMMIT_ID" ]; then
    echo "must rebase to $COMMIT_ID"
    exit
fi

for file in device files platform rules;do
    cp -r $file/* $SONIC_BUILD_PATH/$file/
done

cp *.patch $SONIC_BUILD_PATH/
cd $SONIC_BUILD_PATH/
git apply 0001-sonic-buildimage-support-asterfusion-devices.patch
rm 0001-sonic-buildimage-support-asterfusion-devices.patch
cd $current_path

cd src/
echo "cd src/"
./setup.sh
echo "apply patches successfully"
