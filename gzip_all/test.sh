dotest ()
{
  echo testing: $@
  echo file: results-$b-$sz
  cat file_list | time xargs -n 1 $1 -c 1>/dev/null 2>>results-$b-$sz
  cat results-$b-$sz
}

dotestn ()
{
b=`basename $1`
rm -f results-$b-$sz
j=0
while [ $j -ne $2 ]
do
  echo j=$j runs=$2
  dotest $1 $3
  j=`expr $j + 1`
done
}

nrun=3
SIZES="64 256"
# 512 4096"

generate ()
{
# the files RANDOM, ZERO and ENTROPY should contain sensible data
# the following files are automatically generated from them
for sz in $SIZES
do
  echo dd if=ZERO of=ZERO-$sz bs=$sz count=1
  dd if=ZERO of=ZERO-$sz bs=$sz count=1
  echo dd if=RANDOM of=RANDOM-$sz bs=$sz count=1
  dd if=RANDOM of=RANDOM-$sz bs=$sz count=1
  echo dd if=ENTROPY of=ENTROPY-$sz bs=$sz count=1
  dd if=ENTROPY of=ENTROPY-$sz bs=$sz count=1
done
}

clean ()
{
for sz in $SIZES
do
  echo rm -f ZERO-$sz RANDOM-$sz ENTROPY-$sz
  rm -f ZERO-$sz RANDOM-$sz ENTROPY-$sz
done
echo rm -f results*
rm -f results*
}

runtests ()
{
i=1
for sz in $SIZES
do
  echo "---------BEGIN RUN ($sz bytes) --------"
  echo -e "ZERO-$sz\nRANDOM-$sz\nENTROPY-$sz\n" > file_list
  cat file_list
  dotestn ./gzip_u $nrun "(unmodified gzip + unmodified zlib)"
  dotestn ./gzip_u_libz_c $nrun "(unmodified gzip + capability-only zlib)"
  dotestn ./gzip_a $nrun "(Capsicum gzip)"
  dotestn ./gzip_u_libz_a $nrun "(unmodified gzip + Capsicum zlib)"
  dotestn ./gzip_u_libz_h $nrun "(unmodified gzip + libcheri zlib)"
  dotestn ./gzip_h $nrun "(libcheri gzip)"
  echo "---------END RUN ($sz bytes) --------"
done
}

push ()
{
  rm -f results.tar
  tar cf results.tar results*
  ~/myncp/myproto 192.168.1.99 8888 results.tar results.tar
}

all ()
{
  clean
  generate
  runtests
  push
}

$@
