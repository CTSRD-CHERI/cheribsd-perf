extract_avg ()
{
  # ministat-format dependent:
  avg_field=6
  avg_value=`tail -n 1 ${stat_file} | awk '{print $avg_field;}' avg_field=$avg_field`
}

extract_stats ()
{
  # find average time taken
  stat_file=stat-${filep}_time
  cat $file | sed "s/^\[stat\].*$//g" | ministat -n > $stat_file
  extract_avg
  avg_time=$avg_value

  # find average number of ccalls
  stat_file=stat-${filep}_ccalls
  cat $file | grep "^\[stat\] Number of CCalls" | awk '{print $NF;}' | ministat -n > $stat_file
  extract_avg
  avg_ccalls=$avg_value
  
  # find average number of CHERI sandbox creations
  stat_file=stat-${filep}_cheri_sandboxes
  cat $file | grep "^\[stat\] Number of CHERI sandboxes" | awk '{print $NF;}' | ministat -n > $stat_file
  extract_avg
  avg_cheri_sandboxes=$avg_value
  
  # find average number of Capsicum host RPCs
  stat_file=stat-${filep}_capsicum_host_rpcs
  cat $file | grep "^\[stat\] Number of Capsicum host RPCs" | awk '{print $NF;}' | ministat -n > $stat_file
  extract_avg
  avg_capsicum_host_rpcs=$avg_value
}

append_curve ()
{
  y_ref=\$$y_var
  x_ref=\$$x_var
  echo `eval echo $x_ref` `eval echo $y_ref` >> data-$prefix-$y_var-$impl
}

process_file ()
{
  # filename is of the form results-<prefix>-<implementation>-<number of bytes>
  # any of these might also usefully be an x_var
  impl=`echo $file | sed s/^results-$prefix-/results-/g | awk -F"-" '{print $2}'`
  bytes=`echo $file | sed s/^results-$prefix-/results-/g | awk -F"-" '{print $3}'`
  nfiles=`echo $file | sed s/^results-$prefix-/results-/g | awk -F"-" '{print $4}'`
  filep=`echo $file | sed s/^results-//g`
  if [ $bytes -lt $minbytes ] ; then
    minbytes=$bytes
  fi
  if [ $bytes -gt $maxbytes ] ; then
    maxbytes=$bytes
  fi
  extract_stats
  append_curve
}

generate_graph ()
{
  cat <<EOF >plot-$prefix-$y_var.plot
set terminal png size 1024,800
set output 'output-$prefix-$y_var.png'
set title "$title"
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
  for file in `ls -1 | grep results-$prefix-.*$filter_sz$filter_nfiles`
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
  minbytes=9999999999999
  maxbytes=0
  minfiles=9999999999999
  maxfiles=0
  process_files
  generate_graph
  display_graph
}

process_tests ()
{
  filter_nfiles=
  filter_sz=-64
  prefix=sb_create_test
  y_var=avg_time
  x_var=nfiles
  xlabel="total number of files of size $filter_sz bytes"
  ylabel="total time (seconds)"
  title="compression time for files consecutively (averaged over 3 runs)"
  process_test
  
  x_var=bytes
  filter_nfiles=
  filter_sz=
  prefix=compress_time_test
  y_var=avg_time
  xlabel="bytes each of /dev/random, /dev/zero, b64encode /dev/random"
  ylabel="total time (seconds)"
  title="compression time for 3 files (averaged over 3 runs)"
  process_test

  y_var=avg_ccalls
  ylabel="number of CCalls"
  title="number of CCalls for 3 files compressed consecutively (averaged over 3 runs)"
  process_test
  
  y_var=avg_cheri_sandboxes
  ylabel="number of CHERI sandboxes"
  title="number of CHERI sandboxes for 3 files compressed consecutively (averaged over 3 runs)"
  process_test
  
  y_var=avg_capsicum_host_rpcs
  ylabel="number of Capsicum host RPCs"
  title="number of Capsicum host RPCs for 3 files compressed consecutively (averaged over 3 runs)"
  process_test
}

all ()
{
  clean
  tar xf results.tar
  process_tests
}

$@

