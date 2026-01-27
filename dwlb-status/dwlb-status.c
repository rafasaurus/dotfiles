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

/* Signal mapping: 0=Vol, 1=Batt, 2=Airpods, 3=Disk, 4=Time */
volatile sig_atomic_t update_flags[5] = {0};

static void handle_sig(int sig) {
    int idx = sig - SIGRTMIN;
    if (idx >= 0 && idx < 5) update_flags[idx] = 1;
}

static const double LOOP_SLEEP_SEC = 1.2; /* main refresh period */
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
        char base[256];
        snprintf(base, sizeof base, "/sys/class/hwmon/%s", de->d_name);
        char namep[256]; snprintf(namep, sizeof namep, "%s/name", base);
        FILE *nf = fopen(namep, "re");
        if (!nf) continue;
        char name[128] = {0};
        fgets(name, sizeof name, nf);
        fclose(nf);
        for (char *p = name; *p; ++p) *p = tolower(*p);
        if (strstr(name, "coretemp")) {
            for (int i = 1; i <= 32; ++i) {
                char lbl[256]; snprintf(lbl, sizeof lbl, "%s/temp%d_label", base, i);
                FILE *lf = fopen(lbl, "re");
                if (!lf) continue;
                char ltxt[128] = {0};
                fgets(ltxt, sizeof ltxt, lf);
                fclose(lf);
                if (strncmp(ltxt, "Package id 0", 12) == 0) {
                    char *res = NULL;
                    if (asprintf(&res, "%s/temp%d_input", base, i) > 0) { closedir(root); return res; }
                }
            }
            for (int i = 1; i <= 32; ++i) {
                char inp[256]; snprintf(inp, sizeof inp, "%s/temp%d_input", base, i);
                if (access(inp, R_OK) == 0) {
                    char *res = NULL;
                    if (asprintf(&res, "%s", inp) > 0) { closedir(root); return res; }
                }
            }
        }
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
    char pct_s[16] = "??", status[64] = "Unknown";
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
    char eta[16] = "";
    if (pw_uW > 0) {
        if (strcmp(status, "Discharging") == 0) {
            uint64_t t10 = (en_uWh * 10ull + pw_uW/2ull) / pw_uW;
            snprintf(eta, sizeof eta, " ~%llu.%lluh", t10/10ull, t10%10ull);
        } else if (strcmp(status, "Charging") == 0 && ef_uWh > en_uWh) {
            uint64_t rem = ef_uWh - en_uWh;
            uint64_t t10 = (rem * 10ull + pw_uW/2ull) / pw_uW;
            snprintf(eta, sizeof eta, " ~%llu.%lluh", t10/10ull, t10%10ull);
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

int main(int argc, char **argv) {
    /* Signal mode: ./dwlb-status --signal N */
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--signal") == 0 && i + 1 < argc) {
            int sig_idx = atoi(argv[++i]);
            if (sig_idx < 0 || sig_idx >= 5) return 1;

            /* Find other dwlb-status processes and signal them */
            DIR *dir = opendir("/proc");
            if (!dir) return 1;
            struct dirent *de;
            pid_t my_pid = getpid();
            while ((de = readdir(dir))) {
                if (!isdigit(de->d_name[0])) continue;
                pid_t pid = atoi(de->d_name);
                if (pid == my_pid) continue;

                char cmdpath[256];
                snprintf(cmdpath, sizeof(cmdpath), "/proc/%d/comm", pid);
                FILE *f = fopen(cmdpath, "r");
                if (f) {
                    char comm[256];
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

    /* RAPL paths */
    const char *PKG_CPU = "/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj"; /* RAPL0: CPU */
    const char *PKG_SOC = "/sys/class/powercap/intel-rapl/intel-rapl:1/energy_uj"; /* RAPL1: SoC */

    /* CPU temp path (discover once) */
    char *cpu_temp_path = find_coretemp_input_path();

    /* Baselines */
    uint64_t prev_cpu_uj = 0, prev_soc_uj = 0, prev_us = 0;
    read_u64_file(PKG_CPU, &prev_cpu_uj);
    read_u64_file(PKG_SOC, &prev_soc_uj);
    prev_us = now_us();

    uint64_t prev_idle=0, prev_total=0;
    read_cpu_totals(&prev_idle, &prev_total);

    /* Caches for slow-changing sections */
    int tick = 0;
    char vol_text_buf[64]  = "🔊 0%";
    char batt_text_buf[64] = "🔋 ??";
    char time_text_buf[64] = "📅 -- 🕒 --:--";
    char disk_text_buf[64] = "💾 --/--";
    char airpods_text_buf[64] = "🎧 ??";
    char power_str[64]     = "^fg(FFD700)⚡^fg() 0.0W 0.0W"; /* persisted; updated by RAPL cadence */

    /* HOME path */
    const char *home = getenv("HOME");
    if (!home) home = "/";

    for (;;) {
        ++tick;

        /* Signal-triggered updates */
        if (update_flags[0]) { update_flags[0] = 0; volume_text(vol_text_buf, sizeof vol_text_buf); }
        if (update_flags[1]) { update_flags[1] = 0; battery_text(batt_text_buf, sizeof batt_text_buf); }
        if (update_flags[2]) { update_flags[2] = 0; airpods_text(airpods_text_buf, sizeof airpods_text_buf); }
        if (update_flags[3]) { update_flags[3] = 0; 
            uint64_t used_b=0, tot_b=0;
            if (!read_disk_bytes(home, &used_b, &tot_b)) { read_disk_bytes("/", &used_b, &tot_b); }
            double used = (double)used_b / (1024.0*1024.0*1024.0);
            double tot  = (double)tot_b  / (1024.0*1024.0*1024.0);
            snprintf(disk_text_buf, sizeof disk_text_buf, "💾 %.0f/%.0fMib", used, tot);
        }
        if (update_flags[4]) { update_flags[4] = 0;
            time_t t = time(NULL);
            struct tm tm; localtime_r(&t, &tm);
            char date[16], timebuf[16];
            strftime(date, sizeof date, "%Y-%m-%d", &tm);
            strftime(timebuf, sizeof timebuf, "%H:%M", &tm);
            snprintf(time_text_buf, sizeof time_text_buf, "📅 %s 🕒 %s", date, timebuf);
        }

        /* Volume (left-click=mute, right-click=pavucontrol, scroll ±5%) */
        if (tick % VOL_EVERY == 1) {
            volume_text(vol_text_buf, sizeof vol_text_buf);
        }
        char vol_block[128];
        snprintf(vol_block, sizeof vol_block,
                 "^lm(pamixer -t)^rm(pavucontrol)^su(pamixer -i 5)^sd(pamixer -d 5)%s^sd()^su()^rm()^lm()",
                 vol_text_buf);

        /* Airpods (left-click=toggle connection, right-click=librepods) */
        if (tick % AIRPODS_EVERY == 1) {
            airpods_text(airpods_text_buf, sizeof airpods_text_buf);
        }
        char airpods_block[128];
        snprintf(airpods_block, sizeof airpods_block,
                 "^lm(airpods)^rm(librepods)%s^rm()^lm()",
                 airpods_text_buf);

        char duck_block[128];
        snprintf(duck_block, sizeof duck_block,
                 "^lm(sh -c 'pgrep -x wmbubble >/dev/null || wmbubble &')^rm(pkill -x wmbubble)🦆^rm()^lm()");

        char launcher_block[128];
        snprintf(launcher_block, sizeof launcher_block,
                 "^lm(sh -c 'fuzzel &')🐧^lm()");

        /* RAPL power: first SoC (RAPL1), then CPU (RAPL0) — update every rapl_every ticks */
        if (tick % rapl_every == 1) {
            uint64_t cur_cpu_uj=0, cur_soc_uj=0, cur_us=now_us();
            read_u64_file(PKG_CPU, &cur_cpu_uj);
            read_u64_file(PKG_SOC, &cur_soc_uj);
            uint64_t dus = (cur_us > prev_us) ? (cur_us - prev_us) : 1;
            uint64_t d_cpu = (cur_cpu_uj > prev_cpu_uj) ? (cur_cpu_uj - prev_cpu_uj) : 0;
            uint64_t d_soc = (cur_soc_uj > prev_soc_uj) ? (cur_soc_uj - prev_soc_uj) : 0;
            uint64_t cpu_w10 = (d_cpu * 10ull + dus/2ull) / dus;
            uint64_t soc_w10 = (d_soc * 10ull + dus/2ull) / dus;
            snprintf(power_str, sizeof power_str,
                     "^fg(FFD700)⚡^fg() %llu.%lluW %llu.%lluW",
                     (unsigned long long)(soc_w10/10ull), (unsigned long long)(soc_w10%10ull),
                     (unsigned long long)(cpu_w10/10ull), (unsigned long long)(cpu_w10%10ull));
            prev_cpu_uj = cur_cpu_uj; prev_soc_uj = cur_soc_uj; prev_us = cur_us;
        }

        /* CPU temperature */
        char temp_str[48] = "🔥 ?°C";
        if (cpu_temp_path && path_readable(cpu_temp_path)) {
            uint64_t milli = 0;
            if (read_u64_file(cpu_temp_path, &milli)) {
                snprintf(temp_str, sizeof temp_str, "🔥 %llu°C", (unsigned long long)(milli/1000ull));
            }
        }

        /* CPU load % (clickable) */
        uint64_t cur_idle=0, cur_total=0;
        read_cpu_totals(&cur_idle, &cur_total);
        uint64_t d_tot = (cur_total > prev_total) ? (cur_total - prev_total) : 0;
        uint64_t d_idl = (cur_idle  > prev_idle ) ? (cur_idle  - prev_idle ) : 0;
        prev_total = cur_total; prev_idle = cur_idle;
        unsigned cpu_pct = d_tot ? (unsigned)((d_tot - d_idl) * 100ull / d_tot) : 0;
        char cpu_block[64];
        snprintf(cpu_block, sizeof cpu_block, "^lm(cpupower-gui)📈 %u%%^lm()", cpu_pct);

        /* RAM (GiB) */
        char ram_str[64] = "🧠 --/--GiB";
        uint64_t mem_tot=0, mem_avl=0;
        if (read_mem_kib(&mem_tot, &mem_avl)) {
            uint64_t used_kib = mem_tot - mem_avl;
            double used_g = (double)used_kib / 1048576.0;
            double tot_g  = (double)mem_tot   / 1048576.0;
            snprintf(ram_str, sizeof ram_str, "🧠 %.1f/%.1fGiB", used_g, tot_g);
        }

        /* Disk (HOME) – very infrequent */
        if (tick % DISK_EVERY == 1) {
            uint64_t used_b=0, tot_b=0;
            if (!read_disk_bytes(home, &used_b, &tot_b)) { read_disk_bytes("/", &used_b, &tot_b); }
            double used = (double)used_b / (1024.0*1024.0*1024.0);
            double tot  = (double)tot_b  / (1024.0*1024.0*1024.0);
            snprintf(disk_text_buf, sizeof disk_text_buf, "💾 %.0f/%.0fMib", used, tot);
        }

        /* Battery – moderately infrequent */
        if (tick % BATT_EVERY == 1) {
            battery_text(batt_text_buf, sizeof batt_text_buf);
        }

        /* Date/time – infrequent */
        if (tick % TIME_EVERY == 1) {
            time_t t = time(NULL);
            struct tm tm; localtime_r(&t, &tm);
            char date[16], timebuf[16];
            strftime(date, sizeof date, "%Y-%m-%d", &tm);
            strftime(timebuf, sizeof timebuf, "%H:%M", &tm);
            snprintf(time_text_buf, sizeof time_text_buf, "📅 %s 🕒 %s", date, timebuf);
        }

        /* Compose & send (date/time at end) */
        char bar[1400];
        snprintf(bar, sizeof bar,
            "%s %s %s %s %s %s %s %s %s %s %s",
            vol_block, airpods_block, power_str, duck_block, temp_str, cpu_block, ram_str, disk_text_buf, batt_text_buf, time_text_buf, launcher_block);

        printf("%s\n", bar);
        fflush(stdout);

        /* Sleep - interrupted by signals */
        struct timespec req = { (time_t)LOOP_SLEEP_SEC, (long)((LOOP_SLEEP_SEC - (time_t)LOOP_SLEEP_SEC) * 1e9) };
        if (nanosleep(&req, NULL) == -1 && errno == EINTR) {
            /* Signal received, loop back immediately to process flags and redraw */
            continue;
        }
    }

    /* never reached */
    /* free(cpu_temp_path); */
    return 0;
}

