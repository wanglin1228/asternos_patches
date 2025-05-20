#!/bin/bash

SONIC_BUILD_PATH='../../..'


cp 0001-sonic-platform-daemons-suit-for-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-platform-daemons/
echo "cp 0001-sonic-platform-daemons-suit-for-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-platform-daemons/"
cd $SONIC_BUILD_PATH/src/sonic-platform-daemons/
echo "cd $SONIC_BUILD_PATH/src/sonic-platform-daemons/"
git apply 0001-sonic-platform-daemons-suit-for-asterfusion-devices.patch
echo "git apply 0001-sonic-platform-daemons-suit-for-asterfusion-devices.patch"
rm 0001-sonic-platform-daemons-suit-for-asterfusion-devices.patch
cd -
cp -r sonic-fand $SONIC_BUILD_PATH/src/sonic-platform-daemons/
echo "cp -r sonic-fand $SONIC_BUILD_PATH/src/sonic-platform-daemons/"

