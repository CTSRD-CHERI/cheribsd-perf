prun ()
{
  echo $@
  $@
}

DESC_time_test="tests compression time"
time_test ()
{
  cat file_list | time xargs -n 1 $1 -c 1>/dev/null 2>>results-$b-$sz
  # only show errors:
  cat results-$b-$sz | grep -v ".*real.*user.*sys$"
  rc=$?
  if [ $rc -eq 0 ]
  then
    echo "*****FAILED: $@ (file: results-$b-$sz)-----"
  fi
}

DESC_sb_create_test="tests sandbox creation overhead"
sb_create_test ()
{
  cat file_list | xargs echo "$1 -c"
  cat file_list | time xargs $1 -c 1>/dev/null 2>>results-sb-create-$b-$sz
  cat results-sb-create-$b-$sz | grep -v ".*real.*user.*sys$"
  rc=$?
  if [ $rc -eq 0 ]
  then
    echo "*****FAILED: $@ (file: results-sb-create-$b-$sz)-----"
  fi
}

# define the function to call for each test in $func
dotestn ()
{
b=`basename $1`
rm -f results-$b-$sz
j=0
while [ $j -ne $2 ]
do
  echo "---$func--- `expr $j + 1`/$2 runs for $1 ($sz bytes)"
  $func $1 $desc
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

runtest ()
{
i=1
test_func=$1
func=$test_func
test_desc_ref=\$DESC_$test_func
test_desc=`eval echo $test_desc_ref`
echo runtest: $test_func: $test_desc
for sz in $SIZES
do
  echo "---------BEGIN RUN ($sz bytes) --------"
  echo -e "ZERO-$sz\nRANDOM-$sz\nENTROPY-$sz\n" > file_list
  cat file_list
  for prog in $PROGS
  do
    desc_ref=\$DESC_$prog
    desc=`eval echo $desc_ref`
    dotestn ./$prog $nrun
  done
  echo "---------END RUN ($sz bytes) --------"
done
}

runtests ()
{
runtest sb_create_test
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
