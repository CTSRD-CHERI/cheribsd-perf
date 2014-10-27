pushd ../libz_h2 && \
make -f Makefile-helper.CHERI clean all && \
make -f Makefile-wrapper.CHERI clean all && \
popd && \
make -f Makefile_libz_h.CHERI clean all && \
make -f Makefile_libz_h.CHERI push
