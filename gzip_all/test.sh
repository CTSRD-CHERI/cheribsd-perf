dotest ()
{
  #rm -f RANDOM.gz RANDOM-`basename $1`.gz
  echo testing: $@
  cat file_list | time xargs -n 1 $1 -c > /dev/null
  #mv RANDOM.gz RANDOM-`basename $1`.gz
}

dotestn ()
{
  i=0
  while [ $i -ne $2 ] ; do dotest $1 $3 ; i=`expr $i + 1` ; done
}

mult=10000
nrun=2
ntestrun=1

generate ()
{
# the files RANDOM, ZERO and ENTROPY should contain sensible data
# the following files are automatically generated from them
i=1
while [ $i -ne $nrun ]
do
  sz=`expr $i \* $mult`
  echo dd if=ZERO of=ZERO$i bs=$sz count=1
  dd if=ZERO of=ZERO$i bs=$sz count=1
  dd if=RANDOM of=RANDOM$i bs=$sz count=1
  dd if=ENTROPY of=ENTROPY$i bs=$sz count=1
  i=`expr $i + 1`
done
}

i=1
while [ $i -ne $nrun ]
do
  #sz=`expr $i \* $mult`
  echo "---------BEGIN RUN $i ($sz bytes) --------"
  #echo -e "ZERO$i\nRANDOM$i\nENTROPY$i\n" > file_list
  echo -e "ZERO\nRANDOM\nENTROPY\n" > file_list
  cat file_list
  dotestn ./gzip_u $ntestrun "(unmodified gzip+zlib)"
  dotestn ./gzip_a $ntestrun "(Capsicum gzip)"
  dotestn ./gzip_u_libz_h $ntestrun "(libcheri zlib)"
  dotestn ./gzip_h $ntestrun "(libcheri gzip)"
  echo "---------END RUN $i ($sz bytes) --------"
  i=`expr $i + 1`
done
