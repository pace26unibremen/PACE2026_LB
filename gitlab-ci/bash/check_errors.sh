#!/bin/bash
# check_errors.sh - Check for solver errors after a benchmark run
# Usage: check_errors.sh <local_stride_logs_dir>
#   local_stride_logs_dir: path to the rsync'd stride-logs directory for this job
#                          (contains exactly one run subdirectory + a 'latest' symlink)

LOCAL_LOGS="${1:?Usage: check_errors.sh <local_stride_logs_dir>}"

# Find the single run directory (follow the latest symlink if present,
# otherwise just pick the only subdirectory).
if [ -L "${LOCAL_LOGS}/latest" ]; then
    RUN_DIR_NAME=$(basename "$(readlink -f "${LOCAL_LOGS}/latest")")
else
    # Fallback: only one real subdir should exist
    RUN_DIR_NAME=$(find "$LOCAL_LOGS" -mindepth 1 -maxdepth 1 -type d | head -n1 | xargs basename)
fi

if [ -z "$RUN_DIR_NAME" ]; then
    echo "Warning: Could not determine run directory for error checking"
    exit 1
fi

LOCAL_ERROR_DIR="${LOCAL_LOGS}/${RUN_DIR_NAME}/solvererror"

if [ -d "$LOCAL_ERROR_DIR" ] && [ -n "$(ls -A "$LOCAL_ERROR_DIR")" ]; then
    echo "Error: solvererror non-empty in $RUN_DIR_NAME"
    exit 1
fi

echo "No solver errors found in $RUN_DIR_NAME"