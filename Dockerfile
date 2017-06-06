FROM bitnami/minideb:jessie

RUN install_packages gcc libc6-dev

COPY syslog2stderr.c /build/
RUN gcc -fPIC -shared -Wall -Wextra -Werror /build/syslog2stderr.c -o /usr/local/lib/syslog2stderr.so

COPY test.c /build/
RUN gcc /build/test.c -o /build/test
