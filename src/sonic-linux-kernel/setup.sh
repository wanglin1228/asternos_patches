#!/bin/bash

SONIC_BUILD_PATH='../../..'

cp 0001-sonic-linux-kernel-support-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-linux-kernel/
echo "cp 0001-sonic-linux-kernel-support-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-linux-kernel/"
cd $SONIC_BUILD_PATH/src/sonic-linux-kernel/
echo "cd $SONIC_BUILD_PATH/src/sonic-linux-kernel/"
git apply 0001-sonic-linux-kernel-support-asterfusion-devices.patch
echo "git apply 0001-sonic-linux-kernel-support-asterfusion-devices.patch"
rm 0001-sonic-linux-kernel-support-asterfusion-devices.patch
cd -
