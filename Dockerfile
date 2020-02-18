FROM debian:stretch

RUN apt-get update && \
    apt-get install -y build-essential git gcc-multilib && \
    git clone https://github.com/xoreaxeaxeax/movfuscator.git && \
    cd movfuscator && \
    /bin/sh build.sh && \
    /bin/sh install.sh && \
    /bin/sh check.sh



ENTRYPOINT ["/usr/local/bin/movcc"]
