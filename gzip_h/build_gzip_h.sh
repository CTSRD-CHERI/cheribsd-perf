pushd ../libsep
make -f Makefile.CHERI clean all
popd
pushd ../libz_u
make -f Makefile.CHERI clean all
popd
make -f Makefile-helper.CHERI clean all push
make -f Makefile.CHERI clean all push

