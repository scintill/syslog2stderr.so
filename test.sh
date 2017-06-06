#!/bin/bash
set -euxo pipefail

TAG=syslog2stderr.so
sudo docker build -t $TAG .

# should output nothing because there is no syslogd to report it
sudo docker run $TAG /build/test

# should output to stderr
sudo docker run $TAG env LD_PRELOAD=/usr/local/lib/syslog2stderr.so /build/test
