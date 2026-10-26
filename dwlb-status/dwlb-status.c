#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/select.h>
#include <fcntl.h>

/* ---------- Tunables ---------- */
#ifndef RAPL_EVERY_DEFAULT
#define RAPL_EVERY_DEFAULT 5   /* sample RAPL each tick by default (set via -r or env RAPL_EVERY) */
#endif

typedef struct {
    void (*update)(char *out, size_t sz);
    const char *left_click;
    const char *middle_click;
    const char *right_click;
    const char *scroll_up;
    const char *scroll_down;
    const char *name;
    uint64_t interval_us;
    int signal_idx; /* -1 if no signal */
    uint64_t next_deadline_us;
    char buffer[128];
    bool asynchronous;
    pid_t worker_pid;
    int worker_fd;
} __attribute__((aligned(64))) Unit;

static void notify_text(const char *title, const char *message);

/* Signal mapping: bitmask for units */
#define MAX_UPDATE_SIGNALS 32
volatile sig_atomic_t update_mask = 0;
static sigset_t update_signals;

static void handle_sig(int sig) {
    int idx = sig - SIGRTMIN;
    if (idx >= 0 && idx < MAX_UPDATE_SIGNALS && sig <= SIGRTMAX)
        update_mask |= (sig_atomic_t)(1u << idx);
}

static const uint64_t STATUS_TICK_US = 200000ull;
static const int    STATUS_TICK      = 6; /* former 1.2-second status cadence */
static const int    THEME_EVERY    = 600;
static const int    VOL_EVERY      = 60;
static const uint64_t MUSIC_EVERY_US = 400000ull;
static const int    BATT_EVERY     = 120;
static const int    TIME_EVERY     = 48;
static const int    DISK_EVERY     = 720;
static const int    AIRPODS_EVERY  = 18;  /* poll airpods every ~3.6 seconds */
static const uint64_t GOLD_EVERY_US = 30ull * 60ull * 1000000ull;
static const uint64_t WEATHER_EVERY_US = 30ull * 60ull * 1000000ull;

static bool valid_signal_idx(int idx) {
    return idx >= 0 && idx < MAX_UPDATE_SIGNALS && SIGRTMIN + idx <= SIGRTMAX;
}

static bool setup_signal_handlers(void) {
    sigemptyset(&update_signals);
    for (int i = 0; i < MAX_UPDATE_SIGNALS && SIGRTMIN + i <= SIGRTMAX; ++i)
        if (sigaddset(&update_signals, SIGRTMIN + i) != 0)
            return false;

    if (sigprocmask(SIG_BLOCK, &update_signals, NULL) != 0)
        return false;

    struct sigaction action = {0};
    action.sa_handler = handle_sig;
    action.sa_mask = update_signals;
    for (int i = 0; i < MAX_UPDATE_SIGNALS && SIGRTMIN + i <= SIGRTMAX; ++i)
        if (sigaction(SIGRTMIN + i, &action, NULL) != 0)
            return false;
    return true;
}

/* ---------- Helpers ---------- */
static inline uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

/* Network requests must never hold up the first frame or periodic local
 * updates. Workers only calculate text; the parent owns rendering/signals. */
static bool update_unit(Unit *unit) {
    if (!unit->asynchronous) {
        unit->update(unit->buffer, sizeof unit->buffer);
        return true;
    }
    if (unit->worker_pid > 0) return true;
    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) < 0) return false;
    if (fds[0] >= FD_SETSIZE) { close(fds[0]); close(fds[1]); return false; }
    pid_t pid = fork();
    if (pid < 0) { close(fds[0]); close(fds[1]); return false; }
    if (pid == 0) {
        close(fds[0]);
        /* Do not keep the bar's output pipeline alive across a restart. */
        close(STDOUT_FILENO);
        char result[sizeof unit->buffer] = {0};
        unit->update(result, sizeof result);
        ssize_t written;
        do { written = write(fds[1], result, sizeof result); }
        while (written < 0 && errno == EINTR);
        close(fds[1]);
        _exit(written == (ssize_t)sizeof result ? 0 : 1);
    }
    close(fds[1]);
    unit->worker_pid = pid;
    unit->worker_fd = fds[0];
    return true;
}

static bool collect_unit(Unit *unit) {
    if (unit->worker_pid <= 0) return false;
    int status;
    pid_t result = waitpid(unit->worker_pid, &status, WNOHANG);
    if (result == 0 || (result < 0 && errno == EINTR)) return false;
    char text[sizeof unit->buffer] = {0};
    bool changed = false;
    if (result > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0 &&
        read(unit->worker_fd, text, sizeof text) == (ssize_t)sizeof text) {
        text[sizeof text - 1] = '\0';
        changed = memcmp(unit->buffer, text, sizeof text) != 0;
        memcpy(unit->buffer, text, sizeof text);
    }
    close(unit->worker_fd);
    unit->worker_pid = 0;
    return changed;
}

static bool read_u64_file(const char *path, uint64_t *out) {
    FILE *f = fopen(path, "re");
    if (!f) return false;
    char buf[64];
    if (!fgets(buf, sizeof buf, f)) { fclose(f); return false; }
    fclose(f);
    char *end = NULL;
    errno = 0;
    unsigned long long v = strtoull(buf, &end, 10);
    if (errno != 0) return false;
    *out = (uint64_t)v;
    return true;
}

static bool path_readable(const char *p) { return p && access(p, R_OK) == 0; }

static bool sysfs_file_path(char *out, size_t outsz, const char *base, const char *file) {
    int written = snprintf(out, outsz, "%s/%s", base, file);
    return written >= 0 && (size_t)written < outsz;
}

#define MAX_POWER_COUNTERS 32
typedef struct {
    char path[PATH_MAX];
    char name[64];
    uint64_t max_range;
    uint64_t previous;
    bool has_range;
    bool valid;
} PowerCounter;

static PowerCounter package_counters[MAX_POWER_COUNTERS];
static PowerCounter core_counters[MAX_POWER_COUNTERS];
static PowerCounter uncore_counters[MAX_POWER_COUNTERS];
static size_t package_count = 0, core_count = 0, uncore_count = 0;
static bool power_first_render = true;
static char power_provider[64] = "none";
static char direct_power_path[PATH_MAX] = "";
static char direct_power_name[64] = "";

static bool read_text_file(const char *path, char *out, size_t outsz) {
    FILE *f = fopen(path, "re");
    if (!f) return false;
    bool ok = fgets(out, outsz, f) != NULL;
    fclose(f);
    if (ok) out[strcspn(out, "\n")] = 0;
    return ok;
}

static bool appendf(char *out, size_t outsz, size_t *len, const char *fmt, ...) {
    if (*len >= outsz || outsz == 0) return false;

    va_list ap;
    va_start(ap, fmt);
    int written = vsnprintf(out + *len, outsz - *len, fmt, ap);
    va_end(ap);
    if (written < 0) return false;
    if ((size_t)written >= outsz - *len) {
        *len = outsz - 1;
        out[*len] = '\0';
        return false;
    }
    *len += (size_t)written;
    return true;
}

static void add_power_counter(PowerCounter *counters, size_t *count, const char *path, const char *name) {
    if (*count >= MAX_POWER_COUNTERS || strlen(path) >= PATH_MAX) return;
    for (size_t i = 0; i < *count; ++i) {
        if (strcmp(counters[i].path, path) == 0) return;
    }
    char resolved[PATH_MAX];
    if (!realpath(path, resolved)) return;
    PowerCounter *c = &counters[(*count)++];
    memset(c, 0, sizeof *c);
    snprintf(c->path, sizeof c->path, "%s", resolved);
    snprintf(c->name, sizeof c->name, "%s", name);
    char range_path[PATH_MAX + 32];
    snprintf(range_path, sizeof range_path, "%s/max_energy_range_uj", c->path);
    c->has_range = read_u64_file(range_path, &c->max_range) && c->max_range > 0;
    char energy_path[PATH_MAX + 16];
    snprintf(energy_path, sizeof energy_path, "%s/energy_uj", c->path);
    c->valid = read_u64_file(energy_path, &c->previous);
}

static bool powercap_entry(const char *path, char *name, size_t namesz, char *provider, size_t providersz) {
    char name_path[PATH_MAX], energy_path[PATH_MAX];
    snprintf(name_path, sizeof name_path, "%s/name", path);
    snprintf(energy_path, sizeof energy_path, "%s/energy_uj", path);
    if (!read_text_file(name_path, name, namesz) || !path_readable(energy_path)) return false;
    const char *base = strrchr(path, '/') + 1;
    size_t len = strcspn(base, ":");
    if (len == 0 || len >= providersz) return false;
    memcpy(provider, base, len);
    provider[len] = '\0';
    return true;
}

/* AMD APUs expose the GPU PPT through the amdgpu hwmon device.  This is a
 * component measurement, not the APU package total, so it is detail-only. */
static void discover_amdgpu_power(void) {
    DIR *root = opendir("/sys/class/hwmon");
    if (!root) return;

    struct dirent *de;
    while ((de = readdir(root))) {
        if (strncmp(de->d_name, "hwmon", 5) != 0) continue;

        char base[PATH_MAX], name_path[PATH_MAX], name[64];
        if (snprintf(base, sizeof base, "/sys/class/hwmon/%s", de->d_name) >= (int)sizeof base ||
            snprintf(name_path, sizeof name_path, "%s/name", base) >= (int)sizeof name_path ||
            !read_text_file(name_path, name, sizeof name) || strcmp(name, "amdgpu") != 0)
            continue;

        for (int i = 1; i <= 32; ++i) {
            char path[PATH_MAX], label_path[PATH_MAX], label[64] = "power";
            if (snprintf(path, sizeof path, "%s/power%d_average", base, i) >= (int)sizeof path ||
                !path_readable(path)) {
                if (snprintf(path, sizeof path, "%s/power%d_input", base, i) >= (int)sizeof path ||
                    !path_readable(path))
                    continue;
            }
            if (snprintf(label_path, sizeof label_path, "%s/power%d_label", base, i) < (int)sizeof label_path)
                read_text_file(label_path, label, sizeof label);
            snprintf(direct_power_path, sizeof direct_power_path, "%s", path);
            snprintf(direct_power_name, sizeof direct_power_name, "AMD GPU (%s)", label);
            snprintf(power_provider, sizeof power_provider, "amdgpu-hwmon");
            closedir(root);
            return;
        }
    }
    closedir(root);
}

static void discover_powercounters(void) {
    DIR *root = opendir("/sys/class/powercap");
    if (!root) { discover_amdgpu_power(); return; }
    char preferred[64] = "";
    bool have_intel = false, have_amd = false;
    struct dirent *de;
    while ((de = readdir(root))) {
        if (de->d_name[0] == '.') continue;
        char path[PATH_MAX];
        if (snprintf(path, sizeof path, "/sys/class/powercap/%s", de->d_name) >= (int)sizeof path)
            continue;
        char name[64], provider[64];
        if (!powercap_entry(path, name, sizeof name, provider, sizeof provider)) continue;
        if (strncmp(name, "package", 7) == 0 && (name[7] == '-' || name[7] == '\0')) {
            if (strcmp(provider, "intel-rapl") == 0) have_intel = true;
            if (strcmp(provider, "amd-rapl") == 0) have_amd = true;
            if (!preferred[0]) snprintf(preferred, sizeof preferred, "%s", provider);
        }
    }
    closedir(root);
    if (have_intel) snprintf(preferred, sizeof preferred, "intel-rapl");
    else if (have_amd) snprintf(preferred, sizeof preferred, "amd-rapl");
    if (!preferred[0]) { discover_amdgpu_power(); return; }
    snprintf(power_provider, sizeof power_provider, "%s", preferred);

    root = opendir("/sys/class/powercap");
    if (!root) return;
    while ((de = readdir(root))) {
        if (de->d_name[0] == '.') continue;
        char path[PATH_MAX], name[64], provider[64];
        if (snprintf(path, sizeof path, "/sys/class/powercap/%s", de->d_name) >= (int)sizeof path)
            continue;
        if (!powercap_entry(path, name, sizeof name, provider, sizeof provider) ||
            strcmp(provider, preferred) != 0) continue;
        if (strncmp(name, "package", 7) == 0 && (name[7] == '-' || name[7] == '\0'))
            add_power_counter(package_counters, &package_count, path, name);
        else if (strcmp(name, "core") == 0)
            add_power_counter(core_counters, &core_count, path, name);
        else if (strcmp(name, "uncore") == 0)
            add_power_counter(uncore_counters, &uncore_count, path, name);
    }
    closedir(root);
    discover_amdgpu_power();
}

#define MAX_CPU_CORES 128
static char *cpu_temp_path_g = NULL;
static char *cpu_core_path_g = NULL;
static char *cpu_core_paths_g[MAX_CPU_CORES];
static size_t cpu_core_count_g = 0;

/* Scan hwmon for the package temperature and all available core inputs. */
static void discover_temp_paths(void) {
    DIR *root = opendir("/sys/class/hwmon");
    if (!root) return;
    struct dirent *de;
    while ((de = readdir(root))) {
        if (strncmp(de->d_name, "hwmon", 5) != 0) continue;
        char base[64];
        if (snprintf(base, sizeof base, "/sys/class/hwmon/%s", de->d_name) >= (int)sizeof(base)) continue;
        char namep[72]; 
        if (snprintf(namep, sizeof namep, "%s/name", base) >= (int)sizeof(namep)) continue;
        FILE *nf = fopen(namep, "re");
        if (!nf) continue;
        char name[32] = {0};
        if (fgets(name, sizeof name, nf)) {
            for (char *p = name; *p; ++p) *p = tolower(*p);
            if (strstr(name, "coretemp")) {
                for (int i = 1; i <= 32; ++i) {
                    char lbl[80]; 
                    if (snprintf(lbl, sizeof lbl, "%s/temp%d_label", base, i) >= (int)sizeof(lbl)) continue;
                    FILE *lf = fopen(lbl, "re");
                    if (!lf) continue;
                    char ltxt[32] = {0};
                    if (fgets(ltxt, sizeof ltxt, lf)) {
                        if (!cpu_temp_path_g && strncmp(ltxt, "Package id 0", 12) == 0) {
                            asprintf(&cpu_temp_path_g, "%s/temp%d_input", base, i);
                        } else if (strncmp(ltxt, "Core ", 5) == 0) {
                            char *path = NULL;
                            asprintf(&path, "%s/temp%d_input", base, i);
                            if (path && cpu_core_count_g < MAX_CPU_CORES) {
                                cpu_core_paths_g[cpu_core_count_g++] = path;
                                if (!cpu_core_path_g && strncmp(ltxt, "Core 0", 6) == 0)
                                    cpu_core_path_g = path;
                            } else {
                                free(path);
                            }
                        }
                    }
                    fclose(lf);
                }
            }
        }
        fclose(nf);
    }
    closedir(root);

    /* Fallback if Package id 0 not found */
    if (!cpu_temp_path_g) {
        root = opendir("/sys/class/hwmon");
        if (root) {
            while ((de = readdir(root))) {
                if (strncmp(de->d_name, "hwmon", 5) != 0) continue;
                char base[PATH_MAX];
                if (snprintf(base, sizeof base, "/sys/class/hwmon/%s", de->d_name) >= (int)sizeof base)
                    continue;
                for (int i = 1; i <= 32; ++i) {
                    char inp[PATH_MAX + 16];
                    if (snprintf(inp, sizeof inp, "%s/temp%d_input", base, i) >= (int)sizeof inp)
                        continue;
                    if (access(inp, R_OK) == 0) {
                        cpu_temp_path_g = strdup(inp);
                        break;
                    }
                }
                if (cpu_temp_path_g) break;
            }
            closedir(root);
        }
    }
}

/* Read first line of /proc/stat: returns idle and total jiffies */
static bool read_cpu_totals(uint64_t *idle, uint64_t *total) {
    FILE *f = fopen("/proc/stat", "re");
    if (!f) return false;
    char tag[8];
    unsigned long long u,n,s,i,io,ir,so,st,gu,gn;
    int nread = fscanf(f, "%7s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                       tag, &u, &n, &s, &i, &io, &ir, &so, &st, &gu, &gn);
    fclose(f);
    if (nread < 5) return false;
    *idle  = (uint64_t)i + (uint64_t)io;
    *total = (uint64_t)u + (uint64_t)n + (uint64_t)s + (uint64_t)i +
             (uint64_t)io + (uint64_t)ir + (uint64_t)so + (uint64_t)st;
    return true;
}

/* Memory: read MemTotal & MemAvailable from /proc/meminfo (KiB) */
static bool read_mem_kib(uint64_t *total_kib, uint64_t *avail_kib) {
    FILE *f = fopen("/proc/meminfo", "re");
    if (!f) return false;
    char key[64]; unsigned long long val; char unit[32];
    uint64_t t=0, a=0;
    while (fscanf(f, "%63s %llu %31s", key, &val, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) t = val;
        else if (strcmp(key, "MemAvailable:") == 0) a = val;
        if (t && a) break;
    }
    fclose(f);
    if (!t || !a) return false;
    *total_kib = t; *avail_kib = a;
    return true;
}

/* Disk usage via statvfs() (bytes) */
static bool read_disk_bytes(const char *path, uint64_t *used, uint64_t *total) {
    struct statvfs v;
    if (statvfs(path, &v) != 0) return false;
    uint64_t total_b = (uint64_t)v.f_frsize * (uint64_t)v.f_blocks;
    uint64_t avail_b = (uint64_t)v.f_frsize * (uint64_t)v.f_bavail;
    uint64_t used_b  = total_b - avail_b;
    *total = total_b; *used = used_b; return true;
}

static bool discover_battery(char *path, size_t pathsz) {
    DIR *root = opendir("/sys/class/power_supply");
    if (!root) return false;

    bool found = false;
    struct dirent *de;
    while ((de = readdir(root))) {
        if (de->d_name[0] == '.') continue;
        char candidate[PATH_MAX], type_path[PATH_MAX], type[32];
        if (snprintf(candidate, sizeof candidate, "/sys/class/power_supply/%s", de->d_name) >= (int)sizeof candidate ||
            snprintf(type_path, sizeof type_path, "%s/type", candidate) >= (int)sizeof type_path ||
            !read_text_file(type_path, type, sizeof type) || strcmp(type, "Battery") != 0)
            continue;
        snprintf(path, pathsz, "%s", candidate);
        found = true;
        break;
    }
    closedir(root);
    return found;
}

static bool battery_u64(const char *bat, const char *attr, uint64_t *out) {
    char p[PATH_MAX + 16];
    return sysfs_file_path(p, sizeof p, bat, attr) && read_u64_file(p, out);
}

/* Battery snapshot: returns text like "🔌 92% ~1.8h" or "🔋 85%" */
static void battery_text(char *out, size_t outsz) {
    char B[PATH_MAX];
    if (!discover_battery(B, sizeof B)) { snprintf(out, outsz, "🔋 ??"); return; }
    char cap_p[PATH_MAX + 16], stat_p[PATH_MAX + 16];
    if (!sysfs_file_path(cap_p, sizeof cap_p, B, "capacity") ||
        !sysfs_file_path(stat_p, sizeof stat_p, B, "status")) {
        snprintf(out, outsz, "🔋 ??");
        return;
    }

    /* percent */
    uint64_t pct = 0;
    bool have_pct = read_u64_file(cap_p, &pct) && pct <= 100;
    char status[16] = "Unknown";
    read_text_file(stat_p, status, sizeof status);

    /* energy/power (uWh/uW) with fallbacks */
    uint64_t en_uWh=0, ef_uWh=0, pw_uW=0;
    if (!battery_u64(B, "energy_now", &en_uWh) && !battery_u64(B, "energy_now_uwh", &en_uWh)) en_uWh=0;
    if (!battery_u64(B, "energy_full", &ef_uWh) && !battery_u64(B, "energy_full_uwh", &ef_uWh)) ef_uWh=0;
    if (!battery_u64(B, "power_now", &pw_uW)) pw_uW=0;

    if ((en_uWh == 0 || ef_uWh == 0) || pw_uW == 0) {
        uint64_t qn=0, qf=0, vv=0, ia=0;
        battery_u64(B, "charge_now", &qn);
        battery_u64(B, "charge_full", &qf);
        battery_u64(B, "voltage_now", &vv);
        if (en_uWh == 0 && qn > 0 && vv > 0) en_uWh = (qn * vv) / 1000000ull;
        if (ef_uWh == 0 && qf > 0 && vv > 0) ef_uWh = (qf * vv) / 1000000ull;
        if (!have_pct && qf > 0) {
            pct = (qn * 100ull + qf / 2ull) / qf;
            have_pct = true;
        }
        if (pw_uW == 0) {
            battery_u64(B, "current_now", &ia);
            if (ia > 0 && vv > 0) pw_uW = (ia * vv) / 1000000ull;
        }
    }
    if (!have_pct && ef_uWh > 0) {
        pct = (en_uWh * 100ull + ef_uWh / 2ull) / ef_uWh;
        have_pct = true;
    }

    char icon[8] = "🔋";
    if (strcmp(status, "Charging") == 0) strcpy(icon, "🔌");

    /* ETA (hours, 1 decimal) */
    char eta[32] = "";
    if (pw_uW > 0) {
        if (strcmp(status, "Discharging") == 0) {
            uint64_t t10 = (en_uWh * 10ull + pw_uW/2ull) / pw_uW;
            snprintf(eta, sizeof eta, " ~%llu.%lluh", (unsigned long long)(t10/10ull), (unsigned long long)(t10%10ull));
        } else if (strcmp(status, "Charging") == 0 && ef_uWh > en_uWh) {
            uint64_t rem = ef_uWh - en_uWh;
            uint64_t t10 = (rem * 10ull + pw_uW/2ull) / pw_uW;
            snprintf(eta, sizeof eta, " ~%llu.%lluh", (unsigned long long)(t10/10ull), (unsigned long long)(t10%10ull));
        }
    }

    if (have_pct)
        snprintf(out, outsz, "%s %llu%%%s", icon, (unsigned long long)pct, eta);
    else
        snprintf(out, outsz, "%s%s", icon, eta);
}

static void battery_details(void) {
    char details[512] = "";
    FILE *fp = popen(
        "upower -d 2>/dev/null | awk '"
        "/energy-rate:/ { rate=$2 \" \" $3 }\n"
        "/energy-full-design:/ { design=$2 \" \" $3; design_value=$2 }\n"
        "/energy-full:/ { full=$2 \" \" $3; full_value=$2 }\n"
        "/energy:/ { energy=$2 \" \" $3 }\n"
        "/percentage:/ { percentage=$2 }\n"
        "/state:/ { state=$2 }\n"
        "/voltage:/ { voltage=$2 \" \" $3 }\n"
        "/time to empty:/ { empty=$4 \" \" $5 }\n"
        "/time to full:/ { charge=$4 \" \" $5 }\n"
        "END {\n"
        "    if (rate != \"\") print \"Energy rate: \" rate\n"
        "    if (design != \"\") print \"Designed capacity: \" design\n"
        "    if (full != \"\") {\n"
        "        printf \"Actual capacity: %s\\n\", full\n"
        "        if (design_value > 0) printf \"Capacity health: %.0f%%\\n\", full_value / design_value * 100\n"
        "    }\n"
        "    if (energy != \"\") print \"Current energy: \" energy\n"
        "    if (percentage != \"\") print \"Charge: \" percentage\n"
        "    if (state != \"\") print \"State: \" state\n"
        "    if (voltage != \"\") print \"Voltage: \" voltage\n"
        "    if (empty != \"\") print \"Time to empty: \" empty\n"
        "    if (charge != \"\") print \"Time to full: \" charge\n"
        "}'", "r");
    if (fp) {
        size_t len = 0;
        while (len + 1 < sizeof details && fgets(details + len, sizeof details - len, fp))
            len = strlen(details);
        while (len > 0 && details[len - 1] == '\n') details[--len] = 0;
        pclose(fp);
    }

    if (!details[0])
        snprintf(details, sizeof details, "Battery details unavailable (upower -d)");
    notify_text("Battery", details);
}

/* Read pamixer outputs (polled infrequently) - with timeout to prevent hangs */
static void volume_text(char *out, size_t outsz) {
    FILE *fp;
    char buf[64];
    bool mute = false;
    int vol = 0;

    fp = popen("timeout 1 pamixer --get-mute 2>/dev/null", "r");
    if (fp) { if (fgets(buf, sizeof buf, fp)) mute = (strncmp(buf, "true", 4) == 0); pclose(fp); }

    fp = popen("timeout 1 pamixer --get-volume 2>/dev/null", "r");
    if (fp) { if (fgets(buf, sizeof buf, fp)) vol = atoi(buf); pclose(fp); }

    if (mute) {
        snprintf(out, outsz, "🔇 mute");
    } else if (vol >= 0 && vol < 33) {
        snprintf(out, outsz, "🔈 %d%%", vol);
    } else if (vol <= 66) {
        snprintf(out, outsz, "🔉 %d%%", vol);
    } else {
        snprintf(out, outsz, "🔊 %d%%", vol);
    }
}

static bool music_playing(void) {
    FILE *fp = popen("timeout 1 pactl list sink-inputs 2>/dev/null", "r");
    if (!fp) return false;

    char line[128];
    bool playing = false;
    while (fgets(line, sizeof line, fp)) {
        if (strstr(line, "Corked: no")) {
            playing = true;
            break;
        }
    }
    pclose(fp);
    return playing;
}

static void equalizer_text(char *out, size_t outsz) {
    static const char *const notes[] = { "♪✨", "✨♫", "♬✨" };
    static unsigned frame;

    if (!music_playing()) {
        out[0] = '\0';
        return;
    }
    snprintf(out, outsz, "^fg(FABA17)%s^fg()", notes[frame]);
    frame = (frame + 1) % (sizeof notes / sizeof notes[0]);
}

/* Check airpods connection status (polled infrequently) - with timeout to prevent hangs */
static void airpods_text(char *out, size_t outsz) {
    FILE *fp = popen("timeout 2 airpods -s 2>/dev/null", "r");
    if (!fp) {
        snprintf(out, outsz, "🎧 ??");
        return;
    }
    
    int status = pclose(fp);
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code == 1) {
            snprintf(out, outsz, "🎧 ^fg(00FF00)✓^fg()");  /* connected - green checkmark */
        } else {
            snprintf(out, outsz, "🎧 ^fg(FF5555)x^fg()");  /* disconnected - red x */
        }
    } else {
        snprintf(out, outsz, "🎧 ??");
    }
}

/* -1 means the default controller is unavailable or could not be queried. */
static int bluetooth_powered(void) {
    FILE *fp = popen("timeout 2 bluetoothctl show 2>/dev/null", "r");
    if (!fp) return -1;

    char line[256];
    int powered = -1;
    while (fgets(line, sizeof line, fp)) {
        char *field = line;
        while (isspace((unsigned char)*field)) ++field;
        if (strncmp(field, "Powered: yes", 12) == 0) powered = 1;
        else if (strncmp(field, "Powered: no", 11) == 0) powered = 0;
    }
    int status = pclose(fp);
    return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0 ? powered : -1;
}

static void bluetooth_text(char *out, size_t outsz) {
    int powered = bluetooth_powered();
    if (powered < 0) snprintf(out, outsz, "^fg(888888)^fg()");
    else snprintf(out, outsz, powered ? "^fg(7FD88F)^fg()" : "^fg(FF79C6)^fg()");
}

static int bluetooth_toggle(void) {
    int powered = bluetooth_powered();
    if (powered < 0) {
        notify_text("Bluetooth", "Cannot query the Bluetooth controller.");
        return 1;
    }

    const char *command = powered
        ? "timeout 5 bluetoothctl power off >/dev/null 2>&1"
        : "timeout 2 rfkill unblock bluetooth && timeout 5 bluetoothctl power on >/dev/null 2>&1";
    int status = system(command);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        notify_text("Bluetooth", "Could not toggle Bluetooth power. Check bluetoothctl and rfkill.");
        return 1;
    }
    return 0;
}

static void time_text(char *out, size_t outsz) {
    time_t t = time(NULL);
    struct tm tm; localtime_r(&t, &tm);
    char date[16], timebuf[16];
    strftime(date, sizeof date, "%Y-%m-%d", &tm);
    strftime(timebuf, sizeof timebuf, "%H:%M", &tm);
    snprintf(out, outsz, "📅 %s 🕒 %s", date, timebuf);
}

static void theme_text(char *out, size_t outsz) {
    int ret = system("switch-theme -r >/dev/null 2>&1");
    if (ret == 0) {
        snprintf(out, outsz, "🌘");
    } else {
        snprintf(out, outsz, "🌖");
    }
}

static void disk_text(char *out, size_t outsz) {
    const char *home = getenv("HOME");
    if (!home) home = "/";
    uint64_t used_b=0, tot_b=0;
    if (!read_disk_bytes(home, &used_b, &tot_b)) { read_disk_bytes("/", &used_b, &tot_b); }
    double used = (double)used_b / (1024.0*1024.0*1024.0);
    double tot  = (double)tot_b  / (1024.0*1024.0*1024.0);
    snprintf(out, outsz, "💾 %.0f/%.0fGiB", used, tot);
}

static void gold_text(char *out, size_t outsz) {
    FILE *fp = popen("curl -fsS --connect-timeout 3 --max-time 5 'https://api.goldprice.dev/v1/prices?symbol=XAU-USD-SPOT' 2>/dev/null", "r");
    if (!fp) {
        out[0] = '\0';
        return;
    }

    char response[512] = "";
    size_t len = 0;
    while (len + 1 < sizeof response && fgets(response + len, sizeof response - len, fp))
        len = strlen(response);
    int status = pclose(fp);

    const char *price = strstr(response, "\"price\":\"");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || !price) {
        out[0] = '\0';
        return;
    }
    price += strlen("\"price\":\"");
    char *end = NULL;
    errno = 0;
    double value = strtod(price, &end);
    if (errno != 0 || end == price || *end != '\"' || value <= 0.0) {
        out[0] = '\0';
        return;
    }
    snprintf(out, outsz, "^fg(FFD700)💰^fg() $%.2f/g", value / 31.1034768);
}

/* Untrusted text must not introduce dwlb commands. Never split an escaped ^^. */
static void escape_status_text(char *out, size_t outsz, const char *text) {
    if (!outsz) return;
    size_t len = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p < 0x20 || *p == 0x7f) continue;
        size_t needed = *p == '^' ? 2 : 1;
        if (needed >= outsz - len) break;
        if (*p == '^') out[len++] = '^';
        out[len++] = (char)*p;
    }
    out[len] = '\0';
}

static void weather_text(char *out, size_t outsz) {
    FILE *fp = popen("curl -fsS --connect-timeout 3 --max-time 5 'https://wttr.in/Yerevan?format=1' 2>/dev/null", "r");
    if (!fp) {
        out[0] = '\0';
        return;
    }

    bool got_weather = fgets(out, outsz, fp) != NULL;
    int status = pclose(fp);
    if (!got_weather || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        out[0] = '\0';
        return;
    }
    out[strcspn(out, "\r\n")] = '\0';
    char *src = out, *dst = out;
    bool previous_space = false;
    while (*src) {
        bool is_space = *src == ' ';
        if (!is_space || !previous_space) *dst++ = *src;
        previous_space = is_space;
        ++src;
    }
    *dst = '\0';
    if (strncmp(out, "☀️", strlen("☀️")) == 0) {
        memmove(out + strlen("🔆"), out + strlen("☀️"), strlen(out + strlen("☀️")) + 1);
        memcpy(out, "🔆", strlen("🔆"));
    }
    char raw[128];
    snprintf(raw, sizeof raw, "%s", out);
    escape_status_text(out, outsz, raw);
}

static void notify_text(const char *title, const char *message) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("notify-send", "notify-send", "-a", "dwlb-status", title, message, (char *)NULL);
        _exit(127);
    }
    if (pid > 0) waitpid(pid, NULL, 0);
}

static void disk_details(void) {
    system("notify-send -a dwlb-status 'Disk usage' \"$(df -h \"$HOME\" /)\"");
}

static void calendar_details(void) {
    system("notify-send -a dwlb-status Calendar \"$(cal -n 2)\"");
}

static void duck_text(char *out, size_t outsz) {
    snprintf(out, outsz, "🦆");
}

static void launcher_text(char *out, size_t outsz) {
    snprintf(out, outsz, "🐧");
}

static void font_text(char *out, size_t outsz) {
    snprintf(out, outsz, "Aa");
}

static void ram_text(char *out, size_t outsz) {
    uint64_t mem_tot=0, mem_avl=0;
    if (read_mem_kib(&mem_tot, &mem_avl)) {
        uint64_t used_kib = mem_tot - mem_avl;
        double used_g = (double)used_kib / 1048576.0;
        double tot_g  = (double)mem_tot   / 1048576.0;
        snprintf(out, outsz, "🧠 %.1f/%.1fGiB", used_g, tot_g);
    } else {
        snprintf(out, outsz, "🧠 --/--GiB");
    }
}

static void temp_text(char *out, size_t outsz) {
    uint64_t pkg_milli = 0, core_milli = 0;
    bool has_pkg = false, has_core = false;

    if (cpu_temp_path_g && read_u64_file(cpu_temp_path_g, &pkg_milli)) {
        has_pkg = true;
    }
    for (size_t i = 0; i < cpu_core_count_g; ++i) {
        uint64_t current = 0;
        if (read_u64_file(cpu_core_paths_g[i], &current) && (!has_core || current > core_milli)) {
            core_milli = current;
            has_core = true;
        }
    }
    if (!has_core && cpu_core_path_g && read_u64_file(cpu_core_path_g, &core_milli)) {
        has_core = true;
    }

    if (has_pkg && has_core) {
        snprintf(out, outsz, "🔥 %llu/%llu°C", (unsigned long long)(pkg_milli/1000ull), (unsigned long long)(core_milli/1000ull));
    } else if (has_pkg) {
        snprintf(out, outsz, "🔥 %llu°C", (unsigned long long)(pkg_milli/1000ull));
    } else if (has_core) {
        snprintf(out, outsz, "🔥 %llu°C", (unsigned long long)(core_milli/1000ull));
    } else {
        snprintf(out, outsz, "🔥 ?°C");
    }
}

static void temperature_details(void) {
    discover_temp_paths();
    char message[1024] = "";
    size_t len = 0;
    uint64_t pkg_milli = 0;
    if (cpu_temp_path_g && read_u64_file(cpu_temp_path_g, &pkg_milli)) {
        appendf(message, sizeof message, &len, "Package: %llu C\n",
                (unsigned long long)(pkg_milli / 1000ull));
    } else {
        appendf(message, sizeof message, &len, "Package: unavailable\n");
    }

    for (size_t i = 0; i < cpu_core_count_g; ++i) {
        uint64_t core_milli = 0;
        if (read_u64_file(cpu_core_paths_g[i], &core_milli) &&
            !appendf(message, sizeof message, &len, "Core %zu: %llu C\n",
                     i, (unsigned long long)(core_milli / 1000ull)))
            break;
    }
    if (len > 0 && message[len - 1] == '\n') message[len - 1] = '\0';
    notify_text("CPU temperatures", message);
}

static uint64_t prev_idle_g=0, prev_total_g=0;
static void cpu_load_text(char *out, size_t outsz) {
    uint64_t cur_idle=0, cur_total=0;
    read_cpu_totals(&cur_idle, &cur_total);
    uint64_t d_tot = (cur_total > prev_total_g) ? (cur_total - prev_total_g) : 0;
    uint64_t d_idl = (cur_idle  > prev_idle_g ) ? (cur_idle  - prev_idle_g ) : 0;
    prev_total_g = cur_total; prev_idle_g = cur_idle;
    unsigned cpu_pct = d_tot ? (unsigned)((d_tot - d_idl) * 100ull / d_tot) : 0;
    snprintf(out, outsz, "📈 %u%%", cpu_pct);
}

static uint64_t prev_us_g = 0;

static bool power_delta(PowerCounter *counter, uint64_t current, uint64_t *delta) {
    if (!counter->valid) {
        counter->previous = current;
        counter->valid = true;
        return false;
    }
    if (current >= counter->previous) {
        *delta = current - counter->previous;
    } else if (counter->has_range) {
        *delta = counter->max_range - counter->previous + current;
        if (*delta > counter->max_range / 2ull) {
            counter->previous = current;
            counter->valid = false;
            return false;
        }
    } else {
        counter->valid = false;
        return false;
    }
    counter->previous = current;
    return true;
}

static bool power_group_w10(PowerCounter *counters, size_t count, uint64_t dus, uint64_t *w10) {
    uint64_t energy = 0;
    bool complete = count > 0;
    for (size_t i = 0; i < count; ++i) {
        char energy_path[PATH_MAX + 16];
        uint64_t current = 0, delta = 0;
        snprintf(energy_path, sizeof energy_path, "%s/energy_uj", counters[i].path);
        if (!read_u64_file(energy_path, &current)) {
            counters[i].valid = false;
            complete = false;
            continue;
        }
        if (!power_delta(&counters[i], current, &delta)) {
            complete = false;
            continue;
        }
        energy += delta;
    }
    if (!complete) return false;
    *w10 = (energy * 10ull + dus / 2ull) / dus;
    return true;
}

static bool power_group_energy(PowerCounter *counters, size_t count, uint64_t *energy) {
    *energy = 0;
    if (count == 0) return false;
    for (size_t i = 0; i < count; ++i) {
        char path[PATH_MAX + 16];
        uint64_t current = 0;
        snprintf(path, sizeof path, "%s/energy_uj", counters[i].path);
        if (!read_u64_file(path, &current)) return false;
        *energy += current;
    }
    return true;
}

static void power_text(char *out, size_t outsz) {
    if (package_count == 0) {
        snprintf(out, outsz, "^fg(FFD700)⚡^fg() --W");
        return;
    }
    if (power_first_render) {
        power_first_render = false;
        prev_us_g = now_us();
        snprintf(out, outsz, "^fg(FFD700)⚡^fg() --W");
        return;
    }
    uint64_t cur_us = now_us();
    uint64_t dus = (cur_us > prev_us_g) ? (cur_us - prev_us_g) : 1;
    uint64_t package_w10 = 0, uncore_w10 = 0;
    bool package_ok = power_group_w10(package_counters, package_count, dus, &package_w10);
    bool uncore_ok = power_group_w10(uncore_counters, uncore_count, dus, &uncore_w10);
    prev_us_g = cur_us;
    if (!package_ok) {
        snprintf(out, outsz, "^fg(FFD700)⚡^fg() --/--W");
        return;
    }
    if (uncore_ok) {
        snprintf(out, outsz, "^fg(FFD700)⚡^fg() %llu.%lluW",
                 (unsigned long long)(package_w10 / 10ull), (unsigned long long)(package_w10 % 10ull));
    } else {
        snprintf(out, outsz, "^fg(FFD700)⚡^fg() %llu.%lluW",
                 (unsigned long long)(package_w10 / 10ull), (unsigned long long)(package_w10 % 10ull));
    }
}

static void power_details(void) {
    discover_powercounters();
    char message[512];
    if (package_count == 0) {
        uint64_t microwatts = 0;
        if (direct_power_path[0] && read_u64_file(direct_power_path, &microwatts)) {
            snprintf(message, sizeof message,
                     "APU package: unavailable\nCPU: unavailable\n%s: %.1f W",
                     direct_power_name, (double)microwatts / 1000000.0);
        } else {
            snprintf(message, sizeof message,
                     "No readable APU package energy counter is available.");
        }
    } else {
        uint64_t package_before = 0, package_after = 0, core_before = 0, core_after = 0;
        bool package_ok = power_group_energy(package_counters, package_count, &package_before);
        bool core_ok = core_count > 0 && power_group_energy(core_counters, core_count, &core_before);
        struct timespec req = { 1, 0 };
        nanosleep(&req, NULL);
        package_ok = package_ok && power_group_energy(package_counters, package_count, &package_after);
        core_ok = core_ok && power_group_energy(core_counters, core_count, &core_after);
        if (package_ok) {
            double package_w = (double)(package_after - package_before) / 1000000.0;
            uint64_t gpu_uw = 0;
            bool gpu_ok = direct_power_path[0] && read_u64_file(direct_power_path, &gpu_uw);
            if (core_ok && gpu_ok) {
                double core_w = (double)(core_after - core_before) / 1000000.0;
                snprintf(message, sizeof message,
                         "APU package: %.1f W\nCPU: %.1f W\n%s: %.1f W",
                         package_w, core_w, direct_power_name, (double)gpu_uw / 1000000.0);
            } else if (core_ok) {
                double core_w = (double)(core_after - core_before) / 1000000.0;
                snprintf(message, sizeof message,
                         "APU package: %.1f W\nCPU: %.1f W\nGPU: unavailable",
                         package_w, core_w);
            } else if (gpu_ok) {
                snprintf(message, sizeof message,
                         "APU package: %.1f W\nCPU: unavailable\n%s: %.1f W",
                         package_w, direct_power_name, (double)gpu_uw / 1000000.0);
            } else {
                snprintf(message, sizeof message,
                         "APU package: %.1f W\nCPU: unavailable\nGPU: unavailable", package_w);
            }
        } else {
            snprintf(message, sizeof message, "Unable to read power counters.");
        }
    }
    notify_text("APU power", message);
}

static void vpn_text(char *out, size_t outsz) {
    FILE *fp = popen("timeout 1 nmcli -t -f NAME,TYPE connection show --active 2>/dev/null | awk -F: '$2 == \"vpn\" { print $1; exit }'", "r");
    char name[128];
    if (fp && fgets(name, sizeof name, fp)) {
        name[strcspn(name, "\n")] = 0;
        size_t len = 0;
        const char *prefix = "🛡 ";
        while (*prefix && len + 1 < outsz) out[len++] = *prefix++;
        escape_status_text(out + len, outsz - len, name);
    } else {
        out[0] = 0;
    }
    if (fp) pclose(fp);
}

static inline bool wrap_tag(char *buf, size_t cap, size_t *len, const char *tag, size_t tag_len, const char *cmd) {
    if (__builtin_expect(cmd != NULL, 0)) {
        size_t cmd_len = strlen(cmd);
        char inner[512];
        char *p = inner;

        if (cmd_len > SIZE_MAX - *len || cmd_len + *len > SIZE_MAX - 6 ||
            tag_len > (SIZE_MAX - cmd_len - *len - 6) / 2)
            return false;
        size_t new_len = cmd_len + *len + 2 * tag_len + 6;
        if (new_len >= cap || new_len >= sizeof inner) return false;

        /* ^tag(cmd)buf^tag() */
        *p++ = '^';
        memcpy(p, tag, tag_len);
        p += tag_len;
        *p++ = '(';
        memcpy(p, cmd, cmd_len);
        p += cmd_len;
        *p++ = ')';
        memcpy(p, buf, *len);
        p += *len;
        *p++ = '^';
        memcpy(p, tag, tag_len);
        p += tag_len;
        *p++ = '(';
        *p++ = ')';
        *p = '\0';

        memcpy(buf, inner, new_len + 1);
        *len = new_len;
        return true;
    }
    return true;
}

static void render_unit(const Unit *u, char *out, size_t *out_len) {
    char temp[512];
    size_t blen = strlen(u->buffer);
    if (__builtin_expect(blen >= sizeof(temp), 0)) return;
    memcpy(temp, u->buffer, blen + 1);

    wrap_tag(temp, sizeof temp, &blen, "lm", 2, u->left_click);
    wrap_tag(temp, sizeof temp, &blen, "mm", 2, u->middle_click);
    wrap_tag(temp, sizeof temp, &blen, "rm", 2, u->right_click);
    wrap_tag(temp, sizeof temp, &blen, "us", 2, u->scroll_up);
    wrap_tag(temp, sizeof temp, &blen, "ds", 2, u->scroll_down);

    memcpy(out, temp, blen + 1);
    *out_len = blen;
}

static void send_signal(int sig_idx) {
    DIR *dir = opendir("/proc");
    if (!dir) return;
    struct dirent *de;
    pid_t my_pid = getpid();
    while ((de = readdir(dir))) {
        if (!isdigit(de->d_name[0])) continue;
        pid_t pid = atoi(de->d_name);
        if (pid == my_pid) continue;

        char cmdpath[32];
        if (snprintf(cmdpath, sizeof(cmdpath), "/proc/%d/comm", pid) >= (int)sizeof(cmdpath)) continue;
        FILE *f = fopen(cmdpath, "r");
        if (f) {
            char comm[32];
            if (fgets(comm, sizeof(comm), f)) {
                comm[strcspn(comm, "\n")] = 0;
                if (strcmp(comm, "dwlb-status") == 0) {
                    kill(pid, SIGRTMIN + sig_idx);
                }
            }
            fclose(f);
        }
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    /* dwlb ignores SIGCHLD; click-launched helpers must be able to wait for
     * their children so popen/system report the actual command status. */
    struct sigaction child_action = {0};
    child_action.sa_handler = SIG_DFL;
    sigemptyset(&child_action.sa_mask);
    if (sigaction(SIGCHLD, &child_action, NULL) != 0) return 1;

    /* Signal mode: ./dwlb-status --signal N */
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--bluetooth-toggle") == 0) {
            int status = bluetooth_toggle();
            send_signal(2);
            send_signal(3);
            return status;
        }
        if (strcmp(argv[i], "--disk-details") == 0) {
            disk_details();
            return 0;
        }
        if (strcmp(argv[i], "--calendar") == 0) {
            calendar_details();
            return 0;
        }
        if (strcmp(argv[i], "--power-details") == 0) {
            power_details();
            return 0;
        }
        if (strcmp(argv[i], "--temperature-details") == 0) {
            temperature_details();
            return 0;
        }
        if (strcmp(argv[i], "--battery-details") == 0) {
            battery_details();
            return 0;
        }
        if (strcmp(argv[i], "--signal") == 0 && i + 1 < argc) {
            char *end = NULL;
            errno = 0;
            long parsed = strtol(argv[++i], &end, 10);
            if (errno != 0 || end == argv[i] || *end != '\0' ||
                parsed < 0 || parsed > INT_MAX || !valid_signal_idx((int)parsed))
                return 1;
            int sig_idx = (int)parsed;
            send_signal(sig_idx);
            return 0;
        }
    }

    /* Register signal handlers for the main process */
    if (!setup_signal_handlers()) return 1;

    /* RAPL sampling frequency (in ticks): default, env, CLI -r N */
    uint64_t rapl_every_us = (uint64_t)RAPL_EVERY_DEFAULT * STATUS_TICK * STATUS_TICK_US;
    const char *ev = getenv("RAPL_EVERY");
    if (ev) {
        char *end = NULL;
        errno = 0;
        unsigned long long n = strtoull(ev, &end, 10);
        if (errno == 0 && end != ev && *end == '\0' && n >= 1 &&
            n <= UINT64_MAX / (STATUS_TICK * STATUS_TICK_US))
            rapl_every_us = n * STATUS_TICK * STATUS_TICK_US;
    }
    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--rapl-every") == 0) && i+1 < argc) {
            char *end = NULL;
            errno = 0;
            unsigned long long n = strtoull(argv[++i], &end, 10);
            if (errno == 0 && end != argv[i] && *end == '\0' && n >= 1 &&
                n <= UINT64_MAX / (STATUS_TICK * STATUS_TICK_US))
                rapl_every_us = n * STATUS_TICK * STATUS_TICK_US;
        }
    }

    /* CPU temp path (discover once) */
    discover_temp_paths();

    /* Discover package and optional uncore counters by domain name, not vendor paths. */
    discover_powercounters();
    prev_us_g = now_us();
    read_cpu_totals(&prev_idle_g, &prev_total_g);

    Unit units[] = {
        {
            .name = "Equalizer",
            .interval_us = MUSIC_EVERY_US,
            .update = equalizer_text,
            .signal_idx = -1
        },
        {
            .name = "Volume",
            .interval_us = VOL_EVERY * STATUS_TICK_US,
            .update = volume_text,
            .left_click = "volume-slider",
            .right_click = "pavucontrol",
            .middle_click = "pamixer -t",
            .scroll_up = "sh -c \"pamixer -i 2; dwlb-status --signal 0\"",
            .scroll_down = "sh -c \"pamixer -d 2; dwlb-status --signal 0\"",
            .signal_idx = 0
        },
        {
            .name = "Bluetooth",
            .interval_us = AIRPODS_EVERY * STATUS_TICK_US,
            .update = bluetooth_text,
            .left_click = "dwlb-status --bluetooth-toggle",
            .signal_idx = 2
        },
        {
            .name = "Airpods",
            .interval_us = AIRPODS_EVERY * STATUS_TICK_US,
            .update = airpods_text,
            .left_click = "airpods",
            .right_click = "librepods",
            .signal_idx = 3
        },
        {
            .name = "Power",
            .interval_us = rapl_every_us,
            .update = power_text,
            .left_click = "dwlb-status --power-details",
            .signal_idx = -1
        },
        {
            .name = "Gold",
            .asynchronous = true,
            .interval_us = GOLD_EVERY_US,
            .update = gold_text,
            .signal_idx = -1
        },
        {
            .name = "Weather",
            .asynchronous = true,
            .interval_us = WEATHER_EVERY_US,
            .update = weather_text,
            .signal_idx = -1
        },
        {
            .name = "Duck",
            .interval_us = 999999ull * STATUS_TICK_US,
            .update = duck_text,
            .left_click = "sh -c 'pgrep -x wmbubble >/dev/null || wmbubble &'",
            .right_click = "pkill -x wmbubble",
            .signal_idx = -1
        },
        {
            .name = "Temp",
            .interval_us = STATUS_TICK * STATUS_TICK_US,
            .update = temp_text,
            .left_click = "dwlb-status --temperature-details",
            .signal_idx = -1
        },
        {
            .name = "CPU",
            .interval_us = STATUS_TICK * STATUS_TICK_US,
            .update = cpu_load_text,
            .left_click = "tuned-profile --select",
            .right_click = "tuned-profile --notify",
            .signal_idx = -1
        },
        {
            .name = "RAM",
            .interval_us = STATUS_TICK * STATUS_TICK_US,
            .update = ram_text,
            .signal_idx = -1
        },
        {
            .name = "Disk",
            .interval_us = DISK_EVERY * STATUS_TICK_US,
            .update = disk_text,
            .left_click = "dwlb-status --disk-details",
            .signal_idx = -1
        },
        {
            .name = "Battery",
            .interval_us = BATT_EVERY * STATUS_TICK_US,
            .update = battery_text,
            .left_click = "dwlb-status --battery-details",
            .signal_idx = 4
        },
        {
            .name = "Time",
            .interval_us = TIME_EVERY * STATUS_TICK_US,
            .update = time_text,
            .left_click = "dwlb-status --calendar",
            .signal_idx = 5
        },
        {
            .name = "Theme",
            .interval_us = THEME_EVERY * STATUS_TICK_US,
            .update = theme_text,
            .left_click = "autored --slider",
            .right_click = "sh -c 'switch-theme -a; dwlb-status --signal 1'",
            .signal_idx = 1
        },
        {
            .name = "Font",
            .interval_us = 999999ull * STATUS_TICK_US,
            .update = font_text,
            .left_click = "font-cycle next",
            .scroll_up = "font-cycle next",
            .scroll_down = "font-cycle prev",
            .signal_idx = -1
        },
        {
            .name = "VPN",
            .interval_us = 20 * STATUS_TICK_US * STATUS_TICK,
            .update = vpn_text,
            .signal_idx = -1
        },
        {
            .name = "Launcher",
            .interval_us = 999999ull * STATUS_TICK_US,
            .update = launcher_text,
            .left_click = "sh -c 'fuzzel &'",
            .middle_click = "sh -c 'power &'",
            .right_click = "autored",
            .signal_idx = -1
        }
    };
    int num_units = sizeof(units) / sizeof(units[0]);

    /* Initial update */
    for (int i = 0; i < num_units; i++) {
        bool started = update_unit(&units[i]);
        units[i].next_deadline_us = now_us() + (started ? units[i].interval_us : 1000000ull);
    }

    bool dirty = true;
    sigset_t wait_mask;
    sigemptyset(&wait_mask);
    for (;;) {
        sig_atomic_t current_mask = update_mask;
        update_mask = 0;
        uint64_t current_time = now_us();

        for (int i = 0; i < num_units; i++) {
            dirty |= collect_unit(&units[i]);
            bool sig_hit = (units[i].signal_idx != -1 &&
                            (current_mask & (sig_atomic_t)(1u << units[i].signal_idx)));
            bool time_hit = current_time >= units[i].next_deadline_us;

            if (sig_hit || time_hit) {
                char previous[sizeof units[i].buffer];
                memcpy(previous, units[i].buffer, sizeof previous);
                bool started = update_unit(&units[i]);
                dirty |= memcmp(previous, units[i].buffer, sizeof previous) != 0;
                units[i].next_deadline_us = now_us() + (started ? units[i].interval_us : 1000000ull);
            }
        }

        if (dirty) {
            char bar[4096];
            size_t bar_len = 0;
            bool have_unit = false;
            for (int i = 0; i < num_units; i++) {
                char rendered[512];
                size_t rlen = 0;
                render_unit(&units[i], rendered, &rlen);
                size_t separator = have_unit ? 1 : 0;
                if (bar_len <= sizeof bar - 1 && separator <= sizeof bar - 1 - bar_len &&
                    rlen <= sizeof bar - 1 - bar_len - separator) {
                    if (have_unit) bar[bar_len++] = ' ';
                    memcpy(bar + bar_len, rendered, rlen);
                    bar_len += rlen;
                    have_unit = true;
                }
            }
            bar[bar_len] = '\0';

            printf("%s\n", bar);
            fflush(stdout);
            dirty = false;
        }

        uint64_t next_deadline = UINT64_MAX;
        current_time = now_us();
        for (int i = 0; i < num_units; ++i)
            if (units[i].next_deadline_us < next_deadline)
                next_deadline = units[i].next_deadline_us;

        uint64_t wait_us = next_deadline > current_time ? next_deadline - current_time : 0;
        struct timespec timeout = {
            .tv_sec = (time_t)(wait_us / 1000000ull),
            .tv_nsec = (long)((wait_us % 1000000ull) * 1000ull)
        };
        fd_set readers;
        FD_ZERO(&readers);
        int nfds = 0;
        for (int i = 0; i < num_units; ++i) {
            if (units[i].worker_pid <= 0) continue;
            FD_SET(units[i].worker_fd, &readers);
            if (units[i].worker_fd >= nfds) nfds = units[i].worker_fd + 1;
        }
        if (pselect(nfds, &readers, NULL, NULL, &timeout, &wait_mask) < 0 && errno != EINTR)
            return 1;
    }

    return 0;
}
