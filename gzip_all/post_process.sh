# ministat-format dependent:
avg_field=6

extract_avg ()
{
  ministat -n $file > ${file}_stat
  #avg_field=`cat ${file}_stat | awk '{for(i=0; i<NF; i++) if ($i=="Avg")print i;}'`
  #tail -n 1 ${file}_stat | awk '{for (i=0; i<NF; i++) if ($i==$avg_field) print $i;}' avg_field=$avg_field 
  avg_value=`tail -n 1 ${file}_stat | awk '{for (i=1; i<=NF; i++) if ($i==$avg_field) print $i;}' avg_field=$avg_field`
}

append_curve ()
{
  echo $bytes $avg_value >> graph-$impl
}

process_file ()
{
  # filename is of the form results-<implementation>-<number of bytes>
  impl=`echo $file | awk -F"-" '{print $2}'`
  bytes=`echo $file | awk -F"-" '{print $3}'`
  if [ $bytes -lt $minbytes ] ; then
    minbytes=$bytes
  fi
  if [ $bytes -gt $maxbytes ] ; then
    maxbytes=$bytes
  fi
  extract_avg
  append_curve
}

generate_graph ()
{
  cat <<EOF >graph.plot
set terminal png size 640,480
set output 'tmp.png'
set title "compression time for 3 files (averaged over 3 runs)"
set xlabel "bytes each of /dev/random, /dev/zero, b64encode /dev/random"
set ylabel "total seconds"
plot \\
EOF
  for file in graph-*
  do
    echo Generating graph for $file ...
    sort -n $file -o $file
    #echo "'$file' with linespoints title '$file',\\" >> graph.plot
    echo "'$file' using 2:xticlabels(1) with linespoints title '$file',\\" >> graph.plot
  done
  gnuplot graph.plot
}

display_graph ()
{
  sxiv tmp.png
}

process_files ()
{
  for file in results-*
  do
    echo Processing $file ...
    process_file $file
  done
}

minbytes=9999999999999
maxbytes=0
rm -f results-*_stat graph-* graph.plot tmp.png
tar xf results.tar
process_files
generate_graph
display_graph
