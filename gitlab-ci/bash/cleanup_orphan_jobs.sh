#!/bin/bash
# cleanup_orphan_jobs.sh
# Cronjob script to detect and merge orphaned CI job directories.
#
# A job directory is considered orphaned when:
#   - Its Slurm job is no longer running (COMPLETED, TIMEOUT, FAILED, etc.)
#   - It still exists (the CI job that should have cleaned it up must have died)
#
# Install as a cronjob, e.g. every 30 minutes:
#   */30 * * * * /home/pace26/pace-bench/cleanup_orphan_jobs.sh >> /home/pace26/pace-bench/shared/slurm-logs/cleanup.log 2>&1
# Only keep the last 1000 lines of the logs
# 0 4 * * * tail -n 1000 /home/pace26/pace-bench/shared/slurm-logs/cleanup.log > /tmp/cleanup.log.tmp && mv /tmp/cleanup.log.tmp /home/pace26/pace-bench/shared/slurm-logs/cleanup.log


set -e

JOBS_DIR="$HOME/pace-bench/jobs"
SHARED_DIR="$HOME/pace-bench/shared"

# How long to wait before merging after detecting a finished job,
# to give any still-running CI job a chance to clean up itself.
GRACE_SECONDS=120

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*"
}

if [ ! -d "$JOBS_DIR" ]; then
    log "No jobs directory found at $JOBS_DIR — nothing to do."
    exit 0
fi

# Iterate over every subdirectory in the jobs dir (each is one CI job ID)
for JOB_DIR in "$JOBS_DIR"/*/; do
    [ -d "$JOB_DIR" ] || continue

    CI_JOB_ID=$(basename "$JOB_DIR")

    # Read the Slurm job ID submitted by this CI job
    if [ ! -f "$JOB_DIR/last_job.id" ]; then
        log "[$CI_JOB_ID] No last_job.id found — skipping (job may still be setting up)"
        continue
    fi

    SLURM_JOB_ID=$(cat "$JOB_DIR/last_job.id")

    if [ -z "$SLURM_JOB_ID" ]; then
        log "[$CI_JOB_ID] last_job.id is empty — skipping"
        continue
    fi

    # Check current Slurm state
    STATE=$(sacct -j "$SLURM_JOB_ID" --format=State --noheader 2>/dev/null | head -n1 | awk '{print $1}')
    if [ -z "$STATE" ]; then
        STATE=$(squeue -j "$SLURM_JOB_ID" -h -o "%T" 2>/dev/null || echo "")
    fi

    # If the job is still pending or running, the CI job may still be alive — skip
    if [[ "$STATE" =~ ^(RUNNING|PENDING|CONFIGURING) ]]; then
        log "[$CI_JOB_ID] Slurm job $SLURM_JOB_ID is still in state $STATE — skipping"
        continue
    fi

    # Job is finished (COMPLETED, TIMEOUT, FAILED, CANCELLED, etc.)
    # Wait briefly to give a live CI job the chance to clean up itself first
    log "[$CI_JOB_ID] Slurm job $SLURM_JOB_ID finished with state '${STATE:-unknown}' — waiting ${GRACE_SECONDS}s before merging..."
    sleep "$GRACE_SECONDS"

    # Re-check: if the directory was cleaned up by the CI job during the grace period, we're done
    if [ ! -d "$JOB_DIR" ]; then
        log "[$CI_JOB_ID] Directory was cleaned up during grace period — nothing to do"
        continue
    fi

    log "[$CI_JOB_ID] Directory still exists after grace period — merging as orphan"

    # Merge stride logs into shared directory
    if [ -d "${JOB_DIR}stride-logs" ]; then
        mkdir -p "$SHARED_DIR/stride-logs"
        cp -a "${JOB_DIR}stride-logs/." "$SHARED_DIR/stride-logs/"
        log "[$CI_JOB_ID] Merged stride-logs"
    else
        log "[$CI_JOB_ID] No stride-logs directory found (job may have failed before producing output)"
    fi

    # Move Slurm stdout/stderr logs (%x_%j.out / %x_%j.err)
    mkdir -p "$SHARED_DIR/slurm-logs"
    if ls "${JOB_DIR}"*.out "${JOB_DIR}"*.err 2>/dev/null | grep -q .; then
        mv "${JOB_DIR}"*.out "$SHARED_DIR/slurm-logs/" 2>/dev/null || true
        mv "${JOB_DIR}"*.err "$SHARED_DIR/slurm-logs/" 2>/dev/null || true
        log "[$CI_JOB_ID] Moved Slurm logs"
    else
        log "[$CI_JOB_ID] No Slurm logs found"
    fi

    # Remove the staging directory
    rm -rf "$JOB_DIR"
    log "[$CI_JOB_ID] Removed staging directory — done"

done

log "Orphan check complete"