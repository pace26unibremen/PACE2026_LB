#!/bin/bash
# monitor_job.sh - Monitor a Slurm job and display progress information
# Usage: monitor_job.sh <job_dir>
#   job_dir: absolute path to the job's staging directory (e.g. ~/pace-bench/jobs/<CI_JOB_ID>)

set -e

JOB_DIR="${1:?Usage: monitor_job.sh <job_dir>}"
cd "$JOB_DIR"

JOB_ID=$(cat last_job.id)

# Variables to track the specific run directory for this job
RUN_DIR=""
RUN_DIR_FOUND=false

# Get the Stride Timeout of the .env file
STRIDE_TIMEOUT=300  # fallback default
if [ -f "${JOB_DIR}/.env" ]; then
    STRIDE_TIMEOUT=$(grep -m1 '^STRIDE_TIMEOUT=' "${JOB_DIR}/.env" | cut -d= -f2 | tr -d '"')
    STRIDE_TIMEOUT=${STRIDE_TIMEOUT:-300}
fi

# Main polling loop - continuously check job state until completion or failure
while true; do
  # Query Slurm for the job state
  STATE=$(sacct -j "$JOB_ID" --format=State --noheader | head -n1 | awk '{print $1}')

  # Job completed successfully
  if [[ "$STATE" =~ ^COMPLETED ]]; then
    echo ""
    echo "Job $JOB_ID completed successfully"
    exit 0

  # Job ended with a Timeout, which is kind of expected with the huge test sets we use
  elif [[ "$STATE" =~ ^TIMEOUT ]]; then
    echo ""
    echo "Job $JOB_ID cancelled by a TIMEOUT, this is expected, as the test set is really big"
    exit 0

  # Job ended with an error state
  elif [[ "$STATE" =~ ^(FAILED|CANCELLED|NODE_FAIL|PREEMPTED|OUT_OF_MEMORY) ]]; then
    echo ""
    echo "Slurm job $JOB_ID ended with state $STATE"
    exit 1

  # Job is waiting in queue
  elif [[ "$STATE" =~ ^(PENDING|CONFIGURING) ]]; then
    # Try to show why the job is waiting (e.g., resources, priority)
    REASON=$(squeue -j "$JOB_ID" -h -o "%r" 2>/dev/null || echo "Unknown")
    echo "Job $JOB_ID still in state $STATE (reason: $REASON) - waiting..."
    sleep 60

  # Job is running - show detailed progress
  elif [[ "$STATE" =~ ^RUNNING ]]; then
    OUTPUT="Job $JOB_ID is running..."

    # Detect run directory from latest symlink (only once).
    # Since this is a per-job directory, latest always belongs to us.
    if [ "$RUN_DIR_FOUND" = false ]; then
        if [ -L "stride-logs/latest" ]; then
            CURRENT_LATEST=$(readlink -f "stride-logs/latest")
            if [ -n "$CURRENT_LATEST" ] && [ -d "$CURRENT_LATEST" ]; then
                RUN_DIR="$CURRENT_LATEST"
                RUN_DIR_FOUND=true
                echo ""
                echo "Detected run directory: $RUN_DIR"
            fi
        fi
    fi

    # Display progress information if we've found the run directory
    if [ "$RUN_DIR_FOUND" = true ]; then
      SUMMARY_FILE="${RUN_DIR}/summary.json"
      INSTANCES_FILE="${JOB_DIR}/instances.lst"

      # Check if stride has started producing output
      if [ -f "$SUMMARY_FILE" ] && [ -f "$INSTANCES_FILE" ]; then
        # Count total instances to process
        TOTAL=$(wc -l < "$INSTANCES_FILE" 2>/dev/null | tr -d ' \n' | head -c 10)
        TOTAL=${TOTAL:-0}

        # Parse summary.json for progress statistics
        TIMEOUTS=$(grep -c '"Timeout"' "$SUMMARY_FILE" 2>/dev/null || true)
        ERRORS=$(grep -c '"SolverError"' "$SUMMARY_FILE" 2>/dev/null || true)
        SOLVED=$(grep -c '"Valid"' "$SUMMARY_FILE" 2>/dev/null || true)
        FINISHED=$((TIMEOUTS + ERRORS + SOLVED))

        # Extract trees/leaves from LAST line (grep ^{ grabs JSON objects)
        LAST_LINE=$(grep '^{' "$SUMMARY_FILE" | tail -n1)
        NUM_TREES=$(echo "$LAST_LINE" | grep -o '"s_num_trees":[0-9]*' | sed 's/.*://' | tr -d ' \n' || true)
        NUM_LEAVES=$(echo "$LAST_LINE" | grep -o '"s_num_leaves":[0-9]*' | sed 's/.*://' | tr -d ' \n' || true)

        # Validate that TOTAL is a valid number before arithmetic
        if [[ "$TOTAL" =~ ^[0-9]+$ ]] && [ "$TOTAL" -gt 0 ]; then
          PERCENT=$((FINISHED * 100 / TOTAL))
          OUTPUT="${OUTPUT} | Progress: ${FINISHED}/${TOTAL} (${PERCENT}%) | Solved: ${SOLVED} | Timeouts: ${TIMEOUTS} | Errors: ${ERRORS} | Last Instance Trees: ${NUM_TREES} | Leaves: ${NUM_LEAVES}"

          # Estimate time remaining using s_wtime from the last WINDOW entries in summary.json
          if [ "$FINISHED" -gt 5 ]; then
              WINDOW=200
              PARALLEL=32  # must match STRIDE_PARALLEL in .env

              # Sum s_wtime or timeout duration over the last $WINDOW lines using awk
              AVG_WTIME=$(tail -n "$WINDOW" "$SUMMARY_FILE" \
                  | awk -v timeout="$STRIDE_TIMEOUT" -v parallel="$PARALLEL" -F'"s_wtime":' '
                      NF > 1 {
                          # Has s_wtime field — use actual measured time
                          split($2, a, /[,}]/);
                          sum += a[1];
                          count++
                      }
                      NF == 1 {
                          # No s_wtime field — this is a Timeout, costs STRIDE_TIMEOUT seconds
                          sum += timeout;
                          count++
                      }
                      END {
                          if (count > 0) printf "%.6f", sum / count / parallel
                          else print "0"
                      }')

              # Convert float seconds to integer centiseconds for bash arithmetic
              REMAINING=$((TOTAL - FINISHED))
              ETA_SECONDS=$(awk "BEGIN {printf \"%d\", $AVG_WTIME * $REMAINING}")

              if [ "$ETA_SECONDS" -gt 0 ]; then
                  ETA_HOURS=$((ETA_SECONDS / 3600))
                  ETA_MINS=$(((ETA_SECONDS % 3600) / 60))

                  # Also show elapsed from Slurm for context
                  ELAPSED=$(sacct -j "$JOB_ID" --format=ElapsedRaw --noheader 2>/dev/null \
                      | head -n1 | awk '{print $1}')
                  ELAPSED=${ELAPSED:-0}
                  ELAPSED_HOURS=$((ELAPSED / 3600))
                  ELAPSED_MINS=$(((ELAPSED % 3600) / 60))

                  OUTPUT="${OUTPUT} | Elapsed: ${ELAPSED_HOURS}h ${ELAPSED_MINS}m | ETA: ${ETA_HOURS}h ${ETA_MINS}m (window: last ${WINDOW})"
              fi
          fi
        fi
        # Add cpu count and memory usage if available
        CPU=$(sacct -j "$JOB_ID" --format=AllocCPUS --noheader 2>/dev/null | head -n1 | tr -d ' \n')
        MEM=$(sacct -j "$JOB_ID" --format=AllocTRES%50 --noheader 2>/dev/null | head -n1 | grep -oP 'mem=\K[^,]+')
        if [ -n "$CPU" ] && [ -n "$MEM" ]; then
          OUTPUT="${OUTPUT} | Allocated Resources: ${CPU} CPUs, ${MEM} Memory"
        fi
      else
        # Files don't exist yet - stride is still initializing
        OUTPUT="$OUTPUT | Waiting for output files..."
      fi
    else
      # Still waiting for stride to create the run directory
      OUTPUT="$OUTPUT | Waiting for run directory..."
    fi

    # Output progress on a single line
    echo "$OUTPUT"
    sleep 60

  # Unknown state - shouldn't happen, but handle it gracefully
  else
    echo "Unknown state: $STATE - waiting..."
    sleep 60
  fi
done
