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

if pgrep c_pluginserver -l; then
	PREVIOUS_SERVER="yes"
else
	echo "starting server..."
	[ -S "$SOCKET" ] && rm "$SOCKET"
	build/c_pluginserver -s ./$SOCKET &
	pgrep c_pluginserver -l
	sleep 0.1s
fi

killit() {
	if [ ! -v PREVIOUS_SERVER ]; then
		pkill c_pluginserver
		rm "$SOCKET"
	fi
}

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

		killit
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

		killit
		exit 1
	fi
}

query_result() {
	jq <<< "$RESULT" "$1"
}

msg '[0, 19, "plugin.GetPluginInfo", ["c-hello"]]'
assert_noerr

msg '[0, 19, "plugin.StartInstance", [{"Name":"c-hello", "Config":"{\"message\":\"howdy\"}"}]]'
assert_noerr
helloId=$(query_result '."Id"')
echo "helloId: $helloId"

msg '[0, 19, "plugin.HandleEvent", [{"InstanceId": '$helloId', "EventName": "access", "Params": [45, 23]}]]'
assert_noerr
helloEventId=$(query_result '."EventId"')

assert_fld_match 'Data.Method' 'kong.request.set_header'
assert_fld_match 'Data.Args' '["x-hello", "hello from C"]'



killit
