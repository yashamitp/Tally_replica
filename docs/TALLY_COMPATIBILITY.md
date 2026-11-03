# Tally Compatibility Specification

## 1. Scope & Legal Notice
This project provides documented, clean-room interoperability with standard TallyPrime / Tally.ERP 9 data exchange formats (XML, JSON, HTTP REST). It does **not** decompile or use proprietary Tally binaries or data storage files (`Company.900`, etc.).

---

## 2. Supported Formats

### 2.1 Tally XML Format
Tally uses XML payloads encapsulated within `<ENVELOPE>` tags.

#### 2.1.1 Date Format
- Tally: `YYYYMMDD` (e.g., `20260401`)
- Internal Application: `YYYY-MM-DD` (ISO-8601)

#### 2.1.2 Amount & DR/CR Convention
In standard Tally XML:
- In `ALLLEDGERENTRIES.LIST`:
  - `ISDEEMEDPOSITIVE = "Yes"` signifies a **Debit**.
  - `ISDEEMEDPOSITIVE = "No"` signifies a **Credit**.
  - `AMOUNT < 0` (negative) represents a **Debit**.
  - `AMOUNT > 0` (positive) represents a **Credit**.

The C++ integration layer parses these into explicit positive `ExactDecimal` amounts and assigns `DR` or `CR` accordingly.

#### 2.1.3 Export Envelope
Exported XML conforms to:
```xml
<ENVELOPE>
  <HEADER>
    <TALLYREQUEST>Import Data</TALLYREQUEST>
  </HEADER>
  <BODY>
    <IMPORTDATA>
      <REQUESTDESC>
        <REPORTNAME>All Masters</REPORTNAME>
      </REQUESTDESC>
      <REQUESTDATA>
        <TALLYMESSAGE xmlns:UDF="TallyUDF">
          <!-- Vouchers or Masters -->
        </TALLYMESSAGE>
      </REQUESTDATA>
    </IMPORTDATA>
  </BODY>
</ENVELOPE>
```

---

## 3. Incremental Synchronization via HTTP Port 9000

- Tally can expose an HTTP listener (default port 9000).
- The application implements:
  1. **Ping / Health Probe:** Tests connectivity and queries active company name via TDL query `<STATICVARIABLES><SVCURRENTCOMPANY>...</SVCURRENTCOMPANY></STATICVARIABLES>`.
  2. **Cursor Tracking:** Tracks `ALTERID` to pull only modified or newly created vouchers.
  3. **Voucher Push:** Encapsulates local vouchers in Tally XML envelopes and posts them via HTTP `POST` to port 9000.
