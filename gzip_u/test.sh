dotest ()
{
  rm -f RANDOM.gz
  time $1 -k RANDOM
  sha1 RANDOM.gz
}

dotestn ()
{
  i=0
  while [ $i -ne $2 ] ; do dotest $1 ; done
}

nrun=10
dotestn ./gzip_u $nrun
dotestn ./gzip_u_libz_h $nrun
