set terminal pdf
set output 'RR.pdf'
set title 'Receive Rate'
set xlabel 'Simulation Time (Seconds)'
set ylabel 'Receive Rate'
plot 'AODV.csv' using 1:2 with linespoints title 'AODV', 'OLSR.csv' using 1:2 with linespoints title 'OLSR','DSDV.csv' using 1:2 with linespoints title 'DSDV','DSR.csv' using 1:2 with linespoints title 'DSR' 

set terminal pdf
set output 'PR.pdf'
set title 'Packets Received'
set xlabel 'Simulation Time (Seconds)'
set ylabel 'Packets Received'
plot 'AODV.csv' using 1:3 with linespoints title 'AODV', 'OLSR.csv' using 1:3 with linespoints title 'OLSR','DSDV.csv' using 1:3 with linespoints title 'DSDV','DSR.csv' using 1:3 with linespoints title 'DSR' 

set terminal pdf
set output 'macphy.pdf'
set title 'Mac Phy Overhead'
set xlabel 'Simulation Time (Seconds)'
set ylabel 'Overhead'
plot 'AODV.csv' using 1:22 with linespoints title 'AODV', 'OLSR.csv' using 1:22 with linespoints title 'OLSR','DSDV.csv' using 1:22 with linespoints title 'DSDV','DSR.csv' using 1:22 with linespoints title 'DSR'
