#!/bin/bash
#SBATCH --job-name=pace_stride_quick_run
#SBATCH --time=00:30:00
#SBATCH --mem=32G
#SBATCH --output=run_stride_%j.out
#SBATCH --error=run_stride_%j.err
#SBATCH --partition=CPU
#SBATCH --cpus-per-task=18

set -e

chmod +x stride startSolver
if [ -f .env ]; then
  source .env
else
  echo "Warning: .env missing, using defaults"
fi
mkdir -p "$STRIDE_DOWNLOADS_PATH"

./stride download -i instances.lst
./stride run -O -i instances.lst