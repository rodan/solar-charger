#!/bin/bash

# script that parses the output provided by the charge controller and imports it into an sqlite database
#
#
# Usage:
# cat $file | parse.sh

db="pv_charge.db"

[ ! -e "${db}" ] &&
        echo 'create table sensors (time DATE PRIMARY KEY UNIQUE, t_int REAL, t_heatsink REAL, v_bat REAL, v_pv REAL, i_ch REAL, stat INTEGER);' | sqlite3 "${db}"

cat | while read line; do
    date=$(echo "${line}" | awk '{ print $1 " " $2 }'| sed 's|\([0-9]\{4\}\)\([0-9]\{2\}\)\([0-9]\{2\}\)\s\(.*\)|\1-\2-\3T\4|')
    t_int=$(echo "${line}" | awk '{ print $3 }')
    t_heatsink=$(echo "${line}" | awk '{ print $4 }')
    v_bat=$(echo "${line}" | awk '{ print $5 }')
    v_pv=$(echo "${line}" | awk '{ print $6 }')
    i_ch=$(echo "${line}" | awk '{ print $7 }')
    stat=$(echo "${line}" | sed 's|\r||' | awk '{ print $8 }' | sed 's|0x||')
    stat=$(echo "ibase=16;obase=A;${stat}" | bc)

    echo "insert into sensors values (\"${date}\", \"${t_int}\", \"${t_heatsink}\", \"${v_bat}\", \"${v_pv}\", \"${i_ch}\", \"${stat}\");" | sqlite3 "${db}"

done


