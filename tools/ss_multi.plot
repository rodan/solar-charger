
set terminal png truecolor size 700,900
set output 'pv_multi.png' 

set title "sensor data for the last 7 days"

set multiplot

set tmarg 0
set bmarg 0
set lmarg 7
set rmarg 3
set size 1,0.36
set origin 0.0,0.60

set timefmt "%Y-%m-%dT%H:%M"
set datafile separator "|"


set xdata time
#set xlabel 'time'
#set xrange ['2011-11-25T13:01:08':'2011-11-26T13:01:08']
#set xtics 7200
#set format x "%H"
#set format x "%H\n%a"

#set ylabel 'temperature (dC)'
#set ytics nomirror
set ytics 1
#set yrange [-20:40]

set grid
set palette model RGB
set palette defined ( 0 '#00FFFF', 1 '#DC143C' )
set cbrange [-10:10]

plot "< sqlite3 pv_charge.db 'select time, t_int, t_heatsink from sensors order by time desc limit 0, 1200;'" \
    using 1:2 index 0 title "int temperature (dC)" with lines lt rgb '#1111ee' lw 2, \
    "" using 1:3:3 title "heatsink temperature (dC)" with lines palette lw 2

unset title


# voltages

set tmarg 0
set bmarg 0
set lmarg 7
set rmarg 15
set size 1,0.36
set origin 0.0,0.198

#set ylabel 'voltages (V)'
set ytics 2
#set yrange [0:100]

plot "< sqlite3 pv_charge.db 'select time, v_bat, v_pv from sensors order by time desc limit 0, 1200;'" \
    using 1:2 title "battery" with lines lt rgb '#0457ef' lw 1, \
    "" using 1:3 title "photovoltaic cells" with lines lt rgb '#ef7d04' lw 1 #, \
    #"" using 1:4 title "charging" axes x1y2 fs filledcurve y1=4 transparent solid 0.15 lc rgb "gold"
    #lines lt rgb '#ee2211' lw 1


# current

set tmarg 0
set bmarg 0
set lmarg 7
set rmarg 15
set size 1,0.1
set origin 0.0,0.050

#set ylabel 'current (A)'
set ytics .01
set yrange [0:.1]

set y2range [4:16]
set y2tics 100


plot "< sqlite3 pv_charge.db 'select time, i_ch, status from sensors order by time desc limit 0, 1200;'" \
    using 1:2 index 0 title "i_ch (A)" with lines lt rgb '#867088' lw 1, \
    "" using 1:3 title "charge enabled" with lines lt rgb '#ff0000' lw 1

set style fill transparent solid 0.1 noborder
plot "< sqlite3 pv_charge.db 'select time, status from sensors order by time desc limit 0, 1200;'" \
   using 1:2 title "" axes x1y2 with filledcurve y1=4

