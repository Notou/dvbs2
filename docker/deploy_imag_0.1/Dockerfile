FROM m1mbert/cxlb-gnuradio-3.10:1.2

SHELL [ "/bin/bash", "-lc"  ]
RUN git clone  https://github.com/Notou/dvbs2.git
RUN cd dvbs2 && git submodule update --init --recursive --remote
RUN cd dvbs2 && mkdir build
RUN cd dvbs2/build && cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -msse4.1" -DAFF3CT_LINK_HWLOC=OFF -DAFF3CT_OVERRIDE_VERSION=0.1.0 -DCMAKE_INSTALL_PREFIX=/cortexlab/toolchains/current ..
RUN cd dvbs2/build && make install -j$(nproc) && ldconfig

# ENTRYPOINT [ "/bin/bash", "-l", ";" ]
# CMD [ "/bin/bash", "-lc", "/usr/sbin/sshd -p 2222 -D" ]
# CMD [ "/bin/bash", "-lc", "/usr/sbin/sshd", "-p", "2222", "-D" ]