set terminal pdf
set output "RR.pdf"
set title "Receive Rate"
set xlabel "Simulation Time (Seconds)"
set ylabel "Receive Rate"
plot "AODV.csv" using 1:2 with linespoints title "AODV", "OLSR.csv" using 1:2 with linespoints title "OLSR","DSDV.csv" using 1:2 with linespoints title "DSDV","DSR.csv" using 1:2 with linespoints title "DSR"
