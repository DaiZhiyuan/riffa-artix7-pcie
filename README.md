# RIFFA Artix-7 PCIe Benchmark & MSI Validation

A lightweight verification framework for evaluating PCIe throughput and MSI interrupt signaling on Xilinx Artix-7 FPGAs using the open-source **RIFFA 2.2.2** (Reusable Integration Framework for FPGA Accelerators) architecture.

---

## Architecture & Features

*   **High-Throughput DMA:** Maximizes PCIe link saturation via optimized Scatter-Gather buffer management.
*   **MSI Interrupt Validation:** Triggers and monitors hardware-to-host Message Signaled Interrupts (MSI) during continuous data transfers.
*   **Modular Layout:** Clean separation between the Linux kernel-space driver (`riffa_linux_driver`) and user-space test utilities (`riffa_linux_app`).

---

## Directory Structure

```text
.
├── riffa_linux_driver/    # RIFFA kernel-space driver module
├── riffa_linux_app/       # User-space test suite and benchmark utilities
└── README.md

```

---

## Prerequisites

* **Hardware:** Xilinx Artix-7 FPGA development board (e.g., AC701) configured with the RIFFA 2.2.2 PCIe endpoint bitstream.
* **OS:** Linux kernel (v4.x - v6.x tested).
* **Dependencies:** Build essentials, kernel headers matching the running kernel (`linux-headers-$(uname -r)`).

---

## Installation & Deployment

### 1. Build and Load the Kernel Driver

Navigate to the driver directory, compile the kernel module, and load it into the system:

```bash
cd riffa_linux_driver/
sudo make install
sudo insmod riffa.ko

```

Verify that the driver has successfully initialized and attached to the PCIe endpoint:

```bash
dmesg | grep riffa
lsmod | grep riffa

```

### 2. Compile the User-Space Benchmark Application

Navigate to the application directory and build the executable using the provided Makefile:

```bash
cd ../riffa_linux_app/pcie_bandwidth_test
make

```

---

## Execution & Verification

### Bandwidth Testing

Execute the user-space application to initiate bidirectional DMA transfers and trigger hardware interrupts.

```bash
# Run the performance test (e.g., streaming blocks to measure throughput)
sudo ./pcie_bandwidth_test

Number of FPGA device: 1
========== FPGA Device 0 =========
ID: 0
Number of Channels: 1
Name: 0000:01:00.00
Vendor ID: 10EE
Device ID: 7024

------------------- PCIE RIFFA DATA LOOPBACK TEST ----------------
===============================================================================================
| CH       | SIZE(B)    | TX_BANDWIDTH(MB/s) | TX_TIME(ms) | RX_BANDWIDTH(MB/s) | RX_TIME(ms) |
| 0        | 256        | 34.93              | 0.0280      | 24.39              | 0.0400      |
| 0        | 4096       | 368.42             | 0.0424      | 517.54             | 0.0302      |
| 0        | 65536      | 904.38             | 0.2764      | 809.61             | 0.3088      |
| 0        | 262144     | 1339.86            | 0.7463      | 1338.16            | 0.7473      |
| 0        | 1048576    | 1470.58            | 2.7200      | 1414.52            | 2.8278      |
| 0        | 16777216   | 1669.16            | 38.3427     | 1692.64            | 37.8108     |
-----------------------------------------------------------------------------------------------

######     #     #####   #####
#     #   # #   #     # #     #
#     #  #   #  #       #
######  #     #  #####   #####
#       #######       #       #
#       #     # #     # #     #
#       #     #  #####   #####

```

---

## Performance Reference

Typical performance metrics observed on PCIe Gen2 x4 configurations:

* **Peak Read (C2H) Bandwidth:** $\approx 1.5 \text{ GB/s}$ (approaching theoretical line rate limit).
* **Peak Write (H2C) Bandwidth:** $\approx 1.4 \text{ GB/s}$.
* **Interrupt Latency:** Sub-microsecond MSI dispatch latency under dedicated CPU affinity pinning.

---

## License

Distributed under the MIT License. See `LICENSE` for more information.
