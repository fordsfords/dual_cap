listen_port=9877
mon_file=msend.log
mon_pattern=ERROR
cap_cmd=tshark -i lo -w capdir1/cap.pcapng -b filesize:100 -b files:3 -q
cap_linger_ms=500
