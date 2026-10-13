#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include "riffa.h"

#define DMA_TIMEOUT_MS   5000
#define STATS_DELAY_US   5000

// ANSI Color Codes for Terminal Display
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_RED    "\033[1;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_RESET  "\033[0m"

typedef struct {
    char bdf[64];               // PCI BDF Address (e.g. 0000:01:00.0)
    int irq;                    // System IRQ Number
    unsigned long long count;   // Interrupt Total Count
    char affinity[64];          // Target CPU Cores
} pci_irq_info_t;

/**
 * Dynamically parse PCI BDF via sysfs driver binding (/sys/bus/pci/drivers/riffa/)
 */
static int sysfs_get_bdf(char *bdf_buf, size_t buf_len) {
    DIR *dir = opendir("/sys/bus/pci/drivers/riffa");
    if (!dir) return -1;

    struct dirent *entry;
    int found = 0;
    while ((entry = readdir(dir)) != NULL) {
        // Look for PCI BDF symlink format (contains ':' and '.')
        if (strchr(entry->d_name, ':') && strchr(entry->d_name, '.')) {
            snprintf(bdf_buf, buf_len, "%s", entry->d_name);
            found = 1;
            break;
        }
    }
    closedir(dir);
    return found ? 0 : -1;
}

/**
 * Dynamically read IRQ vector number from /sys/bus/pci/devices/<BDF>/irq
 */
static int sysfs_get_irq(const char *bdf) {
    char path[128];
    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/irq", bdf);
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    int irq = -1;
    if (fscanf(fp, "%d", &irq) != 1) {
        irq = -1;
    }
    fclose(fp);
    return irq;
}

/**
 * Read CPU interrupt counter from /proc/interrupts for a specific IRQ
 */
static unsigned long long proc_get_irq_count(int irq) {
    FILE *fp = fopen("/proc/interrupts", "r");
    if (!fp) return 0;

    char line[1024];
    char irq_prefix[32];
    snprintf(irq_prefix, sizeof(irq_prefix), "%d:", irq);

    unsigned long long total_count = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++; // Skip leading spaces

        if (strncmp(p, irq_prefix, strlen(irq_prefix)) == 0) {
            p += strlen(irq_prefix);
            // Sum counters across all CPU columns
            while (*p) {
                while (*p == ' ' || *p == '\t') p++;
                if (*p >= '0' && *p <= '9') {
                    char *endp;
                    unsigned long long val = strtoull(p, &endp, 10);
                    total_count += val;
                    p = endp;
                } else {
                    break;
                }
            }
            break;
        }
    }
    fclose(fp);
    return total_count;
}

/**
 * Read IRQ Target CPU cores from /proc/irq/<IRQ>/smp_affinity_list
 */
static void proc_get_affinity(int irq, char *buf, size_t buf_len) {
    char path[128];
    snprintf(path, sizeof(path), "/proc/irq/%d/smp_affinity_list", irq);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        snprintf(buf, buf_len, "N/A");
        return;
    }
    if (fgets(buf, buf_len, fp)) {
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
    } else {
        snprintf(buf, buf_len, "Unknown");
    }
    fclose(fp);
}

/**
 * Fully dynamic PCI BDF, IRQ, and CPU affinity resolution
 */
int resolve_pci_irq_info(pci_irq_info_t *info) {
    memset(info, 0, sizeof(*info));

    // 1. Try sysfs dynamic discovery
    if (sysfs_get_bdf(info->bdf, sizeof(info->bdf)) == 0) {
        info->irq = sysfs_get_irq(info->bdf);
    }

    // 2. Fallback: Parse /proc/interrupts for 'riffa' keyword
    if (info->irq <= 0) {
        FILE *fp = fopen("/proc/interrupts", "r");
        if (fp) {
            char line[1024];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "riffa")) {
                    char *colon = strchr(line, ':');
                    if (colon) {
                        *colon = '\0';
                        info->irq = atoi(line);
                        snprintf(info->bdf, sizeof(info->bdf), "Auto (via /proc)");
                        break;
                    }
                }
            }
            fclose(fp);
        }
    }

    if (info->irq <= 0) return -1;

    // 3. Fetch count and target CPU affinity
    info->count = proc_get_irq_count(info->irq);
    proc_get_affinity(info->irq, info->affinity, sizeof(info->affinity));

    return 0;
}

int main() {
    fpga_t *fpga = NULL;
    fpga_info_list list;
    pci_irq_info_t irq_before, irq_after;
    unsigned int send_buf[1] = {0xDEADBEEF};
    int words_sent = 0;

    printf("\n" COLOR_CYAN "==========================================================" COLOR_RESET "\n");
    printf("    PCIe Single DMA & MSI Test       \n");
    printf(COLOR_CYAN "==========================================================" COLOR_RESET "\n\n");

    // -------------------------------------------------------------
    // Step 1: Open FPGA and dynamically parse sysfs BDF & IRQ
    // -------------------------------------------------------------
    printf(COLOR_YELLOW "[Step 1] Detecting Device & Parsing Sysfs Info..." COLOR_RESET "\n");

    if (fpga_list(&list) != 0 || list.num_fpgas == 0) {
        printf(COLOR_RED "[ERROR] No RIFFA FPGA device detected." COLOR_RESET "\n");
        return -1;
    }

    fpga = fpga_open(0);
    if (fpga == NULL) {
        printf(COLOR_RED "[ERROR] Could not open FPGA device 0." COLOR_RESET "\n");
        return -1;
    }
    printf("  -> FPGA Device 0 opened successfully.\n");

    if (resolve_pci_irq_info(&irq_before) != 0) {
        printf(COLOR_RED "[ERROR] Failed to resolve PCI BDF and IRQ from sysfs." COLOR_RESET "\n");
        fpga_close(fpga);
        return -1;
    }

    printf("  -> " COLOR_GREEN "PCI BDF Address   :" COLOR_RESET " %s\n", irq_before.bdf);
    printf("  -> " COLOR_GREEN "System IRQ Vector :" COLOR_RESET " %d\n", irq_before.irq);
    printf("  -> " COLOR_GREEN "Target CPU Core(s):" COLOR_RESET " CPU %s\n", irq_before.affinity);
    printf("  -> " COLOR_GREEN "Initial IRQ Count :" COLOR_RESET " %llu\n", irq_before.count);

    printf("\n");

    // -------------------------------------------------------------
    // Step 2: Execute single DMA transaction
    // -------------------------------------------------------------
    printf(COLOR_YELLOW "[Step 2] Executing Single DMA Transfer..." COLOR_RESET "\n");
    printf("  -> Sending 1 Word (4 Bytes) to Channel 0...\n");

    words_sent = fpga_send(fpga, 0, send_buf, 1, 0, 1, DMA_TIMEOUT_MS);

    if (words_sent > 0) {
        printf("  -> " COLOR_GREEN "[SUCCESS] DMA Transfer completed (%d word sent)." COLOR_RESET "\n", words_sent);
    } else {
        printf("  -> " COLOR_RED "[ERROR] DMA Transfer failed (timeout or hardware lockup)." COLOR_RESET "\n");
        fpga_close(fpga);
        return -1;
    }

    printf("\n");

    // -------------------------------------------------------------
    // Step 3: Verify IRQ count increment
    // -------------------------------------------------------------
    printf(COLOR_YELLOW "[Step 3] Verifying MSI Interrupt Counter..." COLOR_RESET "\n");

    usleep(STATS_DELAY_US); // 2ms delay for kernel stats update

    if (resolve_pci_irq_info(&irq_after) == 0) {
        long long delta = (long long)irq_after.count - (long long)irq_before.count;

        printf("  -> " COLOR_GREEN "Updated IRQ Count :" COLOR_RESET " %llu\n", irq_after.count);
        printf("  -> " COLOR_GREEN "Interrupt Increment:" COLOR_RESET " +%lld\n", delta);

        if (delta > 0) {
            printf("\n" COLOR_GREEN "==========================================================" COLOR_RESET "\n");
            printf(COLOR_GREEN "  [PASS] MSI Interrupt successfully triggered (+%lld)!" COLOR_RESET "\n", delta);
            printf(COLOR_GREEN "==========================================================" COLOR_RESET "\n\n");
        } else {
            printf("\n" COLOR_RED "==========================================================" COLOR_RESET "\n");
            printf(COLOR_RED "  [FAIL] DMA succeeded, but IRQ counter did not increment." COLOR_RESET "\n");
            printf(COLOR_RED "==========================================================" COLOR_RESET "\n\n");
        }
    }

    fpga_close(fpga);
    return 0;
}
