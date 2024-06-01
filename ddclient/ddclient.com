$set def USER1:[MAUSTIN.ddclient]
$now = F$CVTime("","ABSOLUTE","DATETIME")
$n = F$CVTIME("''now'+00:05:00","ABSOLUTE","TIME")
$submit/noprint/after="''n'" ddclient.com -
	/log=USER1:[MAUSTIN.ddclient]ddclient.log
$perl ddclient
$exit
