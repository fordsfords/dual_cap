#!/bin/bash
# tst.sh - Sunny-day test for dual_cap.

start_caps() {
  ./dual_cap listener.cfg &
  LISTENER_PID=$!
  sleep 0.5  # Let listener bind and accept.

  ./dual_cap initiator.cfg &
  INITIATOR_PID=$!
  sleep 2

  # Make sure both programs are still running.

  if kill -0 $LISTENER_PID 2>/dev/null; then :
  else :
    echo "ERROR: listener not running."
    FAIL=1
  fi

  if kill -0 $INITIATOR_PID 2>/dev/null; then :
  else :
    echo "ERROR: initiator not running."
    FAIL=1
  fi
}  # start_caps

check_exits() {
  if kill -0 $LISTENER_PID 2>/dev/null; then
    echo "ERROR: Listener still running."
    kill $LISTENER_PID 2>/dev/null
    FAIL=1
  else
    wait $LISTENER_PID
    RC=$?
    if [ $RC -ne 0 ]; then
      echo "FAIL: listener exited with status $RC"
      FAIL=1
    fi
  fi

  if kill -0 $INITIATOR_PID 2>/dev/null; then
    echo "ERROR: Initiator still running."
    kill $INITIATOR_PID 2>/dev/null
    FAIL=1
  else
    wait $INITIATOR_PID
    RC=$?
    if [ $RC -ne 0 ]; then
      echo "FAIL: Initiator exited with status $RC"
      FAIL=1
    fi
  fi
}  # check_exits

./bld.sh;  if [ "$?" -ne 0 ]; then exit; fi

rm -f logfile1.log logfile2.log listener.cfg initiator.cfg

cat >listener.cfg <<__EOF__
listen_port=9877
mon_file=logfile1.log
__EOF__

cat >initiator.cfg <<__EOF__
init_ip=127.0.0.1
init_port=9877
mon_file=logfile2.log
__EOF__

# Start with non-empty log files.
cat listener.cfg > logfile2.log
cat initiator.cfg > logfile1.log

FAIL=0

# First test - verify listener can trigger.

start_caps

# Triggering listener.

echo "test" >> logfile1.log

sleep 0.5

# Make sure both programs exited.

check_exits

# Second test - verify initiator can trigger.

start_caps

# Triggering listener.

echo "test" >> logfile2.log

sleep 0.5

# Make sure both programs exited.

check_exits

# Third test - verify cap_cmd starts and stops.

if which tshark >/dev/null 2>&1; then
  rm -rf capdir1 capdir2
  mkdir -p capdir1 capdir2
  chmod 777 capdir1 capdir2

  cat >listener.cfg <<__EOF__
listen_port=9877
mon_file=logfile1.log
cap_cmd=tshark -i lo -w capdir1/cap.pcapng -b filesize:100 -b files:3 -q
cap_linger_ms=500
__EOF__

  cat >initiator.cfg <<__EOF__
init_ip=127.0.0.1
init_port=9877
mon_file=logfile2.log
cap_cmd=tshark -i lo -w capdir2/cap.pcapng -b filesize:100 -b files:3 -q
cap_linger_ms=500
__EOF__

  start_caps

  # Triggering listener.

  echo "test" >> logfile1.log

  sleep 1

  check_exits

  # Verify capture files were created.
  CAP1_COUNT=$(ls capdir1/cap*.pcapng 2>/dev/null | wc -l)
  CAP2_COUNT=$(ls capdir2/cap*.pcapng 2>/dev/null | wc -l)

  if [ "$CAP1_COUNT" -eq 0 ]; then
    echo "FAIL: No capture files in capdir1."
    FAIL=1
  fi

  if [ "$CAP2_COUNT" -eq 0 ]; then
    echo "FAIL: No capture files in capdir2."
    FAIL=1
  fi

  # Verify tshark processes are gone.
  sleep 0.5
  if pgrep -f "capdir1/cap.pcapng" >/dev/null 2>&1; then
    echo "FAIL: tshark for capdir1 still running."
    pkill -f "capdir1/cap.pcapng" 2>/dev/null
    FAIL=1
  fi

  if pgrep -f "capdir2/cap.pcapng" >/dev/null 2>&1; then
    echo "FAIL: tshark for capdir2 still running."
    pkill -f "capdir2/cap.pcapng" 2>/dev/null
    FAIL=1
  fi
else
  echo "FYI: tshark not found; skipping cap_cmd test."
fi

# Fourth test - mon_pattern: non-matching lines should not trigger.

cat >listener.cfg <<__EOF__
listen_port=9877
mon_file=logfile1.log
mon_pattern=*ERROR*
__EOF__

cat >initiator.cfg <<__EOF__
init_ip=127.0.0.1
init_port=9877
mon_file=logfile2.log
mon_pattern=*ERROR*
__EOF__

start_caps

# Write a non-matching line.  Should NOT trigger.
echo "INFO: all is well" >> logfile1.log

sleep 1

# Both should still be running.
if kill -0 $LISTENER_PID 2>/dev/null; then :
else
  echo "FAIL: listener triggered on non-matching line."
  FAIL=1
fi

if kill -0 $INITIATOR_PID 2>/dev/null; then :
else
  echo "FAIL: initiator triggered on non-matching line."
  FAIL=1
fi

# Now write a matching line.  Should trigger.
echo "ERROR: something broke" >> logfile1.log

sleep 0.5

check_exits

# Fifth test - mon_pattern: trigger on initiator side.

start_caps

echo "INFO: nothing to see" >> logfile2.log
sleep 1

if kill -0 $LISTENER_PID 2>/dev/null; then :
else
  echo "FAIL: listener triggered on non-matching line (test 5)."
  FAIL=1
fi

echo "ERROR: initiator saw it" >> logfile2.log

sleep 0.5

check_exits

exit $FAIL
