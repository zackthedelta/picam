#!/bin/sh

LOGFILE="/var/log/picam-monitor.log"
INTERVAL="30"

mkdir -p "$(dirname "$LOGFILE")"

while true; do
    TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

    CPU_LINE=$(head -n 1 /proc/stat)
    set -- $CPU_LINE

    USER=$2
    NICE=$3
    SYSTEM=$4
    IDLE=$5
    IOWAIT=$6
    IRQ=$7
    SOFTIRQ=$8
    STEAL=$9
    GUEST=${10}
    GUEST_NICE=${11}

    TOTAL=$((USER + NICE + SYSTEM + IDLE + IOWAIT + IRQ + SOFTIRQ + STEAL + GUEST + GUEST_NICE))
    CPU_USAGE=$((100 - ((IDLE * 100) / TOTAL)))

    MEM_TOTAL=$(awk '/MemTotal/ {print $2}' /proc/meminfo)
    MEM_AVAILABLE=$(awk '/MemAvailable/ {print $2}' /proc/meminfo)
    MEM_USED=$((MEM_TOTAL - MEM_AVAILABLE))
    MEM_PERCENT=$((MEM_USED * 100 / MEM_TOTAL))

    printf '%s cpu_usage=%s%% mem_total_kb=%s mem_used_kb=%s mem_used_percent=%s%%\n' \
        "$TIMESTAMP" "$CPU_USAGE" "$MEM_TOTAL" "$MEM_USED" "$MEM_PERCENT" >> "$LOGFILE"

    sleep "$INTERVAL"
done
