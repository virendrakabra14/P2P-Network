make
fld=$1
n=$2
p=$3


for ((i=1 ; i<$n ; i++))
do
	timeout 60 ./client-phase$p ./$fld/client$i-config.txt ./$fld/files/client$i/ > ./$fld/ans\_$i\_$p.txt &
done

timeout 60 ./client-phase$p ./$fld/client$n-config.txt ./$fld/files/client$n/ > ./$fld/ans\_$n\_$p.txt
