#!/bin/bash
echo "<html>" ;
echo "<head><style> .alert{background-color: red; color: white;} table {border-collapse: collapse;} table, td, th {border: 1px solid black;} </style></head>"
echo "<title> REPORT </title>"
echo "<body><h1> REPORT / $2 </h1>";
header=1
echo "<table border = '1' cellpadding=10>"
IFS=','
while read -r line; do
    echo "<tr>";
    if [[ "${header}" == "1" ]]; then
        echo -n "<th>";
        echo -n "$line" | sed 's/,/\<\/th\>\<th\>/g';
        echo "</th>";
        header=0;
    else
        echo -n "<td>";
        echo -n "$line" | sed 's/ ,/,/g' | sed 's/,\(Command exited with non-zero status [0-9.\s]*\)/\<\/td\>\<td class="alert"\>\1/g' | sed 's/,/\<\/td\>\<td\>/g';
        echo "</td>";
    fi
    echo "</tr>";
done < $1 ;
echo "</table></body></html>";