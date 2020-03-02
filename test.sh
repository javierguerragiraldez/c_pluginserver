#!/bin/bash

if [ "$1" == "-v" ]; then
	VERBOSE='true'
fi
if [ "$1" == "-vv" ]; then
	VERBOSE='very'
fi

pkill c-pluginserver
rq --help >/dev/null

echo "pwd: $PWD"

SOCKET='c_pluginserver.sock'

if pgrep c-pluginserver -l; then
	PREVIOUS_SERVER="yes"
else
	echo "starting server..."
	[ -S "$SOCKET" ] && rm "$SOCKET"
	./c-pluginserver -s ./$SOCKET &
	pgrep c-pluginserver -l
	sleep 0.1s
fi

msg() {
	query="$1"
	[ -v VERBOSE ] && rq <<< "$query"
	[ "$VERBOSE" == "very" ] && rq <<< "$query" -M | hd
	METHOD="$(rq <<< "$query" -- 'at([2])')"
	response="$(rq <<< "$query" -M | ncat -U "$SOCKET" | rq -m | jq 'select(.[0]==1)' )"
	[ -v VERBOSE ] && rq <<< "$response"

	ERROR="$(jq <<< "$response" '.[2]')"
	RESULT="$(jq <<< "$response" '.[3]')"
#	echo $RESULT
}

assert_noerr() {
	if [ "$ERROR" != "null" ]; then
		echo "query: $query"
		echo "response: $response"
		echo "$METHOD : $ERROR" > /dev/stderr
		exit 1
	fi
	echo "$METHOD: ok"
}

assert_fld_match() {
	fld="$1"
	pattern="$2"

	fld_v="$(query_result '.'$fld'')"
	if [[ "$fld_v" =~ "$pattern" ]]; then
		echo "==> $fld_v : ok"
	else
		echo "==> $fld_v : no match '$pattern'"
		exit 1
	fi
}

query_result() {
	jq <<< "$RESULT" "$1"
}

msg '[0, 19, "plugin.GetPluginInfo", ["c-hello"]]'
assert_noerr

if [ ! -v PREVIOUS_SERVER ]; then
	pkill c-pluginserver
	rm "$SOCKET"
fi

