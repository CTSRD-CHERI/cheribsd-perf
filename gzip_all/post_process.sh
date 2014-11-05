extract_avg ()
{
  if [ $nruns -lt 3 ]
  then
    avg_value=`cat $stat_file`
  else
    # ministat-format dependent:
    avg_field=6
    avg_value=`tail -n 1 $stat_file | awk '{print $avg_field;}' avg_field=$avg_field`
  fi
}

run_ministat ()
{
  if [ $nruns -lt 3 ]
  then
    cat tmp | awk '{print $1}' > $stat_file
  else
    cat tmp | ministat -n > $stat_file
  fi
}

extract_stats ()
{
  nruns=`cat $file | sed "s/^\[stat\].*$//g" | sed "/^$/d" | wc -l | tr -d " "`

  # find average time taken
  stat_file=stat-${filep}_time
  cat $file | sed "s/^\[stat\].*$//g" > tmp
  run_ministat
  extract_avg
  avg_time=$avg_value

  # find average number of ccalls
  stat_file=stat-${filep}_ccalls
  cat $file | grep "^\[stat\] Number of CCalls" | awk '{print $NF;}' > tmp
  run_ministat
  extract_avg
  avg_ccalls=$avg_value
  
  # find average number of CHERI sandbox creations
  stat_file=stat-${filep}_cheri_sandboxes
  cat $file | grep "^\[stat\] Number of CHERI sandboxes" | awk '{print $NF;}' > tmp
  run_ministat
  extract_avg
  avg_cheri_sandboxes=$avg_value
  
  # find average number of Capsicum host RPCs
  stat_file=stat-${filep}_capsicum_host_rpcs
  cat $file | grep "^\[stat\] Number of Capsicum host RPCs" | awk '{print $NF;}' > tmp
  run_ministat
  extract_avg
  avg_capsicum_host_rpcs=$avg_value

  # extract the BUFLEN used
  BUFLEN=`cat $file | grep "^\[stat\] BUFLEN" | awk '{print $NF;}'`
}

append_curve ()
{
  y_ref=\$$y_var
  x_ref=\$$x_var
  echo `eval echo $x_ref` `eval echo $y_ref` >> data-$prefix-$y_var-$impl
}

process_file ()
{
  # filename is of the form results-<prefix>-<implementation>-<outer loop variable>-<inner loop variable>
  # any of these might also usefully be an x_var
  impl=`echo $file | sed s/^results-$prefix-/results-/g | awk -F"-" '{print $2}'`
  x_outer=`echo $file | sed s/^results-$prefix-/results-/g | awk -F"-" '{print $3}'`
  x_inner=`echo $file | sed s/^results-$prefix-/results-/g | awk -F"-" '{print $4}'`
  filep=`echo $file | sed s/^results-//g`
  extract_stats
  append_curve
}

generate_graph ()
{
  cat <<EOF >plot-$prefix-$y_var.plot
set terminal png size 1024,800
set output 'output-$prefix-$y_var.png'
set title "$title (averaged over $nruns runs)"
set xlabel "$xlabel"
set ylabel "$ylabel"
plot \\
EOF
  for file in data-$prefix-$y_var-*
  do
    echo Generating graph for $file ...
    sort -n $file -o $file
    #echo "'$file' with linespoints title '$file',\\" >> graph.plot
    echo "'$file' using 2:xticlabels(1) with linespoints title '$file',\\" >> plot-$prefix-$y_var.plot
  done
  gnuplot plot-$prefix-$y_var.plot
}

display_graph ()
{
  sxiv output-$prefix-$y_var.png &
}

process_files ()
{
  FILES=`ls -1 | grep "results-$prefix-[^-]*$filter_outer$filter_inner"`
  NFILES=`echo $FILES | wc -w | tr -d " "`
  for file in $FILES
  do
    echo Processing $file ...
    process_file $file
  done
}

clean ()
{
  rm -f results-* data-* stat-* *.plot *.png
}

# Call this with the following variables set appropriately:
# prefix: the name of the test (see process_file for what the prefix
#         represents)
# y_var: the statistic to be plotted (see extract_stats for valid
#           statistics)
# xlabel: x-axis label
# ylabel: y-axis label
# title: graph title
process_test ()
{
  process_files
if [ $NFILES -ne 0 ]
then
  generate_graph
  display_graph
fi
}

process_tests ()
{
  filter_inner=
  sz=500000
  filter_outer=-$sz
  prefix=buflen_test
  y_var=avg_ccalls
  x_var=BUFLEN
  xlabel="buffer size"
  ylabel="total time (seconds)"
  title="compression time for file of size $sz with varying buffer size"
  process_test
  
  filter_inner=
  sz=65536
  filter_outer=-$sz
  prefix=sb_create_test
  y_var=avg_time
  x_var=x_inner
  xlabel="total number of files of size $sz bytes"
  ylabel="total time (seconds)"
  title="compression time for files consecutively"
  process_test
  
  x_var=x_outer
  filter_inner=
  filter_outer=
  prefix=compress_time_test
  y_var=avg_time
  xlabel="bytes each of /dev/random, /dev/zero, b64encode /dev/random"
  ylabel="total time (seconds)"
  title="compression time for 3 files"
  process_test

  y_var=avg_ccalls
  ylabel="number of CCalls"
  title="number of CCalls for 3 files compressed consecutively"
  process_test
  
  y_var=avg_cheri_sandboxes
  ylabel="number of CHERI sandboxes"
  title="number of CHERI sandboxes for 3 files compressed consecutively"
  process_test
  
  y_var=avg_capsicum_host_rpcs
  ylabel="number of Capsicum host RPCs"
  title="number of Capsicum host RPCs for 3 files compressed consecutively"
  process_test
}

all ()
{
  clean
  tar xf results.tar
  process_tests
}

$@

