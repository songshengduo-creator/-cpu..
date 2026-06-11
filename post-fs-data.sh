#!/system/bin/sh
FAKE="/data/adb/modules/cpuinfo_spoofer/cpuinfo_spoof"
TARGET="/proc/cpuinfo"

[ ! -f "$FAKE" ] && exit 1

ORIG_SELINUX=$(getenforce)
setenforce 0

umount "$TARGET" 2>/dev/null
mount --bind "$FAKE" "$TARGET"

(
    while true
    do
        if ! mount | grep -q "$TARGET"; then
            umount "$TARGET" 2>/dev/null
            mount --bind "$FAKE" "$TARGET"
        fi
        sleep 1
    done
) &

# 恢复系统原始 SELinux 状态
setenforce "$ORIG_SELINUX"

exit 0
