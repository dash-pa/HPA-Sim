#!/bin/bash
### BEGIN INIT INFO
# Provides: startup.bash
# Required-Start: $all
# Required-Stop: 
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: so much shit I have to fill in
### END INIT INFO

SCREEN_NAME="ble_wattage_service"
RUNNING_FILE=/tmp/${SCREEN_NAME}_is_running

if [ ! -f $RUNNING_FILE ]; then
    /usr/bin/touch $RUNNING_FILE
    /usr/bin/screen -dmS $SCREEN_NAME /home/pi/startup_screen_cmd.bash $RUNNING_FILE
    /bin/echo Started. as screen $SCREEN_NAME
else
    /bin/echo $RUNNING_FILE exists, so already running.
fi
