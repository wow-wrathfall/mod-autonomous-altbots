#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
module_root="$(cd -- "${script_dir}/.." && pwd)"
core_root="$(cd -- "${module_root}/../.." && pwd)"
check_only=0
reverse=0

usage() {
    echo "Usage: $0 [--core-root PATH] [--check] [--reverse]"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --core-root)
            core_root="$(cd -- "$2" && pwd)"
            shift 2
            ;;
        --check)
            check_only=1
            shift
            ;;
        --reverse)
            reverse=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 2
            ;;
    esac
done

playerbots_root="${core_root}/modules/mod-playerbots"
core_patch="${module_root}/patches/azerothcore-playerbot.patch"
playerbots_patch="${module_root}/patches/mod-playerbots.patch"

[[ -f "${core_root}/CMakeLists.txt" ]] || { echo "AzerothCore root not found: ${core_root}" >&2; exit 1; }
[[ -f "${playerbots_root}/src/Bot/PlayerbotAI.cpp" ]] || {
    echo "mod-playerbots not found: ${playerbots_root}" >&2
    exit 1
}

names=("AzerothCore" "mod-playerbots")
repos=("${core_root}" "${playerbots_root}")
patches=("${core_patch}" "${playerbots_patch}")
actions=()

for i in "${!names[@]}"; do
    can_apply=0
    can_reverse=0
    git -C "${repos[$i]}" apply --check --whitespace=error-all "${patches[$i]}" >/dev/null 2>&1 && can_apply=1
    git -C "${repos[$i]}" apply --reverse --check --whitespace=error-all "${patches[$i]}" >/dev/null 2>&1 && can_reverse=1

    if [[ $reverse -eq 1 ]]; then
        if [[ $can_reverse -eq 1 ]]; then
            actions+=("reverse")
        elif [[ $can_apply -eq 1 ]]; then
            actions+=("skip")
        else
            echo "${names[$i]} patch is neither cleanly applied nor cleanly reversible." >&2
            exit 1
        fi
    else
        if [[ $can_apply -eq 1 ]]; then
            actions+=("apply")
        elif [[ $can_reverse -eq 1 ]]; then
            actions+=("skip")
        else
            echo "${names[$i]} patch does not match this revision. Do not force it." >&2
            exit 1
        fi
    fi
done

for i in "${!names[@]}"; do
    if [[ "${actions[$i]}" == "skip" ]]; then
        [[ $reverse -eq 1 ]] && state="already absent" || state="already applied"
        echo "${names[$i]}: ${state}"
        continue
    fi

    if [[ $check_only -eq 1 ]]; then
        echo "${names[$i]}: patch can ${actions[$i]} cleanly"
        continue
    fi

    if [[ "${actions[$i]}" == "reverse" ]]; then
        git -C "${repos[$i]}" apply --reverse --whitespace=error-all "${patches[$i]}"
        echo "${names[$i]}: patch reversed"
    else
        git -C "${repos[$i]}" apply --whitespace=error-all "${patches[$i]}"
        echo "${names[$i]}: patch applied"
    fi
done

if [[ $reverse -eq 0 ]]; then
    echo
    echo "Next: set EnablePlayerSettings = 1 and AiPlayerbot.DisabledWithoutRealPlayer = 0, then rebuild."
fi
