# syslog2stderr.so

## Experimental redirection of syslog to stderr

This Dockerfile builds a shared library designed for `LD_PRELOAD`ing into another program. Calls to the `syslog(3)` family of libc functions are caught, and output is sent to stderr instead of the syslog socket.

## Caveats

I don't think I'd trust it for production.

1. I don't like how much C code is necessary to get this to work. If I made an error, any network-facing daemon that you use this with might have a new security hole!

1. It relies on the target program using the libc functions to syslog. util-linux's `logger` is one that I know of that writes directly to the syslog socket. We could try to intercept that, but that would be even more libc functions being tampered with, and we'd have to get in the business of skipping the priority, timestamp, etc. written to that socket (at least to match this library's current output when the libc functions are used.)

There is a similar but much simpler and probably safer approach [here](https://gist.github.com/scintill/f332033cf0bd27a0fe693ccd162b61a2), if the target program doesn't change its stderr file descriptor to point at something else.

## Usage
This Docker image is meant to be used with the new multi-stage build feature, like so (assuming `Dockerfile` has been built and tagged as `syslog2stderr.so`):

```Dockerfile
FROM syslog2stderr.so AS syslog2stderr

FROM gliderlabs/alpine:3.4

COPY --from=syslog2stderr /usr/local/lib/syslog2stderr.so /usr/local/lib/

RUN apk-install rsync

ENV LD_PRELOAD=/usr/local/lib/syslog2stderr.so
CMD ["/usr/bin/rsync", "--daemon", "--no-detach"]
```

That Dockerfile should output something like `rsyncd[1]: rsyncd version 3.1.2 starting, listening on port 873` when run. Without `syslog2stderr`, that log entry is lost because the container has no syslog daemon.

Note: in the case of `rsync`, using the `log file` config option in `rsyncd.conf` set to `/dev/stderr` is probably better, but the library is made for daemons that can't be configured this way.

Note: I suppose if your target image is different enough (e.g. different linker/libc versions) from the one that builds the library, this might not work.
