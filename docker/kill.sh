#!/bin/bash

docker kill `docker ps | grep "symfusion-runner-s" | awk '{ print $1 }'`

exit 0
