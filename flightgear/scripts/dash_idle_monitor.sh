#!/bin/bash -x
# GPL V2

FGFS=/usr/games/fgfs
FGFS_PARAMS="--aircraft=dash --enable-fullscreen --fov=80 --enable-hud --heading=85 --vc=70 --lat=51.186869 --lon=-1.043459 --timeofday=dawn --disable-real-weather-fetch --enable-clouds3d"

VIDEO_PLAYER=/usr/bin/vlc
# -R == repeat
# VIDEO_PARAMS="-R --fullscreen"
VIDEO_PARAMS="-R"
IDLE_TIME_VIDEO='/home/kiosk/Videos/IDLE_VIDEO'

# all times in seconds
let PILOT_TIMEOUT=300
let IDLE_MS=30*1000

run_fgfs()
{
	if jobs 1 > /dev/null 2>&1 ; then
		return
	fi
	killall "$FGFS"
	sleep 1

	${FGFS} ${FGFS_PARAMS} &
	FG_START=${SECONDS}
}

pause_fgfs()
{
	# shift-w == W == toggle full screen mode
	xdotool key 50 25

 	# 33 == 'p' == pause/unpause (toggle)
	xdotool key 33

	# If toggling full screen mode doesn't allow video play to
	# to get full screen, kill FGFS and let the main loop restart fgfs.
	# comment out the above lines and everything in unpause_fgfs() below.
	# killall "$FGFS"
}

unpause_fgfs()
{
	# go back to start location
	xdotool key 50 9

	# same keypress to unpause
	xdotool key 33

	# shift-w == W == toggle full screen mode
	xdotool key 50 25

	FG_START=${SECONDS}
}

play_video()
{
	pause_fgfs
	sleep 1

	${VIDEO_PLAYER} ${VIDEO_PARAMS} "${IDLE_TIME_VIDEO}" &

	# xprintidle is too sensitive: _any_ input will trip it. :(
	let key=""
	while [ -z "$key" ] ; do
		read -n 1 key
	done

	killall "${VIDEO_PLAYER}"
	sleep 1
	unpause_fgfs

	notify-send 'SUMPAC Video' 'Video interrupted by new pilot - Prepare to fly!'
}

while :
do
	run_fgfs

	let IDLE=$(xprintidle)
	if [ $? -ne 0 ] ; then
		sleep 1
		let IDLE=$(xprintidle)
		if [ $? -ne 0 ] ; then
			# couldn't contact X server :( (session ended?)
			killall  "$FGFS"
			break
		fi
	fi

	if [ $IDLE -gt $IDLE_MS ] ; then
		play_video
		continue
	fi

	let FG_RUNNING=${SECONDS}-${FG_START}

	if [ $FG_RUNNING -gt $PILOT_TIMEOUT ] ; then
		notify-send 'End of Session' 'Hope you enjoyed your session! Please allow the next pilot a shot at flying. We apologize for the inconvenience.'
		sleep 5

		# go back to start location
		xdotool key 50 9
		FG_START=${SECONDS}
		continue
	fi

	sleep 5
done
