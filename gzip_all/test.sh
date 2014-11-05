nrun=1
#SIZES="64 256 512 4096 16384 65536 500000 1000000 5000000 10000000"
#SIZES="64 4096 65536 500000 1000000"
SIZES="4096 65536 500000"
MAXSIZE=50000000
BS=$MAXSIZE

# for sb_create_test:
NFILES="1 2 5 10"
SB_CREATE_FIXED_SIZE=500000
ZERO="0"

# for buflen_test:
BUFLENS="4096 65536 131072"

#PROGS="gzip_u gzip_u_libz_c gzip_a gzip_u_libz_a gzip_u_libz_h1 gzip_u_libz_hm gzip_h gzip_a_libz_c"
PROGS="gzip_u gzip_h"
DESC_gzip_u="(unmodified gzip + unmodified zlib)"
DESC_gzip_u_libz_c="(unmodified gzip + capability-only zlib)"
DESC_gzip_a="(Capsicum gzip)"
DESC_gzip_u_libz_a="(unmodified gzip + Capsicum zlib)"
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
  cat file_list | time xargs -n 1 $1 -B65536 -c 1>/dev/null 2>>$results_file
  check_results
}
gen_compress_time_file_list ()
{
  sz=$outer_var
  echo -e "ZERO-$sz\nRANDOM-$sz\nENTROPY-$sz\n" > file_list
}
init_compress_time_test ()
{
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
}

# define the function to call for each test in $func
dotestn ()
{
  b=`basename $1`
  rm -f results$CASE-$func-$b-$outer_var
  j=0
  while [ $j -ne $2 ]
  do
    echo "---$func--- `expr $j + 1`/$2 runs for $1 ($outer_var $outer_loop_unit)"
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
  for sz in $SIZES $SB_CREATE_FIXED_SIZE
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
      if [ $inner_var -ne 0 ]
      then
        echo "(inner: $inner_var $inner_loop_unit)"
      fi
      $test_input_gen_func
      cat file_list
      for prog in $PROGS
      do
        desc_ref=\$DESC_$prog
        desc=`eval echo $desc_ref`
        dotestn ./$prog $nrun
      done
      echo "! inner: $inner_counter / $inner_max outer: $outer_counter / $outer_max"
    done
    echo "---------END RUN ($outer_var $outer_loop_unit) --------"
  done
}

runtests ()
{
  rm -f failures
  #runtest compress_verify_test init_compress_verify_test gen_compress_verify_file_list SIZES bytes ZERO ""
  #runtest buflen_test init_buflen_test gen_buflen_file_list SIZES bytes BUFLENS bytes
  #runtest sb_create_test init_sb_create_test gen_sb_create_file_list SIZES bytes NFILES files
  #runtest compress_time_test init_compress_time_test gen_compress_time_file_list SIZES bytes ZERO ""

  # perfnotes3: case 1
  BUFLENS="4096 65536 131072"
  SIZES="4096 65536 500000"
  PROGS="gzip_u gzip_u_libz_h1"
  CASE=1
  runtest buflen_test init_buflen_test gen_buflen_file_list SIZES bytes BUFLENS bytes

  # perfnotes3: case 2
  SIZES="4096 65536 500000"
  PROGS="gzip_u gzip_u_libz_h1 gzip_u_libz_a"
  CASE=2
  runtest compress_time_test init_compress_time_test gen_compress_time_file_list SIZES bytes ZERO ""
  
  # perfnotes3: case 3
  NFILES="1 5 10"
  SIZES="500000"
  PROGS="gzip_u_libz_h1 gzip_u_libz_hm"
  CASE=3
  runtest sb_create_test init_sb_create_test gen_sb_create_file_list SIZES bytes NFILES files
  
  # perfnotes3: case 4
  SIZES="4096 65536 500000"
  PROGS="gzip_u gzip_u_libz_c gzip_a gzip_a_libz_c"
  CASE=4
  runtest compress_time_test init_compress_time_test gen_compress_time_file_list SIZES bytes ZERO ""

  cat failures
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
