pushd ../libsep && \
make -f Makefile.CHERI clean all && \
popd && \
pushd ../libz_u && \
make -f Makefile.CHERI clean all && \
popd && \
make -f Makefile.CHERI clean all && \
make -f Makefile-helper.CHERI clean all && \
make -f Makefile.CHERI push && \
make -f Makefile-helper.CHERI push

