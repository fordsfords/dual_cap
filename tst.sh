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

exit $FAIL
