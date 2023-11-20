#!/bin/bash

declare -r n=30

for url in "1" "2"; do
    rm tmp/test$url.curl.csv;
    rm tmp/test$url.time.csv;
    echo -e "$2" > tmp/test$url.curl.csv;
    echo -e "$2" > tmp/test$url.time.csv;
    for i in `seq 0 $n`; do
        echo -n "$i ";
        (
            timeout 2h curl -w "%{time_total}" http://localhost:$1/$i/$url -o /dev/null --silent ;
        ) | sed ':a;N;$!ba;s/\n/ /g' >> tmp/test$url.curl.csv
        echo "" >> tmp/test$url.curl.csv
        
        echo -n "$i ";
        (
            /usr/bin/time -f "%e" timeout 2h curl http://localhost:$1/$i/$url -o /dev/null --silent 2>&1 ;
        ) | sed ':a;N;$!ba;s/\n/ /g' >> tmp/test$url.time.csv
    done
done

echo "";
