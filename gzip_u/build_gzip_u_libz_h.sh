pushd ../libcheri_u && \
make -f Makefile.CHERI clean all && \
popd && \
pushd ../libz_h2 && \
make -f Makefile.CHERI clean all ; \
make -f Makefile-helper.CHERI clean all && \
make -f Makefile-wrapper.CHERI clean all && \
popd && \
make -f Makefile_libz_h.CHERI clean all && \
make -f Makefile_libz_h.CHERI push
