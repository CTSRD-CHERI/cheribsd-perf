pushd ../libz_h && \
make -f Makefile.CHERI clean all && \
popd && \
make -f Makefile_libz_h.CHERI clean all && \
make -f Makefile_libz_h.CHERI push
