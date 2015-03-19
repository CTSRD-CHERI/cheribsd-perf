nrun=1
SIZES="4096 65536 500000"
MAXSIZE=10000000
BS=$MAXSIZE

ZERO="0"

DESC_gzip_u="(unmodified gzip + unmodified zlib)"
DESC_gzip_u_libz_c="(unmodified gzip + capability-only zlib)"
DESC_gzip_a="(Capsicum gzip)"
DESC_gzip_u_libz_a1="(unmodified gzip + Capsicum zlib with single sandbox)"
DESC_gzip_u_libz_am="(unmodified gzip + Capsicum zlib with multiple sandboxes)"
DESC_gzip_u_libz_h1="(unmodified gzip + libcheri zlib with single sandbox)"
DESC_gzip_u_libz_hm="(unmodified gzip + libcheri zlib with multiple sandboxes)"
DESC_gzip_h="(libcheri gzip)"
DESC_gzip_a_libz_c="(Capsicum gzip + capability-only zlib)"

prun ()
{
  echo $@
  $@
}

check_results ()
{
  # only show errors:
  cat $results_file | grep -v ".*real.*user.*sys$\|^\[stat\]"
  rc=$?
  if [ $rc -eq 0 ]
  then
    echo "*****FAILED: $@ (file: $results_file)-----"
  fi
}

DESC_compress_time_test="tests compression time"
compress_time_test ()
{
  sz=$outer_var
  results_file=results$CASE-$func-$b-$sz
  time $1 -B65536 -kc DATA-$sz 1>/dev/null 2>>$results_file
  check_results
}
gen_compress_time_file_list ()
{
}
init_compress_time_test ()
{
  generate
}

DESC_sb_create_test="tests sandbox creation overhead"
sb_create_test ()
{
  sz=$outer_var
  results_file=results$CASE-$func-$b-$sz-$nfiles
  cat file_list | xargs echo "$1 -B65536 -c"
  cat file_list | time xargs $1 -B65536 -c 1>/dev/null 2>>$results_file
  check_results
}
gen_sb_create_file_list ()
{
  sz=$outer_var
  nfiles=$inner_var
  rm -f file_list
  i=0
  while [ $i -lt $nfiles ]
  do
    echo -e "DATA-$sz\n" >> file_list
    i=`expr $i + 1`
  done
}
init_sb_create_test ()
{
  generate
}

DESC_buflen_test="tests varying buffer size"
buflen_test ()
{
  sz=$outer_var
  buflen=$inner_var
  results_file=results$CASE-$func-$b-$sz-$buflen
  cat file_list | time xargs $1 -B $buflen -c 1>/dev/null 2>>$results_file
  cat file_list | xargs echo $1 -B $buflen -c
  check_results
}
gen_buflen_file_list ()
{
  sz=$outer_var
  rm -f file_list
  i=0
  echo -e "DATA-$sz\n" >> file_list
}
init_buflen_test ()
{
  generate
}

DESC_compress_verify_test="tests compression correctness"
compress_verify_test ()
{
  sz=$outer_var
  rm -f ZERO-$sz.gz RANDOM-$sz.gz ENTROPY-$sz.gz
  cat file_list | xargs -n 1 $1 -B65536 -k 2>>results$CASE-$func-$b-$sz
  sha1 ZERO-$sz.gz RANDOM-$sz.gz ENTROPY-$sz.gz > hashes-$b-$sz
  diff hashes-$sz hashes-$b-$sz
  rc=$?
  if [ $rc -ne 0 ]
  then
    echo "*****FAILED: $@"
    echo Failed: $b $sz >> failures
  fi
}
gen_compress_verify_file_list ()
{
  sz=$outer_var
  rm -f ZERO-$sz.gz RANDOM-$sz.gz ENTROPY-$sz.gz
  echo -e "ZERO-$sz\nRANDOM-$sz\nENTROPY-$sz\n" > file_list
  cat file_list | xargs -n 1 gzip -k
  sha1 ZERO-$sz.gz RANDOM-$sz.gz ENTROPY-$sz.gz > hashes-$sz
  cat hashes-$sz
}
init_compress_verify_test ()
{
  generate
}

# define the function to call for each test in $func
dotestn ()
{
  b=`basename $1`
  rm -f results$CASE-$func-$b-$outer_var
  j=0
  while [ $j -ne $2 ]
  do
    echo "---$func--- `expr $j + 1`/$2 runs for $1 ($outer_var $outer_loop_unit; $inner_var $inner_loop_unit)"
    $func $1 $desc
    j=`expr $j + 1`
  done
}

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
    szdiv3=`expr $sz / 3`
    dd if=ZERO bs=$szdiv3 count=1 > DATA-$sz
    dd if=RANDOM bs=$szdiv3 count=1 >> DATA-$sz
    dd if=ENTROPY bs=$szdiv3 count=1 >> DATA-$sz
  done
}

clean ()
{
  prun rm -f ZERO-* RANDOM-* ENTROPY-* DATA-*
  prun rm -f file_list results* hashes* failures
}

cleancase ()
{
  if [ ! -n "$CASE" ]
  then
    echo no case
  else
    prun rm -f file_list results$CASE* hashes$CASE* failures
  fi
}

# runtest runs 3-level-deep nested loop. The outermost loop iterates 
# over outer_loop_array. The next inner loop iterates over
# inner_loop_array. The innermost loop iterates over each test
# program. runtest itself does not run the tests directly: dotestn
# does that. However, parameters to control dotestn are required.
#
# Note that dotestn itself runs another loop, $nrun many times, for
# the final averaging process.
#
# So in total, the nesting is 4 levels deep.
#
# Parameters:
#
# $1: test_func: the function to call at the very innermost loop in
#     dotestn.
# $2: test_input_init_func: the function to call to initialize the
#     test. This is called exactly once, before the main loop.
# $3: test_input_gen_func: the function to call at each iteration of
#     inner_loop_array.
# $4: outer_loop_array_ref: the name of the variable containing the
#     items to iterate over in the outer loop.
# $5: outer_loop_unit: the units of the elements in outer_loop_array.
# $6: inner_loop_array_ref: the name of the variable containing the
#     items to iterate over in the inner loop.
# $7: inner_loop_unit: the units of the elements in inner_loop_array.
runtest ()
{
  i=1
  test_func=$1
  func=$test_func
  test_desc_ref=\$DESC_$test_func
  test_desc=`eval echo $test_desc_ref`
  test_input_init_func=$2
  test_input_gen_func=$3
  outer_loop_array_ref=\$$4
  outer_loop_array=`eval echo $outer_loop_array_ref`
  outer_loop_unit=$5
  inner_loop_array_ref=\$$6
  inner_loop_array=`eval echo $inner_loop_array_ref`
  inner_loop_unit=$7
  outer_counter=0
  inner_counter=0
  outer_max=`echo $outer_loop_array | wc -w | tr -d " "`
  inner_max=`echo $inner_loop_array | wc -w | tr -d " "`
  echo runtest: $test_func: $test_desc
  $test_input_init_func
  for outer_var in $outer_loop_array
  do
    outer_counter=`expr $outer_counter + 1`
    inner_counter=0
    echo "---------BEGIN RUN ($outer_var $outer_loop_unit) --------"
    for inner_var in $inner_loop_array
    do
      inner_counter=`expr $inner_counter + 1`
      echo "! inner: $inner_counter / $inner_max outer: $outer_counter / $outer_max"
      $test_input_gen_func
      cat file_list
      for prog in $PROGS
      do
        desc_ref=\$DESC_$prog
        desc=`eval echo $desc_ref`
        dotestn ./$prog $nrun
      done
    done
    echo "---------END RUN ($outer_var $outer_loop_unit) --------"
  done
}

case1 ()
{
  # perfnotes3: case 1
  CASE=1
  if [ ! -n "$LONG_RUN" ]
  then
    nrun=4
    SIZES="500000"
    BUFLENS="64 4096 65536 131072 1000000"
    PROGS="gzip_u gzip_u_libz_c gzip_u_libz_h1"
  else
    nrun=4
    SIZES="10000000"
    BUFLENS="64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 1000000"
    PROGS="gzip_u gzip_u_libz_c gzip_u_libz_h1"
  fi
  runtest buflen_test init_buflen_test gen_buflen_file_list SIZES bytes BUFLENS bytes
}

case2 ()
{
  # perfnotes3: case 2
  CASE=2
  if [ ! -n "$LONG_RUN" ]
  then
    nrun=1
    SIZES="4096 65536 500000"
    PROGS="gzip_u gzip_u_libz_h1 gzip_u_libz_a1"
  else
    nrun=3
    SIZES="4096 32768 60000 65536 70000 131072 500000 1000000"
    PROGS="gzip_u gzip_u_libz_h1 gzip_u_libz_a1"
  fi
  runtest compress_time_test init_compress_time_test gen_compress_time_file_list SIZES bytes ZERO ""
}

case3 ()
{
  # perfnotes3: case 3
  CASE=3
  if [ ! -n "$LONG_RUN" ]
  then
    nrun=3
    SIZES="500000"
    NFILES="1 32 128"
    PROGS="gzip_u_libz_am gzip_u_libz_a1 gzip_u_libz_a1_shmem"
  else
    nrun=11
    SIZES="500000"
    NFILES="1 2 4 8 16 32 64 128"
    PROGS="gzip_u gzip_u_libz_h1 gzip_u_libz_hm gzip_u_libz_am gzip_u_libz_a1 gzip_u_libz_a1_shmem"
  fi
  runtest sb_create_test init_sb_create_test gen_sb_create_file_list SIZES bytes NFILES files
}

case4 ()
{
  # perfnotes3: case 4
  CASE=4
  if [ ! -n "$LONG_RUN" ]
  then
    nrun=1
    SIZES="4096 65536 500000"
    PROGS="gzip_u gzip_u_libz_c gzip_a gzip_a_libz_c"
  else
    nrun=11
    SIZES="4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608"
    PROGS="gzip_u gzip_h gzip_a gzip_u_libz_h1 gzip_u_libz_a1 gzip_u_libz_a1_shmem"
  fi
  runtest compress_time_test init_compress_time_test gen_compress_time_file_list SIZES bytes ZERO ""
}

case5 ()
{
  CASE=5
  if [ ! -n "$LONG_RUN" ]
  then
    nrun=1
    SIZES="4096 524288"
    PROGS="gzip_u gzip_u_libz_h1 gzip_u_libz_hj1"
  else
    nrun=3
    SIZES="4096 8192 524288 1048576 2097152"
    PROGS="gzip_u gzip_u_libz_h1 gzip_u_libz_hj1"
  fi
  runtest compress_time_test init_compress_time_test gen_compress_time_file_list SIZES bytes ZERO ""
}

case6 ()
{
  CASE=6
  if [ ! -n "$LONG_RUN" ]
  then
    nrun=1
    SIZES="524288 1048576 2097152"
    BUFLENS="512 4096 65536"
    PROGS="gzip_u gzip_u_libz_h1 gzip_u_libz_hj1"
  else
    nrun=3
    SIZES="524288 1048576 2097152"
    BUFLENS="512 1024 4096 65536 131072"
    PROGS="gzip_u gzip_u_libz_h1 gzip_u_libz_hj1"
  fi
  runtest buflen_test init_buflen_test gen_buflen_file_list SIZES bytes BUFLENS bytes
}

runtests ()
{
  rm -f failures
  #runtest compress_verify_test init_compress_verify_test gen_compress_verify_file_list SIZES bytes ZERO ""
  #runtest buflen_test init_buflen_test gen_buflen_file_list SIZES bytes BUFLENS bytes
  #runtest sb_create_test init_sb_create_test gen_sb_create_file_list SIZES bytes NFILES files
  #runtest compress_time_test init_compress_time_test gen_compress_time_file_list SIZES bytes ZERO ""

  #LONG_RUN=1
  #case1
  #case2
  #case3
  #case4
  case5
  case6
  
  cat failures
}

push ()
{
  rm -f results.tar
  tar cf results.tar results*
  n localhost 8898 results.tar results.tar
}

all ()
{
  clean
  generate
  runtests
  push
}

$@
