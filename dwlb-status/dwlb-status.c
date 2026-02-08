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

/* Signal mapping: bitmask for 0=Vol, 1=Batt, 2=Airpods, 3=Disk, 4=Time */
volatile sig_atomic_t update_mask = 0;

static void handle_sig(int sig) {
    int idx = sig - SIGRTMIN;
    if (idx >= 0 && idx < 5) update_mask |= (1 << idx);
}

static const double LOOP_SLEEP_SEC = 2.0; /* main refresh period */
static const int    VOL_EVERY      = 3;   /* poll volume every N ticks */
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

/* Scan hwmon for coretemp "Package id 0" temp input; fallback to any temp*_input */
static char *find_coretemp_input_path(void) {
    DIR *root = opendir("/sys/class/hwmon");
    if (!root) return NULL;
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
                        if (strncmp(ltxt, "Package id 0", 12) == 0) {
                            char *res = NULL;
                            if (asprintf(&res, "%s/temp%d_input", base, i) > 0) { fclose(lf); fclose(nf); closedir(root); return res; }
                        }
                    }
                    fclose(lf);
                }
                for (int i = 1; i <= 32; ++i) {
                    char inp[80]; 
                    if (snprintf(inp, sizeof inp, "%s/temp%d_input", base, i) >= (int)sizeof(inp)) continue;
                    if (access(inp, R_OK) == 0) {
                        char *res = strdup(inp);
                        if (res) { fclose(nf); closedir(root); return res; }
                    }
                }
            }
        }
        fclose(nf);
    }
    closedir(root);
    return NULL;
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

static void disk_text(char *out, size_t outsz) {
    const char *home = getenv("HOME");
    if (!home) home = "/";
    uint64_t used_b=0, tot_b=0;
    if (!read_disk_bytes(home, &used_b, &tot_b)) { read_disk_bytes("/", &used_b, &tot_b); }
    double used = (double)used_b / (1024.0*1024.0*1024.0);
    double tot  = (double)tot_b  / (1024.0*1024.0*1024.0);
    snprintf(out, outsz, "💾 %.0f/%.0fMib", used, tot);
}

static void duck_text(char *out, size_t outsz) {
    snprintf(out, outsz, "🦆");
}

static void launcher_text(char *out, size_t outsz) {
    snprintf(out, outsz, "🐧");
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

static char *cpu_temp_path_g = NULL;
static void temp_text(char *out, size_t outsz) {
    if (cpu_temp_path_g && path_readable(cpu_temp_path_g)) {
        uint64_t milli = 0;
        if (read_u64_file(cpu_temp_path_g, &milli)) {
            snprintf(out, outsz, "🔥 %llu°C", (unsigned long long)(milli/1000ull));
            return;
        }
    }
    snprintf(out, outsz, "🔥 ?°C");
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

static uint64_t prev_cpu_uj_g = 0, prev_soc_uj_g = 0, prev_us_g = 0;
static void power_text(char *out, size_t outsz) {
    const char *PKG_CPU = "/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj";
    const char *PKG_SOC = "/sys/class/powercap/intel-rapl/intel-rapl:1/energy_uj";
    if (access(PKG_CPU, R_OK) != 0) {
        snprintf(out, outsz, "^fg(FFD700)⚡^fg() --W --W");
        return;
    }
    uint64_t cur_cpu_uj=0, cur_soc_uj=0, cur_us=now_us();
    read_u64_file(PKG_CPU, &cur_cpu_uj);
    read_u64_file(PKG_SOC, &cur_soc_uj);
    uint64_t dus = (cur_us > prev_us_g) ? (cur_us - prev_us_g) : 1;
    uint64_t d_cpu = (cur_cpu_uj > prev_cpu_uj_g) ? (cur_cpu_uj - prev_cpu_uj_g) : 0;
    uint64_t d_soc = (cur_soc_uj > prev_soc_uj_g) ? (cur_soc_uj - prev_soc_uj_g) : 0;
    uint64_t cpu_w10 = (d_cpu * 10ull + dus/2ull) / dus;
    uint64_t soc_w10 = (d_soc * 10ull + dus/2ull) / dus;
    snprintf(out, outsz, "^fg(FFD700)⚡^fg() %llu.%llu/%llu.%lluW",
             (unsigned long long)(soc_w10/10ull), (unsigned long long)(soc_w10%10ull),
             (unsigned long long)(cpu_w10/10ull), (unsigned long long)(cpu_w10%10ull));
    prev_cpu_uj_g = cur_cpu_uj; prev_soc_uj_g = cur_soc_uj; prev_us_g = cur_us;
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
        if (strcmp(argv[i], "--signal") == 0 && i + 1 < argc) {
            int sig_idx = atoi(argv[++i]);
            if (sig_idx < 0 || sig_idx >= 5) return 1;
            send_signal(sig_idx);
            return 0;
        }
    }

    /* Register signal handlers for the main process */
    for (int i = 0; i < 5; ++i) {
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
    cpu_temp_path_g = find_coretemp_input_path();

    /* Baselines */
    read_u64_file("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj", &prev_cpu_uj_g);
    read_u64_file("/sys/class/powercap/intel-rapl/intel-rapl:1/energy_uj", &prev_soc_uj_g);
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
            .signal_idx = 2
        },
        {
            .name = "Power",
            .interval = rapl_every,
            .update = power_text,
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
            .signal_idx = -1
        },
        {
            .name = "CPU",
            .interval = 1,
            .update = cpu_load_text,
            .left_click = "cpupower-gui",
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
            .signal_idx = 3
        },
        {
            .name = "Battery",
            .interval = BATT_EVERY,
            .update = battery_text,
            .signal_idx = 1
        },
        {
            .name = "Time",
            .interval = TIME_EVERY,
            .update = time_text,
            .signal_idx = 4
        },
        {
            .name = "Launcher",
            .interval = 999999,
            .update = launcher_text,
            .left_click = "sh -c 'fuzzel &'",
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
    for (;;) {
        tick++;

        sig_atomic_t current_mask = update_mask;
        if (__builtin_expect(current_mask != 0, 0)) {
            update_mask = 0;
        }

        for (int i = 0; i < num_units; i++) {
            bool update_needed = false;
            if (__builtin_expect(units[i].signal_idx != -1, 0)) {
                if (current_mask & (1 << units[i].signal_idx)) {
                    update_needed = true;
                }
            }
            if (__builtin_expect(tick % units[i].interval == 1, 0)) {
                update_needed = true;
            }

            if (__builtin_expect(update_needed, 0)) {
                units[i].update(units[i].buffer, sizeof(units[i].buffer));
            }
        }

        char bar[1024];
        char *p = bar;
        char *end = bar + sizeof(bar);
        for (int i = 0; i < num_units; i++) {
            char rendered[512];
            size_t rlen = 0;
            render_unit(&units[i], rendered, &rlen);
            if (__builtin_expect(p + rlen + 2 < end, 1)) {
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

        struct timespec req = { (time_t)LOOP_SLEEP_SEC, (long)((LOOP_SLEEP_SEC - (time_t)LOOP_SLEEP_SEC) * 1e9) };
        if (nanosleep(&req, NULL) == -1 && errno == EINTR) {
            continue;
        }
    }

    return 0;
}

