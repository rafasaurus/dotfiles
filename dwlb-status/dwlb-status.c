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
    int interval;
    int signal_idx; /* -1 if no signal */
    char buffer[128];
} __attribute__((aligned(64))) Unit;

static void notify_text(const char *title, const char *message);

/* Signal mapping: bitmask for units */
volatile sig_atomic_t update_mask = 0;

static void handle_sig(int sig) {
    int idx = sig - SIGRTMIN;
    if (idx >= 0 && idx < 32) update_mask |= (1 << idx);
}

static const double LOOP_SLEEP_SEC = 1.2; /* main refresh period */
static const int    THEME_EVERY    = 100; /* poll theme every N ticks */
static const int    VOL_EVERY      = 10;  /* poll volume every N ticks */
static const int    BATT_EVERY     = 20;  /* poll battery every N ticks */
static const int    TIME_EVERY     = 8;   /* poll date/time every N ticks */
static const int    DISK_EVERY     = 120; /* poll disk every N ticks */
static const int    AIRPODS_EVERY  = 3;  /* poll airpods every N ticks (~10 seconds) */

/* ---------- Helpers ---------- */
static inline uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
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

static bool read_first_existing_u64(const char *a, const char *b, uint64_t *out) {
    if (a && read_u64_file(a, out)) return true;
    if (b && read_u64_file(b, out)) return true;
    return false;
}

static bool path_readable(const char *p) { return p && access(p, R_OK) == 0; }

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
static PowerCounter uncore_counters[MAX_POWER_COUNTERS];
static size_t package_count = 0, uncore_count = 0;
static bool power_first_render = true;
static char power_provider[64] = "none";

static bool read_text_file(const char *path, char *out, size_t outsz) {
    FILE *f = fopen(path, "re");
    if (!f) return false;
    bool ok = fgets(out, outsz, f) != NULL;
    fclose(f);
    if (ok) out[strcspn(out, "\n")] = 0;
    return ok;
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

static void discover_powercounters(void) {
    DIR *root = opendir("/sys/class/powercap");
    if (!root) return;
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
    if (!preferred[0]) return;
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
        else if (strcmp(name, "uncore") == 0)
            add_power_counter(uncore_counters, &uncore_count, path, name);
    }
    closedir(root);
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
                char base[64];
                snprintf(base, sizeof base, "/sys/class/hwmon/%s", de->d_name);
                for (int i = 1; i <= 32; ++i) {
                    char inp[80];
                    snprintf(inp, sizeof inp, "%s/temp%d_input", base, i);
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

/* Battery snapshot: returns text like "🔌 92% ~1.8h" or "🔋 85%" */
static void battery_text(char *out, size_t outsz) {
    const char *B = "/sys/class/power_supply/BAT0";
    char cap_p[256], stat_p[256];
    snprintf(cap_p, sizeof cap_p,  "%s/capacity", B);
    snprintf(stat_p, sizeof stat_p, "%s/status",   B);
    if (access(B, R_OK) != 0) { snprintf(out, outsz, "🔋 ??"); return; }

    /* percent */
    char pct_s[8] = "??", status[16] = "Unknown";
    FILE *f = fopen(cap_p, "re"); if (f) { if (fgets(pct_s, sizeof pct_s, f)) {} fclose(f); }
    f = fopen(stat_p, "re"); if (f) { if (fgets(status, sizeof status, f)) {} fclose(f); }
    pct_s[strcspn(pct_s, "\n")] = 0; status[strcspn(status, "\n")] = 0;

    /* energy/power (uWh/uW) with fallbacks */
    uint64_t en_uWh=0, ef_uWh=0, pw_uW=0;
    if (!read_first_existing_u64("/sys/class/power_supply/BAT0/energy_now",
                                 "/sys/class/power_supply/BAT0/energy_now_uwh", &en_uWh)) en_uWh=0;
    if (!read_first_existing_u64("/sys/class/power_supply/BAT0/energy_full",
                                 "/sys/class/power_supply/BAT0/energy_full_uwh", &ef_uWh)) ef_uWh=0;
    if (!read_u64_file("/sys/class/power_supply/BAT0/power_now", &pw_uW)) pw_uW=0;

    if ((en_uWh == 0 || ef_uWh == 0) || pw_uW == 0) {
        uint64_t qn=0, qf=0, vv=0, ia=0;
        read_u64_file("/sys/class/power_supply/BAT0/charge_now", &qn);
        read_u64_file("/sys/class/power_supply/BAT0/charge_full", &qf);
        read_u64_file("/sys/class/power_supply/BAT0/voltage_now", &vv);
        if (en_uWh == 0 && qn > 0 && vv > 0) en_uWh = (qn * vv) / 1000000ull;
        if (ef_uWh == 0 && qf > 0 && vv > 0) ef_uWh = (qf * vv) / 1000000ull;
        if (pw_uW == 0) {
            read_u64_file("/sys/class/power_supply/BAT0/current_now", &ia);
            if (ia > 0 && vv > 0) pw_uW = (ia * vv) / 1000000ull;
        }
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

    snprintf(out, outsz, "%s %s%%%s", icon, pct_s, eta);
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
    char message[1024];
    size_t len = 0;
    uint64_t pkg_milli = 0;
    if (cpu_temp_path_g && read_u64_file(cpu_temp_path_g, &pkg_milli))
        len += (size_t)snprintf(message + len, sizeof message - len, "Package: %llu C\n",
                                 (unsigned long long)(pkg_milli / 1000ull));
    else
        len += (size_t)snprintf(message + len, sizeof message - len, "Package: unavailable\n");

    for (size_t i = 0; i < cpu_core_count_g && len < sizeof message; ++i) {
        uint64_t core_milli = 0;
        if (read_u64_file(cpu_core_paths_g[i], &core_milli))
            len += (size_t)snprintf(message + len, sizeof message - len, "Core %zu: %llu C\n",
                                     i, (unsigned long long)(core_milli / 1000ull));
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
        snprintf(out, outsz, "^fg(FFD700)⚡^fg() --/--W");
        return;
    }
    if (power_first_render) {
        power_first_render = false;
        snprintf(out, outsz, "^fg(FFD700)⚡^fg() --/--W");
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
        snprintf(message, sizeof message,
                 "No package energy counter is available in /sys/class/powercap.");
    } else {
        uint64_t package_before = 0, package_after = 0, uncore_before = 0, uncore_after = 0;
        bool package_ok = power_group_energy(package_counters, package_count, &package_before);
        bool uncore_ok = uncore_count > 0 && power_group_energy(uncore_counters, uncore_count, &uncore_before);
        struct timespec req = { 1, 0 };
        nanosleep(&req, NULL);
        package_ok = package_ok && power_group_energy(package_counters, package_count, &package_after);
        uncore_ok = uncore_ok && power_group_energy(uncore_counters, uncore_count, &uncore_after);
        if (package_ok) {
            double package_w = (double)(package_after - package_before) / 1000000.0;
            if (uncore_ok) {
                double uncore_w = (double)(uncore_after - uncore_before) / 1000000.0;
                snprintf(message, sizeof message, "Uncore: %.1f W\nPackage: %.1f W", uncore_w, package_w);
            } else {
                snprintf(message, sizeof message, "Uncore: unavailable\nPackage: %.1f W", package_w);
            }
        } else {
            snprintf(message, sizeof message, "Unable to read power counters.");
        }
    }
    notify_text("CPU energy", message);
}

static void vpn_text(char *out, size_t outsz) {
    FILE *fp = popen("timeout 1 nmcli -t -f NAME,TYPE connection show --active 2>/dev/null | awk -F: '$2 == \"vpn\" { print $1; exit }'", "r");
    if (fp && fgets(out, outsz, fp)) {
        out[strcspn(out, "\n")] = 0;
        char text[128];
        snprintf(text, sizeof text, "🛡 %s", out);
        snprintf(out, outsz, "%s", text);
    } else {
        out[0] = 0;
    }
    if (fp) pclose(fp);
}

static inline void wrap_tag(char *buf, size_t *len, const char *tag, size_t tag_len, const char *cmd) {
    if (__builtin_expect(cmd != NULL, 0)) {
        size_t cmd_len = strlen(cmd);
        char inner[512];
        char *p = inner;

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

        size_t new_len = p - inner;
        if (__builtin_expect(new_len < 512, 1)) {
            memcpy(buf, inner, new_len + 1);
            *len = new_len;
        }
    }
}

static void render_unit(const Unit *u, char *out, size_t *out_len) {
    char temp[512];
    size_t blen = strlen(u->buffer);
    if (__builtin_expect(blen >= sizeof(temp), 0)) return;
    memcpy(temp, u->buffer, blen + 1);

    wrap_tag(temp, &blen, "lm", 2, u->left_click);
    wrap_tag(temp, &blen, "mm", 2, u->middle_click);
    wrap_tag(temp, &blen, "rm", 2, u->right_click);
    wrap_tag(temp, &blen, "us", 2, u->scroll_up);
    wrap_tag(temp, &blen, "ds", 2, u->scroll_down);

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
    /* Signal mode: ./dwlb-status --signal N */
    for (int i = 1; i < argc; ++i) {
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
        if (strcmp(argv[i], "--signal") == 0 && i + 1 < argc) {
            int sig_idx = atoi(argv[++i]);
            if (sig_idx < 0 || sig_idx >= 32) return 1;
            send_signal(sig_idx);
            return 0;
        }
    }

    /* Register signal handlers for the main process */
    for (int i = 0; i < 32; ++i) {
        signal(SIGRTMIN + i, handle_sig);
    }

    /* RAPL sampling frequency (in ticks): default, env, CLI -r N */
    int rapl_every = RAPL_EVERY_DEFAULT;
    const char *ev = getenv("RAPL_EVERY");
    if (ev) {
        int n = atoi(ev);
        if (n >= 1) rapl_every = n;
    }
    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--rapl-every") == 0) && i+1 < argc) {
            int n = atoi(argv[++i]);
            if (n >= 1) rapl_every = n;
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
            .name = "Volume",
            .interval = VOL_EVERY,
            .update = volume_text,
            .left_click = "pamixer -t",
            .right_click = "pavucontrol",
            .scroll_up = "sh -c \"pamixer -i 2; dwlb-status --signal 0\"",
            .scroll_down = "sh -c \"pamixer -d 2; dwlb-status --signal 0\"",
            .signal_idx = 0
        },
        {
            .name = "Airpods",
            .interval = AIRPODS_EVERY,
            .update = airpods_text,
            .left_click = "airpods",
            .right_click = "librepods",
            .signal_idx = 3
        },
        {
            .name = "Power",
            .interval = rapl_every,
            .update = power_text,
            .left_click = "dwlb-status --power-details",
            .signal_idx = -1
        },
        {
            .name = "Duck",
            .interval = 999999,
            .update = duck_text,
            .left_click = "sh -c 'pgrep -x wmbubble >/dev/null || wmbubble &'",
            .right_click = "pkill -x wmbubble",
            .signal_idx = -1
        },
        {
            .name = "Temp",
            .interval = 1,
            .update = temp_text,
            .left_click = "dwlb-status --temperature-details",
            .signal_idx = -1
        },
        {
            .name = "CPU",
            .interval = 1,
            .update = cpu_load_text,
            .left_click = "tuned-profile --select",
            .right_click = "tuned-profile --notify",
            .signal_idx = -1
        },
        {
            .name = "RAM",
            .interval = 1,
            .update = ram_text,
            .signal_idx = -1
        },
        {
            .name = "Disk",
            .interval = DISK_EVERY,
            .update = disk_text,
            .left_click = "dwlb-status --disk-details",
            .signal_idx = -1
        },
        {
            .name = "Battery",
            .interval = BATT_EVERY,
            .update = battery_text,
            .signal_idx = 4
        },
        {
            .name = "Time",
            .interval = TIME_EVERY,
            .update = time_text,
            .left_click = "dwlb-status --calendar",
            .signal_idx = 5
        },
        {
            .name = "Theme",
            .interval = THEME_EVERY,
            .update = theme_text,
            .left_click = "sh -c 'switch-theme -a; dwlb-status --signal 1'",
            .signal_idx = 1
        },
        {
            .name = "Font",
            .interval = 999999,
            .update = font_text,
            .left_click = "font-cycle next",
            .scroll_up = "font-cycle next",
            .scroll_down = "font-cycle prev",
            .signal_idx = -1
        },
        {
            .name = "VPN",
            .interval = 20,
            .update = vpn_text,
            .signal_idx = -1
        },
        {
            .name = "Launcher",
            .interval = 999999,
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
        units[i].update(units[i].buffer, sizeof(units[i].buffer));
    }

    int tick = 0;
    bool dirty = true;
    for (;;) {
        sig_atomic_t current_mask = update_mask;
        update_mask = 0;

        if (current_mask == 0) tick++;

        for (int i = 0; i < num_units; i++) {
            bool sig_hit = (units[i].signal_idx != -1 && (current_mask & (1 << units[i].signal_idx)));
            bool time_hit = (current_mask == 0 && tick % units[i].interval == 0);

            if (sig_hit || time_hit) {
                units[i].update(units[i].buffer, sizeof(units[i].buffer));
                dirty = true;
            }
        }

        if (dirty) {
            char bar[1024];
            char *p = bar;
            char *end = bar + sizeof(bar);
            for (int i = 0; i < num_units; i++) {
                char rendered[512];
                size_t rlen = 0;
                render_unit(&units[i], rendered, &rlen);
                if (p + rlen + 2 < end) {
                    memcpy(p, rendered, rlen);
                    p += rlen;
                    if (i < num_units - 1) {
                        *p++ = ' ';
                    }
                }
            }
            *p = '\0';

            printf("%s\n", bar);
            fflush(stdout);
            dirty = false;
        }

        struct timespec req = { (time_t)LOOP_SLEEP_SEC, (long)((LOOP_SLEEP_SEC - (time_t)LOOP_SLEEP_SEC) * 1e9) };
        if (nanosleep(&req, NULL) == -1 && errno == EINTR) {
            continue;
        }
    }

    return 0;
}
