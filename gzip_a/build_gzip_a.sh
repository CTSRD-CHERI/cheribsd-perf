pushd ../libsep && \
make -f Makefile.CHERI clean all && \
popd && \
pushd ../libz_u && \
make -f Makefile.CHERI clean all && \
popd && \
make -f Makefile.CHERI clean all push

