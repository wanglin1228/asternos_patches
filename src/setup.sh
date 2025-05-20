#!/bin/bash

for dir in */; do
    if [ -d "$dir" ]; then
        echo "Processing directory: $dir"
        cd $dir
        if [ -x "setup.sh" ]; then
            ./setup.sh
            if [ $? -eq 0 ]; then
                echo "setup.sh success"
            else
                echo "setup.sh failed"
            fi
        else
            echo "$dir doesn't exist setup.sh"
        fi
        cd ../
    fi
done