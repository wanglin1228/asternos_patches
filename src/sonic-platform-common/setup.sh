#!/bin/bash

SONIC_BUILD_PATH='../../..'


cp 0001-sonic-platform-common-suit-for-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-platform-common/
echo "cp 0001-sonic-platform-common-suit-for-asterfusion-devices.patch $SONIC_BUILD_PATH/src/sonic-platform-common/"
cd $SONIC_BUILD_PATH/src/sonic-platform-common/
echo "cd $SONIC_BUILD_PATH/src/sonic-platform-common/"
git apply 0001-sonic-platform-common-suit-for-asterfusion-devices.patch
echo "git apply 0001-sonic-platform-common-suit-for-asterfusion-devices.patch"
rm 0001-sonic-platform-common-suit-for-asterfusion-devices.patch
cd -