#!/bin/bash

SONIC_BUILD_PATH='../../../'

cp 0001-sonic-sairedis-suit-for-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-sairedis/
echo "cp 0001-sonic-sairedis-suit-for-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-sairedis/"
cd $SONIC_BUILD_PATH/src/sonic-sairedis/
echo "cd $SONIC_BUILD_PATH/src/sonic-sairedis/"
git apply 0001-sonic-sairedis-suit-for-asterfusion-devices.patch
echo "git apply 0001-sonic-sairedis-suit-for-asterfusion-devices.patch"
rm 0001-sonic-sairedis-suit-for-asterfusion-devices.patch
cd -