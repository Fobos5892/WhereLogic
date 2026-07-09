# Parallel build helpers for qmake + mingw32-make / GNU make.
#
# Qt Creator (recommended):
#   Projects > Build > Build Steps > Make > "Parallel jobs" = number of CPU cores
#   (or leave empty and use scripts/build.ps1 / scripts/build.sh)
#
# Command line:
#   powershell -File scripts/build.ps1
#   ./scripts/build.sh
#
# Override job count:
#   qmake BUILD_PARALLEL_JOBS=12
#   powershell -File scripts/build.ps1 -Jobs 12

isEmpty(BUILD_PARALLEL_JOBS) {
    win32 {
        BUILD_PARALLEL_JOBS = $$(NUMBER_OF_PROCESSORS)
    } else {
        BUILD_PARALLEL_JOBS = $$system(sh -c "nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4")
    }
}

BUILD_PARALLEL_JOBS = $$num_add($$BUILD_PARALLEL_JOBS, 0)
lessThan(BUILD_PARALLEL_JOBS, 1): BUILD_PARALLEL_JOBS = 4
greaterThan(BUILD_PARALLEL_JOBS, 64): BUILD_PARALLEL_JOBS = 64

message("Build: use $$BUILD_PARALLEL_JOBS parallel jobs (make -j$$BUILD_PARALLEL_JOBS)")
