prun ()
{
echo $@
$@
}

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
SIZES="64 256 512 4096 16384 65536 500000 1000000 5000000 10000000"
MAXSIZE=50000000
BS=1000000

generate_large_files ()
{
  COUNT=`expr $MAXSIZE / $BS`
  prun rm -f ZERO RANDOM ENTROPY
  prun dd if=/dev/zero of=ZERO bs=$BS count=$COUNT
  prun dd if=/dev/random of=RANDOM bs=$BS count=$COUNT
  cat RANDOM | b64encode - > ENTROPY
}

generate ()
{
# the files RANDOM, ZERO and ENTROPY should contain sensible data
# the following files are automatically generated from them
for sz in $SIZES
do
  prun dd if=ZERO of=ZERO-$sz bs=$sz count=1
  prun dd if=RANDOM of=RANDOM-$sz bs=$sz count=1
  prun dd if=ENTROPY of=ENTROPY-$sz bs=$sz count=1
done
}

clean ()
{
for sz in $SIZES
do
  prun rm -f ZERO-$sz RANDOM-$sz ENTROPY-$sz
done
prun rm -f results*
}

PROGS="gzip_u gzip_u_libz_c gzip_a gzip_u_libz_a gzip_u_libz_h gzip_h gzip_a_libz_c"
DESC_gzip_u="(unmodified gzip + unmodified zlib)"
DESC_gzip_u_libz_c="(unmodified gzip + capability-only zlib)"
DESC_gzip_a="(Capsicum gzip)"
DESC_gzip_u_libz_a="(unmodified gzip + Capsicum zlib)"
DESC_gzip_u_libz_h1="(unmodified gzip + libcheri zlib with single sandbox)"
DESC_gzip_u_libz_hm="(unmodified gzip + libcheri zlib with multiple sandboxes)"
DESC_gzip_h="(libcheri gzip)"
DESC_gzip_a_libz_c="(Capsicum gzip + capability-only zlib)"

runtests ()
{
i=1
for sz in $SIZES
do
  echo "---------BEGIN RUN ($sz bytes) --------"
  echo -e "ZERO-$sz\nRANDOM-$sz\nENTROPY-$sz\n" > file_list
  cat file_list
  for prog in $PROGS
  do
    desc_ref=\$DESC_$prog
    prun dotestn ./$prog $nrun `eval echo $desc_ref`
  done
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
