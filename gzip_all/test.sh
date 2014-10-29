dotest ()
{
  rm -f RANDOM.gz RANDOM-`basename $1`.gz
  echo testing: $1
  time $1 -k RANDOM
  mv RANDOM.gz RANDOM-`basename $1`.gz
}

dotestn ()
{
  i=0
  while [ $i -ne $2 ] ;do   echo run $i ;  dotest $1 ; i=`expr $i + 1` ; done
}

nrun=10
#dotestn echo $nrun
dotest ./gzip_u_libz_h $nrun
dotest ./gzip_u $nrun
dotest ./gzip_a $nrun
dotest ./gzip_h $nrun
