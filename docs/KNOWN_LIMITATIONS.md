# Known Limitations

This document tracks known architectural, platform, and accounting limitations across current and planned phases.

1. **Single-User SQLite Initial Scope:**
   - The initial C++ release utilizes SQLite with Write-Ahead Logging (WAL) for local desktop responsiveness. While highly reliable for single workstations, multi-user concurrent networked access will require the planned PostgreSQL server backend.
2. **Proprietary Tally Binary Files:**
   - The application does not read or parse encrypted, proprietary `.900` data directories directly. Data exchange is achieved through documented XML envelopes and JSON exchange formats.
3. **Statutory Return Filing:**
   - Basic GSTIN and tax rates are supported; full statutory e-way bill generation and direct GST portal API filing are outside the scope of Milestone 1-8.
4. **Bill-by-Bill Allocation:**
   - In the initial accounting core, receivables/payables are calculated on a net ledger balance basis rather than discrete FIFO bill allocations (`Agst Ref`, `New Ref`).
5. **Operating System Target:**
   - The primary UI and UX design targets the Windows desktop experience (fonts, shortcuts, Alt-key mnemonic accelerators). The codebase is cross-platform standard C++20 and Qt 6, allowing compilation on Linux for development and CI/CD.
