#!/bin/bash -l
#SBATCH --job-name=apac-bora
#SBATCH --chdir=/projets/schnaps/apac
#SBATCH --output=/projets/schnaps/apac/apac-bora-%j.out
#SBATCH --error=/projets/schnaps/apac/apac-bora-%j.err
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=36
#SBATCH --constraint=bora&cascadelake
#SBATCH --hint=nomultithread
#SBATCH --exclusive
#SBATCH --time=24:00:00

# Soumettre depuis /projets/schnaps/apac : sbatch apac-bora.sbatch
# Options facultatives, par exemple :
#   sbatch apac-bora.sbatch --profile small
#   sbatch apac-bora.sbatch --cases matmul --size 1024 --grain 32
#   sbatch apac-bora.sbatch --cost-threshold 10000000
#   sbatch apac-bora.sbatch --variants apac-cost
# PROFILE, REPEATS, WARMUPS, COST_THRESHOLDS et BENCH_CXXFLAGS peuvent etre exportes.
# Par defaut : sans limite, profondeur automatique et seuils de cout 100000, 1000000, 100000000, 1000000000.
# Le lanceur isole les reglages APAC_* pour chacune des trois variantes.
# Les arguments ajoutes a sbatch sont transmis au lanceur en fin de commande.
set -euo pipefail

module purge
module load git/2.54.0 gcc-toolchain/14.3.0 python/3.12.12
module list

bench_dir=/projets/schnaps/apac/apac-benchmarks
if [[ ! -f "$bench_dir/run.py" || ! -f "$bench_dir/manifest.json" ]]; then
    echo "Dossier de benchmarks incomplet : $bench_dir" >&2
    exit 1
fi

cpus=${SLURM_CPUS_PER_TASK:-36}
threads=(1 2 4 8 16 18 24 32 36)
for count in "${threads[@]}"; do
    if (( count > cpus )); then
        echo "Allocation insuffisante : $cpus CPU pour $count threads." >&2
        exit 1
    fi
done

# Meme compilation et meme politique d'affinite sur les deux architectures.
# Pas de -march=native par defaut, pour faciliter leur comparaison.
export OMP_DYNAMIC=FALSE
export OMP_PLACES=cores
export OMP_PROC_BIND=close
export OMP_MAX_ACTIVE_LEVELS=1
export OMP_THREAD_LIMIT="$cpus"

profile=${PROFILE:-standard}
repeats=${REPEATS:-7}
warmups=${WARMUPS:-1}
cxxflags=${BENCH_CXXFLAGS:--O2}
read -r -a cost_thresholds <<< "${COST_THRESHOLDS:-${COST_THRESHOLD:-100000 1000000 100000000 1000000000}}"

printf 'Job=%s | machine=%s | famille=bora | CPU=%s\n' \
    "${SLURM_JOB_ID:-unknown}" "$(hostname)" "$cpus"
printf 'Profil=%s | repetitions=%s | chauffe=%s | flags=%s\n' \
    "$profile" "$repeats" "$warmups" "$cxxflags"
printf 'Variantes=unlimited,depth,cost | seuils de cout=%s\n' "${cost_thresholds[*]}"
python3 --version
g++ -dumpfullversion -dumpversion

# Un seul processus Python dispose de tous les coeurs reserves. Le lanceur
# regle OMP_NUM_THREADS pour chaque mesure et verifie la taille reelle de l'equipe.
# Sources et resultats restent sur /projets ; chaque lancement cree un nouveau
# sous-dossier horodate dans apac-benchmarks/results, y compris en cas d'echec.
exec srun --nodes=1 --ntasks=1 --cpus-per-task="$cpus" --cpu-bind=cores \
    python3 "$bench_dir/run.py" run \
    --output-dir "$bench_dir" \
    --profile "$profile" \
    --variants apac-unlimited apac-depth apac-cost \
    --cost-thresholds "${cost_thresholds[@]}" \
    --threads "${threads[@]}" \
    --repeat "$repeats" \
    --warmups "$warmups" \
    --compiler g++ \
    --cxxflags="$cxxflags" \
    --timeout 600 \
    "$@"
