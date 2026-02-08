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
#include <fcntl.h>
#include <inttypes.h>

#define LOOP_DELAY_SEC 1.2
#define RAPL_DEFAULT   2

/* Data Structures */

typedef struct {
    void (*update)(void *ctx, char *buf, size_t len);
    void *ctx;
    const char *label;
    const char *onLeft;
    const char *onMiddle;
    const char *onRight;
    const char *onScrollUp;
    const char *onScrollDown;
    int interval;
    int signalId;
    char buffer[128];
} __attribute__((aligned(64))) Module;

/* Globals */

volatile sig_atomic_t signalMask = 0;
static char *cpuPkgPath = NULL;
static char *cpuCorePath = NULL;
static uint64_t prevCpuUj = 0;
static uint64_t prevSocUj = 0;
static uint64_t prevTimeUs = 0;
static uint64_t prevCpuIdle = 0;
static uint64_t prevCpuTotal = 0;

/* Core Helpers */

static void handleSig(int sig) {
    int idx = sig - SIGRTMIN;
    if (idx >= 0 && idx < 32) signalMask |= (1 << idx);
}

static inline uint64_t getNowUs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

static bool readFileU64(const char *path, uint64_t *out) {
    FILE *f = fopen(path, "re");
    if (!f) return false;
    char buf[64];
    bool ok = fgets(buf, sizeof(buf), f) != NULL;
    fclose(f);
    if (!ok) return false;
    char *end = NULL;
    errno = 0;
    uint64_t v = strtoull(buf, &end, 10);
    if (errno != 0) return false;
    *out = v;
    return true;
}

static bool readFdU64(int fd, uint64_t *out) {
    if (fd < 0) return false;
    char buf[64];
    lseek(fd, 0, SEEK_SET);
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) return false;
    buf[n] = 0;
    char *end = NULL;
    errno = 0;
    uint64_t v = strtoull(buf, &end, 10);
    if (errno != 0) return false;
    *out = v;
    return true;
}

static bool readFirstU64(const char *p1, const char *p2, uint64_t *out) {
    return (p1 && readFileU64(p1, out)) || (p2 && readFileU64(p2, out));
}

static void runCmd(const char *cmd, char *out, size_t sz) {
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(out, sz, fp)) out[strcspn(out, "\n")] = 0;
        pclose(fp);
    }
}

static inline void applyTag(char *buf, size_t *len, const char *tag, const char *cmd) {
    if (!cmd) return;
    char inner[512];
    int w = snprintf(inner, sizeof(inner), "^%s(%s)%.*s^%s()", tag, cmd, (int)*len, buf, tag);
    if (w > 0 && w < (int)sizeof(inner)) {
        memcpy(buf, inner, w + 1);
        *len = w;
    }
}

static void buildLine(Module *modules, int count, char *line, size_t size) {
    char *p = line;
    size_t rem = size;
    
    for (int i = 0; i < count; i++) {
        char seg[512];
        size_t segLen = strlen(modules[i].buffer);
        memcpy(seg, modules[i].buffer, segLen + 1);
        
        applyTag(seg, &segLen, "lm", modules[i].onLeft);
        applyTag(seg, &segLen, "mm", modules[i].onMiddle);
        applyTag(seg, &segLen, "rm", modules[i].onRight);
        applyTag(seg, &segLen, "us", modules[i].onScrollUp);
        applyTag(seg, &segLen, "ds", modules[i].onScrollDown);

        if (rem > segLen + 1) {
            memcpy(p, seg, segLen);
            p += segLen;
            if (i < count - 1) {
                *p++ = ' ';
                rem--;
            }
            rem -= segLen;
        }
    }
    *p = 0;
}

/* Initialization */

static void initHwmon(void) {
    DIR *root = opendir("/sys/class/hwmon");
    if (!root) return;
    struct dirent *de;
    
    while ((de = readdir(root))) {
        if (strncmp(de->d_name, "hwmon", 5) != 0) continue;
        char base[64];
        snprintf(base, sizeof(base), "/sys/class/hwmon/%s", de->d_name);

        for (int i = 1; i <= 32; ++i) {
            char lblPath[80], label[32] = {0};
            snprintf(lblPath, sizeof(lblPath), "%s/temp%d_label", base, i);
            FILE *f = fopen(lblPath, "re");
            if (!f) continue;
            if (fgets(label, sizeof(label), f)) {
                if (!cpuPkgPath && strncmp(label, "Package id 0", 12) == 0)
                    asprintf(&cpuPkgPath, "%s/temp%d_input", base, i);
                else if (!cpuCorePath && strncmp(label, "CPU", 3) == 0)
                    asprintf(&cpuCorePath, "%s/temp%d_input", base, i);
            }
            fclose(f);
        }
    }
    closedir(root);

    if (!cpuPkgPath) {
        root = opendir("/sys/class/hwmon");
        if (root) {
            while ((de = readdir(root))) {
                if (strncmp(de->d_name, "hwmon", 5) != 0) continue;
                char base[64], nameP[80], name[32] = {0};
                snprintf(base, sizeof(base), "/sys/class/hwmon/%s", de->d_name);
                snprintf(nameP, sizeof(nameP), "%s/name", base);
                
                FILE *f = fopen(nameP, "re");
                if (!f) continue;
                if (fgets(name, sizeof(name), f) && strstr(name, "coretemp")) {
                    for (int i = 1; i <= 32; ++i) {
                        char input[80];
                        snprintf(input, sizeof(input), "%s/temp%d_input", base, i);
                        if (access(input, R_OK) == 0) {
                            cpuPkgPath = strdup(input);
                            break;
                        }
                    }
                }
                fclose(f);
                if (cpuPkgPath) break;
            }
            closedir(root);
        }
    }
}

/* Module Contexts */

typedef struct {
    int fd_pkg;
    int fd_core;
} TempCtx;

typedef struct {
    int fd_stat;
} CpuCtx;

typedef struct {
    int fd_mem;
} RamCtx;

typedef struct {
    int fd_rapl0;
    int fd_rapl1;
} PowerCtx;

typedef struct {
    int fd_cap;
    int fd_stat;
    int fd_enNow;
    int fd_enFull;
    int fd_pwr;
} BatCtx;

/* Module Updaters */

static void updateVolume(void *ctx, char *out, size_t sz) {
    (void)ctx;
    char buf[64] = {0};
    bool mute = false;
    int vol = 0;

    FILE *fp = popen("pamixer --get-mute --get-volume", "r");
    if (fp) {
        if (fgets(buf, sizeof(buf), fp)) {
            char *space = strchr(buf, ' ');
            if (space) {
                *space = 0;
                mute = (strcmp(buf, "true") == 0);
                vol = atoi(space + 1);
            } else {
                /* Fallback if pamixer doesn't support combined output or format is different */
                mute = (strncmp(buf, "true", 4) == 0);
                if (fgets(buf, sizeof(buf), fp)) vol = atoi(buf);
            }
        }
        pclose(fp);
    }

    if (mute) snprintf(out, sz, "🔇 mute");
    else if (vol < 33) snprintf(out, sz, "🔈 %d%%", vol);
    else if (vol <= 66) snprintf(out, sz, "🔉 %d%%", vol);
    else snprintf(out, sz, "🔊 %d%%", vol);
}

static void updateAirpods(void *ctx, char *out, size_t sz) {
    (void)ctx;
    FILE *fp = popen("timeout 2 airpods -s 2>/dev/null", "r");
    if (!fp) { snprintf(out, sz, "🎧 ??"); return; }
    int status = pclose(fp);
    if (WIFEXITED(status)) {
        snprintf(out, sz, WEXITSTATUS(status) == 1 ? "🎧 ^fg(00FF00)✓^fg()" : "🎧 ^fg(FF5555)x^fg()");
    } else {
        snprintf(out, sz, "🎧 ??");
    }
}

static void updateTime(void *ctx, char *out, size_t sz) {
    (void)ctx;
    time_t t = time(NULL);
    struct tm tm; 
    localtime_r(&t, &tm);
    char d[16], tb[16];
    strftime(d, sizeof(d), "%d-%m-%Y", &tm);
    strftime(tb, sizeof(tb), "%H:%M", &tm);
    snprintf(out, sz, "📅 %s 🕒 %s", d, tb);
}

static void updateTheme(void *ctx, char *out, size_t sz) {
    (void)ctx;
    snprintf(out, sz, system("switch-theme -r >/dev/null 2>&1") == 0 ? "🌘" : "🌖");
}

static void updateDisk(void *ctx, char *out, size_t sz) {
    (void)ctx;
    const char *path = getenv("HOME") ? getenv("HOME") : "/";
    struct statvfs v;
    if (statvfs(path, &v) != 0 && statvfs("/", &v) != 0) { snprintf(out, sz, "💾 err"); return; }
    
    double total = (double)(v.f_frsize * v.f_blocks) / (1024.0*1024.0*1024.0);
    double avail = (double)(v.f_frsize * v.f_bavail) / (1024.0*1024.0*1024.0);
    snprintf(out, sz, "💾 %.0f/%.0fMib", total - avail, total);
}

static void updateBattery(void *ctx, char *out, size_t sz) {
    BatCtx *b = (BatCtx *)ctx;
    uint64_t enNow=0, enFull=0, pwr=0, pct=0;
    char status[16]="Unknown";

    if (!readFdU64(b->fd_cap, &pct)) pct = 0;
    
    if (b->fd_stat >= 0) {
        lseek(b->fd_stat, 0, SEEK_SET);
        ssize_t n = read(b->fd_stat, status, sizeof(status)-1);
        if (n > 0) {
            status[n] = 0;
            char *nl = strchr(status, '\n');
            if (nl) *nl = 0;
        }
    }

    readFdU64(b->fd_enNow, &enNow);
    readFdU64(b->fd_enFull, &enFull);
    readFdU64(b->fd_pwr, &pwr);

    char eta[32] = "";
    if (pwr > 0) {
        uint64_t hrs = 0;
        if (status[0] == 'D') /* Discharging */
            hrs = (enNow * 10ull + pwr/2ull) / pwr;
        else if (status[0] == 'C' && enFull > enNow) /* Charging */
            hrs = ((enFull - enNow) * 10ull + pwr/2ull) / pwr;
        
        if (hrs) snprintf(eta, sizeof(eta), " ~%" PRIu64 ".%" PRIu64 "h", (uint64_t)(hrs/10ull), (uint64_t)(hrs%10ull));
    }
    snprintf(out, sz, "%s %" PRIu64 "%%%s", status[0] == 'C' ? "🔌" : "🔋", (uint64_t)pct, eta);
}

static void updateRam(void *ctx, char *out, size_t sz) {
    RamCtx *c = (RamCtx *)ctx;
    if (c->fd_mem < 0) { snprintf(out, sz, "🧠 --"); return; }
    
    lseek(c->fd_mem, 0, SEEK_SET);
    char buf[1024];
    ssize_t n = read(c->fd_mem, buf, sizeof(buf)-1);
    if (n <= 0) { snprintf(out, sz, "🧠 --"); return; }
    buf[n] = 0;

    uint64_t tot=0, avl=0;
    char *p = buf;
    while (p && *p) {
        if (strncmp(p, "MemTotal:", 9) == 0) tot = (uint64_t)strtoull(p + 9, NULL, 10);
        else if (strncmp(p, "MemAvailable:", 13) == 0) avl = (uint64_t)strtoull(p + 13, NULL, 10);
        p = strchr(p, '\n');
        if (p) p++;
    }
    
    double uGiB = (double)(tot - avl) / 1048576.0;
    double tGiB = (double)tot / 1048576.0;
    snprintf(out, sz, "🧠 %.1f/%.1fGiB", uGiB, tGiB);
}

static void updateTemp(void *ctx, char *out, size_t sz) {
    TempCtx *c = (TempCtx *)ctx;
    uint64_t pkg = 0, core = 0;
    bool pOk = readFdU64(c->fd_pkg, &pkg);
    bool cOk = readFdU64(c->fd_core, &core);

    if (pOk && cOk) snprintf(out, sz, "🔥 %" PRIu64 "/%" PRIu64 "°C", pkg/1000, core/1000);
    else if (pOk) snprintf(out, sz, "🔥 %" PRIu64 "°C", pkg/1000);
    else if (cOk) snprintf(out, sz, "🔥 %" PRIu64 "°C", core/1000);
    else snprintf(out, sz, "🔥 ?°C");
}

static void updateCpu(void *ctx, char *out, size_t sz) {
    CpuCtx *c = (CpuCtx *)ctx;
    if (c->fd_stat < 0) return;
    
    lseek(c->fd_stat, 0, SEEK_SET);
    char buf[256];
    ssize_t n = read(c->fd_stat, buf, sizeof(buf)-1);
    if (n <= 0) return;
    buf[n] = 0;

    uint64_t u, n_val, s, i, io, ir, so, st;
    if (sscanf(buf, "cpu %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64, &u, &n_val, &s, &i, &io, &ir, &so, &st) < 8) return;

    uint64_t idle = i + io;
    uint64_t total = u + n_val + s + i + io + ir + so + st;
    uint64_t dTotal = total - prevCpuTotal;
    uint64_t dIdle = idle - prevCpuIdle;
    
    prevCpuTotal = total; 
    prevCpuIdle = idle;
    
    unsigned pct = dTotal ? (unsigned)((dTotal - dIdle) * 100ull / dTotal) : 0;
    snprintf(out, sz, "📈 %u%%", pct);
}

static void updatePower(void *ctx, char *out, size_t sz) {
    PowerCtx *c = (PowerCtx *)ctx;
    uint64_t cpuUj=0, socUj=0, now = getNowUs();
    bool hCpu = readFdU64(c->fd_rapl0, &cpuUj);
    bool hSoc = readFdU64(c->fd_rapl1, &socUj);
    
    uint64_t dt = (now > prevTimeUs) ? (now - prevTimeUs) : 1;
    uint64_t dCpu = (hCpu && cpuUj >= prevCpuUj) ? (cpuUj - prevCpuUj) : 0;
    uint64_t dSoc = (hSoc && socUj >= prevSocUj) ? (socUj - prevSocUj) : 0;

    uint64_t wCpu = (dCpu * 10ull + dt/2ull) / dt;
    uint64_t wSoc = (dSoc * 10ull + dt/2ull) / dt;

    if (hCpu && hSoc) 
        snprintf(out, sz, "^fg(FFD700)⚡^fg() %" PRIu64 ".%" PRIu64 "/%" PRIu64 ".%" PRIu64 "W", (uint64_t)(wSoc/10), (uint64_t)(wSoc%10), (uint64_t)(wCpu/10), (uint64_t)(wCpu%10));
    else if (hCpu) 
        snprintf(out, sz, "^fg(FFD700)⚡^fg() %" PRIu64 ".%" PRIu64 "W", (uint64_t)(wCpu/10), (uint64_t)(wCpu%10));
    else 
        snprintf(out, sz, "^fg(FFD700)⚡^fg() %s", hSoc ? "" : "?W");

    prevCpuUj = cpuUj; prevSocUj = socUj; prevTimeUs = now;
}

static void updateDuck(void *ctx, char *out, size_t sz) { (void)ctx; snprintf(out, sz, "🦆"); }
static void updateLauncher(void *ctx, char *out, size_t sz) { (void)ctx; snprintf(out, sz, "🐧"); }


static void sendSignal(int idx) {
    DIR *d = opendir("/proc");
    if (!d) return;
    struct dirent *de;
    pid_t me = getpid();
    while ((de = readdir(d))) {
        if (!isdigit(de->d_name[0])) continue;
        pid_t pid = atoi(de->d_name);
        if (pid == me) continue;
        char path[64], comm[64];
        snprintf(path, sizeof(path), "/proc/%d/comm", pid);
        FILE *f = fopen(path, "r");
        if (f) {
            if (fgets(comm, sizeof(comm), f)) {
                comm[strcspn(comm, "\n")] = 0; /* strip newline for valid check */
                if (strcmp(comm, "dwlb-status") == 0)
                    kill(pid, SIGRTMIN + idx);
            }
            fclose(f);
        }
    }
    closedir(d);
}

/* Main */

/* ... (includes and defines same as before) ... */
/* ... (structs, globals, helpers, and update functions same as before) ... */

/* Replace the Main function with this updated version */

int main(int argc, char **argv) {
    if (argc > 2 && strcmp(argv[1], "--signal") == 0) {
        sendSignal(atoi(argv[2]));
        return 0;
    }

    int raplInt = RAPL_DEFAULT;
    char *envRapl = getenv("RAPL_EVERY");
    if (envRapl) raplInt = atoi(envRapl);
    for (int i=1; i<argc-1; ++i) 
        if (!strcmp(argv[i], "-r")) raplInt = atoi(argv[i+1]);

    for (int i = 0; i < 32; ++i) signal(SIGRTMIN + i, handleSig);
    
    initHwmon();
    readFileU64("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj", &prevCpuUj);
    prevTimeUs = getNowUs();

    TempCtx tempCtx = {
        .fd_pkg = cpuPkgPath ? open(cpuPkgPath, O_RDONLY | O_CLOEXEC) : -1,
        .fd_core = cpuCorePath ? open(cpuCorePath, O_RDONLY | O_CLOEXEC) : -1
    };
    CpuCtx cpuCtx = { .fd_stat = open("/proc/stat", O_RDONLY | O_CLOEXEC) };
    RamCtx ramCtx = { .fd_mem = open("/proc/meminfo", O_RDONLY | O_CLOEXEC) };
    PowerCtx powerCtx = {
        .fd_rapl0 = open("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj", O_RDONLY | O_CLOEXEC),
        .fd_rapl1 = open("/sys/class/powercap/intel-rapl/intel-rapl:1/energy_uj", O_RDONLY | O_CLOEXEC)
    };
    BatCtx batCtx = {
        .fd_cap = open("/sys/class/power_supply/BAT0/capacity", O_RDONLY | O_CLOEXEC),
        .fd_stat = open("/sys/class/power_supply/BAT0/status", O_RDONLY | O_CLOEXEC),
        .fd_enNow = open("/sys/class/power_supply/BAT0/energy_now", O_RDONLY | O_CLOEXEC),
        .fd_enFull = open("/sys/class/power_supply/BAT0/energy_full", O_RDONLY | O_CLOEXEC),
        .fd_pwr = open("/sys/class/power_supply/BAT0/power_now", O_RDONLY | O_CLOEXEC)
    };

    Module modules[] = {
        { .update = updateVolume,   .interval = 3,       .signalId = 0,  .label = "Vol",
          .onLeft = "pamixer -t",   .onRight = "pavucontrol",
          .onScrollUp = "sh -c \"pamixer -i 2; dwlb-status --signal 0\"",
          .onScrollDown = "sh -c \"pamixer -d 2; dwlb-status --signal 0\"" },
        
        { .update = updateAirpods,  .interval = 3,       .signalId = -1, .label = "Pods",
          .onLeft = "airpods",      .onRight = "librepods" },
        
        { .update = updatePower,    .interval = raplInt, .signalId = -1, .label = "Pwr", .ctx = &powerCtx },
        
        { .update = updateDuck,     .interval = 9999,    .signalId = -1, .label = "Duck",
          .onLeft = "sh -c 'pgrep -x wmbubble >/dev/null || wmbubble &'",
          .onRight = "pkill -x wmbubble" },
        
        { .update = updateTemp,     .interval = 1,       .signalId = -1, .label = "Temp", .ctx = &tempCtx },
        
        { .update = updateCpu,      .interval = 1,       .signalId = -1, .label = "CPU", .ctx = &cpuCtx,
          .onLeft = "cpupower-gui" },
        
        { .update = updateRam,      .interval = 1,       .signalId = -1, .label = "RAM", .ctx = &ramCtx },
        
        { .update = updateDisk,     .interval = 120,     .signalId = -1, .label = "Disk" },
        
        { .update = updateBattery,  .interval = 20,      .signalId = -1, .label = "Bat", .ctx = &batCtx },
        
        { .update = updateTime,     .interval = 8,       .signalId = -1, .label = "Time" },
        
        { .update = updateTheme,    .interval = 100,     .signalId = 1,  .label = "Theme",
          .onLeft = "sh -c 'switch-theme -a; dwlb-status --signal 1'" },
        
        { .update = updateLauncher, .interval = 9999,    .signalId = -1, .label = "Launch",
          .onLeft = "sh -c 'fuzzel &'", .onRight = "autored" }
    };
    
    int count = sizeof(modules) / sizeof(modules[0]);
    int tick = 0;
    bool timePassed = true; /* true = normal tick, false = signal interruption */

    /* Initial run: update all */
    for (int i=0; i<count; ++i) modules[i].update(modules[i].ctx, modules[i].buffer, sizeof(modules[i].buffer));

    for (;;) {
        /* Only increment tick if we woke up from sleep naturally */
        if (timePassed) tick++;

        sig_atomic_t mask = signalMask;
        signalMask = 0;

        for (int i = 0; i < count; i++) {
            bool forced = (modules[i].signalId != -1) && (mask & (1 << modules[i].signalId));
            
            /* Only check interval if time actually passed */
            bool timeDue = timePassed && (tick % modules[i].interval == 0);

            if (forced || timeDue) {
                modules[i].update(modules[i].ctx, modules[i].buffer, sizeof(modules[i].buffer));
            }
        }

        char line[1024];
        buildLine(modules, count, line, sizeof(line));
        puts(line);
        fflush(stdout);

        struct timespec req = { (time_t)LOOP_DELAY_SEC, (long)((LOOP_DELAY_SEC - (time_t)LOOP_DELAY_SEC) * 1e9) };
        if (nanosleep(&req, NULL) == -1 && errno == EINTR) {
            timePassed = false; /* Signal received: skip tick increment next loop */
            continue;
        }
        timePassed = true; /* Timeout reached: normal tick next loop */
    }
    return 0;
}
