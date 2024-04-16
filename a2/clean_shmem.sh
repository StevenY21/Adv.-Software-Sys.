#!/bin/bash

# Check if the script is being run with root privileges
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root or with sudo" >&2
    exit 1
fi

# List all shared memory segments and parse their IDs
echo "Output of ipcs -m:"
ipcs -m
echo "Parsing output to extract shared memory segment IDs..."
shm_ids=$(ipcs -m | grep -E '^[0-9]+' | awk '{print $2}')
echo "Extracted shared memory segment IDs: $shm_ids"

# Check if there are any shared memory segments to delete
if [ -z "$shm_ids" ]; then
    echo "No shared memory segments found"
    exit 0
fi

# Delete each shared memory segment
for shm_id in $shm_ids; do
    if ! ipcrm -m $shm_id &> /dev/null; then
        echo "Failed to delete shared memory segment $shm_id" >&2
    else
        echo "Shared memory segment $shm_id deleted"
    fi
done