addf ()
{
  eval $1=`awk -v a=$2 -v b=$3 'BEGIN{print a+b;}'`
}

extract_avg ()
{
  if [ $nruns -lt 3 ]
  then
    avg_value=`cat $stat_file`
    stdev_value=0
  else
    # ministat-format dependent:
    avg_field=6
    avg_value=`tail -n 1 $stat_file | awk '{print $avg_field;}' avg_field=$avg_field`
    stdev_field=7
    stdev_value=`tail -n 1 $stat_file | awk '{print $stdev_field;}' stdev_field=$stdev_field`
  fi
}

run_ministat ()
{
  if [ $nruns -lt 3 ]
  then
    cat tmp | head -n 1 | awk '{print $1}' > $stat_file
  else
    cat tmp | ministat -n > $stat_file
  fi
}

extract_stats ()
{
  nruns=`cat $file | sed "s/^\[stat\].*$//g" | sed "/^$/d" | wc -l | tr -d " "`

  # find average time taken
  stat_file=stat$CASE-${filep}_time
  cat $file | sed "s/^\[stat\].*$//g" | sed "/^$/d" > tmp
  run_ministat
  extract_avg
  avg_time=$avg_value
  stdev_time=$stdev_value

  # find average number of ccalls
  stat_file=stat$CASE-${filep}_ccalls
  cat $file | grep "^\[stat\] Number of CCalls" | awk '{print $NF;}' > tmp
  run_ministat
  extract_avg
  avg_ccalls=$avg_value
  stdev_ccalls=$stdev_value
  
  # find average number of CHERI sandbox creations
  stat_file=stat$CASE-${filep}_cheri_sandboxes
  cat $file | grep "^\[stat\] Number of CHERI sandboxes" | awk '{print $NF;}' > tmp
  run_ministat
  extract_avg
  avg_cheri_sandboxes=$avg_value
  stdev_cheri_sandboxes=$stdev_value
  
  # find average number of Capsicum host RPCs
  stat_file=stat$CASE-${filep}_capsicum_host_rpcs
  cat $file | grep "^\[stat\] Number of Capsicum host RPCs" | awk '{print $NF;}' > tmp
  run_ministat
  extract_avg
  avg_capsicum_host_rpcs=$avg_value
  stdev_capsicum_host_rpcs=$stdev_value

  addf avg_ccall_plus_rpc $avg_ccalls $avg_capsicum_host_rpcs

  # extract the BUFLEN used
  BUFLEN=`cat $file | grep "^\[stat\] BUFLEN" | head -n 1 | awk '{print $NF;}'`
}

append_curve ()
{
  y_ref=\$$y_var
  x_ref=\$$x_var
  y_stdev_ref=\$`echo $y_var | sed s/^avg_/stdev_/g`
  echo `eval echo $x_ref` `eval echo $y_ref` `eval echo $y_stdev_ref` >> data$CASE-$prefix-$y_var-$impl$filter_outer$filter_inner
}

process_file ()
{
  # filename is of the form results<case>-<prefix>-<implementation>-<outer loop variable>-<inner loop variable>
  # any of these might also usefully be an x_var
  impl=`echo $file | sed s/^results$CASE-$prefix-/results$CASE-/g | awk -F"-" '{print $2}'`
  x_outer=`echo $file | sed s/^results$CASE-$prefix-/results$CASE-/g | awk -F"-" '{print $3}'`
  x_inner=`echo $file | sed s/^results$CASE-$prefix-/results$CASE-/g | awk -F"-" '{print $4}'`
  filep=`echo $file | sed s/^results$CASE-//g`
  extract_stats
  append_curve
}

generate_pgfplots_graph ()
{
  cat <<EOF >pgf$CASE-$prefix-$y_var$filter_outer$filter_inner.tex
\documentclass{article}
\usepackage{pgfplots}
\pgfplotsset{compat=1.5}
\begin{document}
\newcommand{\insertdata}[1]{\addplot+[error bars/.cd, y dir=both, y explicit] table[y error index=2] {#1};}
\begin{figure}
\centering
\caption{Caption here}
\begin{tikzpicture}
\begin{axis}[
  %title=
  xlabel=XLabel here
  ylabel=YLabel here
  xmin=0,
  xmax=20,
  scaled ticks=false,
  axis lines=left,
  legend entries={A,B,C,D,},
  legend style={legend pos=outer north east,},
]
EOF
  FILES=`ls -1 | grep "data$CASE-$prefix-$y_var-[^-]*$filter_outer$filter_inner"`
  for file in $FILES

  do
    echo Generating LaTeX graph for $file ...
    sort -n $file -o $file
    echo "\\insertdata{$file}" >> pgf$CASE-$prefix-$y_var$filter_outer$filter_inner.tex
  done
  cat <<EOF >>pgf$CASE-$prefix-$y_var$filter_outer$filter_inner.tex
\end{axis}
\end{tikzpicture}
\end{figure}
EOF
}

generate_gnuplot_graph ()
{
  PLOT_FILE=plot$CASE-$prefix-$y_var$filter_outer$filter_inner.plot
  cat <<EOF >$PLOT_FILE
set terminal png size 1024,800
set output 'output$CASE-$prefix-$y_var$filter_outer$filter_inner.png'
set title "$title (averaged over $nruns runs)"
set xlabel "$xlabel"
set ylabel "$ylabel"
plot \\
EOF
  FILES=`ls -1 | grep "data$CASE-$prefix-$y_var-[^-]*$filter_outer$filter_inner"`
  for file in $FILES

  do
    echo Generating graph for $file ...
    sort -n $file -o $file
    #echo "'$file' using 2:xticlabels(1) with linespoints title '$file',\\" >> plot$CASE-$prefix-$y_var$filter_outer$filter_inner.plot
    echo "'$file' using 0:2:3:xticlabels(1) with yerrorlines title '$file',\\" >> $PLOT_FILE
  done
# remove trailing ",\"
  fsz=`cat $PLOT_FILE | wc -c | tr -d " "`
  dd if=$PLOT_FILE of=tmp2 bs=`expr $fsz - 3` count=1
  mv tmp2 $PLOT_FILE
  gnuplot $PLOT_FILE
}

display_graph ()
{
  sxiv output$CASE-$prefix-$y_var$filter_outer$filter_inner.png &
}

process_files ()
{
  FILES=`ls -1 | grep "results$CASE-$prefix-[^-]*$filter_outer$filter_inner"`
  NFILES=`echo $FILES | wc -w | tr -d " "`
  for file in $FILES
  do
    echo Processing $file ...
    process_file $file
  done
}

clean ()
{
  rm -f results*-* data*-* stat*-* *.plot *.png *.tex
}

# Call this with the following variables set appropriately:
# prefix: the name of the test (see process_file for what the prefix
#         represents)
# x_var: reference to the variable to plot on the x-axis
# y_var: reference to the statistic to be plotted (see extract_stats
#        for valid statistics)
# xlabel: x-axis label
# ylabel: y-axis label
# title: graph title
process_test ()
{
  process_files
if [ $NFILES -ne 0 ]
then
  generate_gnuplot_graph
  generate_pgfplots_graph
  display_graph
fi
}

process_tests ()
{
  CASE=1
  filter_inner=
  prefix=buflen_test
  y_var=avg_time
  x_var=BUFLEN
  xlabel="buffer size (bytes)"
  ylabel="total time (seconds)"
  for sz in 5000000 10000000
  do
    filter_outer=-$sz
    title="CASE $CASE: buffer size variation: compression time for one file of size $sz"
    process_test
  done
  
  CASE=2
  filter_inner=
  filter_outer=
  prefix=compress_time_test
  y_var=avg_time
  x_var=x_outer
  xlabel="bytes each of /dev/random, /dev/zero, b64encode /dev/random"
  ylabel="total time (seconds)"
  title="CASE $CASE: CCall vs RPC: compression time for 3 files (individually)"
  process_test
  
  y_var=avg_ccall_plus_rpc
  ylabel="number of CCalls + RPCs"
  title="CASE $CASE: CCall vs RPC: number of CCalls + RPCs for 3 files (individually)"
  process_test
  
  CASE=3
  filter_inner=
  prefix=sb_create_test
  y_var=avg_time
  x_var=x_inner
  xlabel="number of files"
  ylabel="total time (seconds)"
  for sz in 4096 65536 500000
  do
    filter_outer=-$sz
    title="CASE $CASE: sandbox creation overhead: compression time for files of size $sz (consecutively)"
    process_test
  done
  
  CASE=4
  filter_inner=
  filter_outer=
  prefix=compress_time_test
  y_var=avg_time
  x_var=x_outer
  xlabel="bytes each of /dev/random, /dev/zero, b64encode /dev/random"
  ylabel="total time (seconds)"
  title="CASE $CASE: capability overhead: compression time for 3 files (individually)"
  process_test
}

process_tests_example ()
{
  filter_inner=
  sz=500000
  filter_outer=-$sz
  prefix=buflen_test
  y_var=avg_time
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

