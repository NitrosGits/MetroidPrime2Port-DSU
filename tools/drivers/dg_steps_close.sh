# Steps for dg_area53.sh: "CLOS" to the body (sets 0x128C bit 0x08 and requests its deletion).
sleep 5
echo "PRE $(grep DGA138 $LOGF | sort | uniq -c | tr '\n' ' ')"
C "send $BODY 0x434c4f53"
sleep 5
C "objs Guardian"
echo "POST $(grep DGA138 $LOGF | sort | uniq -c | tr '\n' ' ')"
