set terminal png
set output 'results.png'
set xrange [0.0:100.0]
set xlabel "Time(in seconds)"
set autoscale
set yrange [0:2000]
set ylabel "Throughput(in Kbps)"
set grid
set style data lines
plot "rdfinal.txt" using 1:2 title "UDP RD Throughput" lt rgb "red" , "sdfinal.txt" using 1:2 title "UDP SD Throughput" lt rgb "blue"

