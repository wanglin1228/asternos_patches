#!/bin/bash

SONIC_BUILD_PATH='../../..'

cp 0001-sonic-utilities-suit-for-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-utilities/
echo "cp 0001-sonic-utilities-suit-for-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-utilities/"
cd $SONIC_BUILD_PATH/src/sonic-utilities/
echo "cd $SONIC_BUILD_PATH/src/sonic-utilities/"
git apply 0001-sonic-utilities-suit-for-asterfusion-devices.patch 
echo "git apply 0001-sonic-utilities-suit-for-asterfusion-devices.patch"
rm 0001-sonic-utilities-suit-for-asterfusion-devices.patch
cd -