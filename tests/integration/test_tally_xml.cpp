#include <cassert>
#include <iostream>
#include <QCoreApplication>
#include "../../src/database/DatabaseManager.hpp"
#include "../../src/services/CompanyService.hpp"
#include "../../src/services/LedgerService.hpp"
#include "../../src/integrations/xml/TallyXmlReader.hpp"
#include "../../src/integrations/xml/TallyXmlWriter.hpp"
#include "../../src/integrations/xml/TallyXmlImporter.hpp"
#include "../../src/integrations/xml/TallyXmlExporter.hpp"

using namespace tally;
using namespace tally::integrations::xml;

void testXmlParsingAndNormalization() {
    std::string sampleXml = R"(
<ENVELOPE>
  <HEADER>
    <VERSION>1</VERSION>
    <TALLYREQUEST>Import</TALLYREQUEST>
    <TYPE>Data</TYPE>
    <ID>All Masters</ID>
  </HEADER>
  <BODY>
    <DESC>
      <STATICVARIABLES>
        <SVCURRENTCOMPANY>National Motors</SVCURRENTCOMPANY>
      </STATICVARIABLES>
    </DESC>
    <DATA>
      <TALLYMESSAGE xmlns:UDF="TallyUDF">
        <GROUP NAME="Sundry Creditors" ACTION="Create">
          <NAME>Sundry Creditors</NAME>
          <PARENT>Current Liabilities</PARENT>
        </GROUP>
        <LEDGER NAME="Bosch Auto Parts" ACTION="Create">
          <NAME>Bosch Auto Parts</NAME>
          <PARENT>Sundry Creditors</PARENT>
          <OPENINGBALANCE>50000.0000</OPENINGBALANCE>
          <ISDEEMEDPOSITIVE>No</ISDEEMEDPOSITIVE>
          <PARTYGSTIN>27AABCB1234F1Z1</PARTYGSTIN>
        </LEDGER>
        <VOUCHER VCHTYPE="Purchase" ACTION="Create">
          <DATE>20260515</DATE>
          <VOUCHERTYPENAME>Purchase</VOUCHERTYPENAME>
          <VOUCHERNUMBER>PUR-001</VOUCHERNUMBER>
          <REFERENCE>INV-9921</REFERENCE>
          <NARRATION>Purchase of spare brake components</NARRATION>
          <ALLLEDGERENTRIES.LIST>
            <LEDGERNAME>Purchase Accounts</LEDGERNAME>
            <ISDEEMEDPOSITIVE>Yes</ISDEEMEDPOSITIVE>
            <AMOUNT>-25000.0000</AMOUNT>
          </ALLLEDGERENTRIES.LIST>
          <ALLLEDGERENTRIES.LIST>
            <LEDGERNAME>Bosch Auto Parts</LEDGERNAME>
            <ISDEEMEDPOSITIVE>No</ISDEEMEDPOSITIVE>
            <AMOUNT>25000.0000</AMOUNT>
          </ALLLEDGERENTRIES.LIST>
        </VOUCHER>
      </TALLYMESSAGE>
    </DATA>
  </BODY>
</ENVELOPE>
)";

    auto payload = TallyXmlReader::parse(sampleXml);
    assert(!payload.companyNames.empty());
    assert(payload.companyNames[0] == "National Motors");
    assert(payload.groups.size() == 1);
    assert(payload.groups[0].name == "Sundry Creditors");
    assert(payload.groups[0].parentName == "Current Liabilities");

    assert(payload.ledgers.size() == 1);
    assert(payload.ledgers[0].name == "Bosch Auto Parts");
    assert(payload.ledgers[0].gstin == "27AABCB1234F1Z1");

    assert(payload.vouchers.size() == 1);
    const auto& vch = payload.vouchers[0];
    assert(vch.voucherTypeName == "Purchase");
    assert(vch.voucherNumber == "PUR-001");
    assert(vch.date == "2026-05-15"); // Parsed from 20260515
    assert(vch.referenceNumber == "INV-9921");
    assert(vch.entries.size() == 2);
    assert(vch.isBalanced);
    assert(vch.entries[0].ledgerName == "Purchase Accounts");
    assert(vch.entries[0].entryType == domain::BalanceType::Dr);
    assert(vch.entries[0].amount.toDbString() == "25000.0000");

    assert(vch.entries[1].ledgerName == "Bosch Auto Parts");
    assert(vch.entries[1].entryType == domain::BalanceType::Cr);
    assert(vch.entries[1].amount.toDbString() == "25000.0000");

    std::cout << "[PASS] testXmlParsingAndNormalization" << std::endl;
}

void testXmlImportPreviewAndExecution() {
    database::DatabaseManager dbManager("XmlTestConnection");
    bool ok = dbManager.open(":memory:");
    assert(ok);

    services::CompanyService companyService(dbManager);
    services::LedgerService ledgerService(dbManager);

    services::CreateCompanyParams cp;
    cp.name = "Delta Traders";
    cp.financialYearStart = "2026-04-01";
    auto comp = companyService.createCompany(cp);

    std::string xmlData = R"(
<ENVELOPE>
  <BODY>
    <DATA>
      <TALLYMESSAGE>
        <GROUP NAME="Regional Debtors">
          <NAME>Regional Debtors</NAME>
          <PARENT>Sundry Debtors</PARENT>
        </GROUP>
        <LEDGER NAME="Apex Infotech">
          <NAME>Apex Infotech</NAME>
          <PARENT>Regional Debtors</PARENT>
          <OPENINGBALANCE>10000.0000</OPENINGBALANCE>
          <ISDEEMEDPOSITIVE>Yes</ISDEEMEDPOSITIVE>
        </LEDGER>
        <LEDGER NAME="Sales Accounts">
          <NAME>Sales Accounts</NAME>
          <PARENT>Sales Accounts</PARENT>
          <OPENINGBALANCE>0.0000</OPENINGBALANCE>
          <ISDEEMEDPOSITIVE>No</ISDEEMEDPOSITIVE>
        </LEDGER>
        <VOUCHER VCHTYPE="Sales">
          <DATE>20260410</DATE>
          <VOUCHERTYPENAME>Sales</VOUCHERTYPENAME>
          <VOUCHERNUMBER>SALES-001</VOUCHERNUMBER>
          <ALLLEDGERENTRIES.LIST>
            <LEDGERNAME>Apex Infotech</LEDGERNAME>
            <ISDEEMEDPOSITIVE>Yes</ISDEEMEDPOSITIVE>
            <AMOUNT>-30000.0000</AMOUNT>
          </ALLLEDGERENTRIES.LIST>
          <ALLLEDGERENTRIES.LIST>
            <LEDGERNAME>Sales Accounts</LEDGERNAME>
            <ISDEEMEDPOSITIVE>No</ISDEEMEDPOSITIVE>
            <AMOUNT>30000.0000</AMOUNT>
          </ALLLEDGERENTRIES.LIST>
        </VOUCHER>
      </TALLYMESSAGE>
    </DATA>
  </BODY>
</ENVELOPE>
)";

    TallyXmlImporter importer(dbManager);

    // 1. Preview
    auto preview = importer.preview(comp.id, xmlData);
    assert(preview.groupsCount == 1);
    assert(preview.ledgersCount == 2);
    assert(preview.vouchersCount == 1);
    assert(preview.acceptedCount == 4);
    assert(preview.canCommit);

    // 2. Execute
    auto exec = importer.execute(comp.id, xmlData);
    if (!exec.success) {
        for (const auto& err : exec.errors) {
            std::cerr << "EXEC ERROR: " << err << std::endl;
        }
    }
    assert(exec.success);
    assert(exec.createdGroups == 1);
    assert(exec.createdLedgers == 2);
    assert(exec.createdVouchers == 1);

    // Verify ledger created and balance updated
    auto apexLedger = ledgerService.getLedgerByName(comp.id, "Apex Infotech");
    assert(apexLedger.has_value());
    // Opening 10,000 DR + 30,000 DR Sales = 40,000 DR
    assert(apexLedger->currentBalance.toDbString() == "40000.0000");

    // 3. Round-trip Export
    TallyXmlExporter exporter(dbManager);
    int masterCount = 0;
    std::string exportedMasters = exporter.exportMasters(comp.id, &masterCount);
    assert(masterCount > 0);
    assert(exportedMasters.find("Apex Infotech") != std::string::npos);
    assert(exportedMasters.find("Regional Debtors") != std::string::npos);

    int voucherCount = 0;
    ExportVouchersFilter vf;
    vf.companyId = comp.id;
    std::string exportedVouchers = exporter.exportVouchers(vf, &voucherCount);
    assert(voucherCount == 1);
    assert(exportedVouchers.find("SALES-001") != std::string::npos);
    assert(exportedVouchers.find("30000.0000") != std::string::npos);

    std::cout << "[PASS] testXmlImportPreviewAndExecution" << std::endl;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "--- Running Tally XML Compatibility Tests ---" << std::endl;
    testXmlParsingAndNormalization();
    testXmlImportPreviewAndExecution();
    std::cout << "All Tally XML tests passed successfully!" << std::endl;
    return 0;
}
