#!/bin/sh
read -p "input ip 1 byte: "  first
read -p "input ip 2 byte: "  second
read -p "input ip 3 byte: "  third
read -p "input start 4 byte: "  fourStart
read -p "input stop 4 byte: "  fourStop
  echo cat result.log for result
  echo $(date +%Y-%m-%d_%H:%M:%S)
  echo $(date +%Y-%m-%d_%H:%M:%S) > result.log
while [ $fourStart -le $fourStop ]
do
	
  result=
	result=$(curl $first.$second.$third.$fourStart:8888 -s --connect-timeout 0.5)
   if [ ! "$result" ]; then  
    echo $first.$second.$third.$fourStart:8888 NULL 
  else  
    echo $first.$second.$third.$fourStart:8888 $result
    echo $first.$second.$third.$fourStart:8888 $result >> result.log
  fi  
	
	fourStart=`expr $fourStart + 1`
done
