#!/bin/bash

source tests/regression/common.sh

check_env
setup_build_opts

# Get command line arguments
# Current arguments parsed are <sleep_time> <remaining_args>
get_cli_args $@

# Install prerequisites
make MQTT
pip3 install --user -r tests/regression/requirements.txt

# FIXME: Check Redis status
redis-server &

# Iterate over all available build options
for (( i = 0; i < ${#OPTIONS[@]}; i++ )); do
    option=${OPTIONS[${i}]}
    cli_arg=${option} | cut -d '|' -f 1
    build_arg=${option} | cut -d '|' -f 2

    trap 'TA_INIT=1' USR1
    bazel run accelerator --define mqtt=enable ${build_arg} -- --quiet --ta_port=${TA_PORT} ${cli_arg} &
    TA=$!
    
    TA_INIT=0
    while [ "$TA_INIT" -ne 1 ]; do
        echo "waiting for tangle-accelerator initialization"
        sleep 1
    done
    
    trap "kill -9 ${TA};" INT # Trap SIGINT from Ctrl-C to stop TA

    python3 tests/regression/runner.py ${remaining_args} --url "localhost" --mqtt
    rc=$?

    if [ $rc -ne 0 ]
    then
        echo "Build option '${option}' failed"
        fail+=("${option}")
    else
        success+=("${option}")
    fi

    bazel clean
    wait $(kill -9 ${TA})
done

echo "--------- Successful build options ---------"
for (( i = 0; i < ${#success[@]}; i++ )); do echo ${success[${i}]}; done
echo "----------- Failed build options -----------"
for (( i = 0; i < ${#fail[@]}; i++ )); do echo ${fail[${i}]}; done

if [ ${#fail[@]} -gt 0 ]; then
    exit 1
fi
