pushd ../libcheri_u && \
make -f Makefile.CHERI clean && \
popd && \
pushd ../libz_u && \
make -f Makefile.CHERI clean && \
popd && \
make -f Makefile.CHERI clean && \
make -f Makefile-helper.CHERI clean

