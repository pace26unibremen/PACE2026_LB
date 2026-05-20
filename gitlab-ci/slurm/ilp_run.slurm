#!/bin/bash
#SBATCH --job-name=pace_stride_release_run
#SBATCH --time=12:00:00
#SBATCH --mem=124G
#SBATCH --output=run_stride_%j.out
#SBATCH --error=run_stride_%j.err
#SBATCH --partition=CPU
#SBATCH --cpus-per-task=1

# With 124G of memory, we can run 8 instances in parallel, each with 15G of memory.
# This is a more conservative setting than the 18 instances with 6G of memory each, to avoid running out of memory.

set -e

chmod +x stride startSolver
if [ -f .env ]; then
  source .env
else
  echo "Warning: .env missing, using defaults"
fi
mkdir -p "$STRIDE_DOWNLOADS_PATH"

./stride download -i instances.lst
./stride run -p 1 -i instances.lst