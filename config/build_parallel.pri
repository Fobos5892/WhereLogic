# Parallel build helpers for qmake + mingw32-make / GNU make.
#
# Note: .qmake.conf is loaded early, before platform scopes are always reliable.
# Do not call $$system(sh ...) here - on Windows that prints
# "The system cannot find the path specified."

isEmpty(BUILD_PARALLEL_JOBS) {
    BUILD_PARALLEL_JOBS = $$(NUMBER_OF_PROCESSORS)
}

BUILD_PARALLEL_JOBS = $$num_add($$BUILD_PARALLEL_JOBS, 0)
lessThan(BUILD_PARALLEL_JOBS, 1): BUILD_PARALLEL_JOBS = 4
greaterThan(BUILD_PARALLEL_JOBS, 64): BUILD_PARALLEL_JOBS = 64

# Cap under GitHub Actions to avoid OOM when game + tests compile together.
GITHUB_ACTIONS_ENV = $$(GITHUB_ACTIONS)
!isEmpty(GITHUB_ACTIONS_ENV) {
    greaterThan(BUILD_PARALLEL_JOBS, 2): BUILD_PARALLEL_JOBS = 2
}

message("Build: use $$BUILD_PARALLEL_JOBS parallel jobs (make -j$$BUILD_PARALLEL_JOBS)")
