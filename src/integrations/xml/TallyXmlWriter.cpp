#include "TallyXmlWriter.hpp"
#include "TallyXmlUtils.hpp"
#include <sstream>

namespace tally::integrations::xml {

std::string TallyXmlWriter::groupToXml(const domain::AccountingGroup& group, const std::string& parentName) {
    std::ostringstream oss;
    std::string safeName = TallyXmlUtils::escapeXml(group.name);
    oss << "      <GROUP NAME=\"" << safeName << "\" ACTION=\"Create\">\n";
    oss << "        <NAME>" << safeName << "</NAME>\n";
    if (!parentName.empty()) {
        oss << "        <PARENT>" << TallyXmlUtils::escapeXml(parentName) << "</PARENT>\n";
    }
    oss << "      </GROUP>";
    return oss.str();
}

std::string TallyXmlWriter::ledgerToXml(const domain::Ledger& ledger, const std::string& parentGroupName) {
    std::ostringstream oss;
    std::string safeName = TallyXmlUtils::escapeXml(ledger.name);
    std::string safeParent = TallyXmlUtils::escapeXml(parentGroupName);

    std::string opBalStr = "0.0000";
    std::string isDeemed = "Yes";

    if (!ledger.openingBalance.isZero()) {
        if (ledger.openingBalanceType == domain::BalanceType::Dr) {
            opBalStr = "-" + ledger.openingBalance.toDbString();
            isDeemed = "Yes";
        } else {
            opBalStr = ledger.openingBalance.toDbString();
            isDeemed = "No";
        }
    }

    oss << "      <LEDGER NAME=\"" << safeName << "\" ACTION=\"Create\">\n";
    oss << "        <NAME>" << safeName << "</NAME>\n";
    oss << "        <PARENT>" << safeParent << "</PARENT>\n";
    oss << "        <OPENINGBALANCE>" << opBalStr << "</OPENINGBALANCE>\n";
    oss << "        <ISDEEMEDPOSITIVE>" << isDeemed << "</ISDEEMEDPOSITIVE>\n";
    if (ledger.gstin.has_value() && !ledger.gstin->empty()) {
        oss << "        <PARTYGSTIN>" << TallyXmlUtils::escapeXml(*ledger.gstin) << "</PARTYGSTIN>\n";
    }
    oss << "      </LEDGER>";
    return oss.str();
}

std::string TallyXmlWriter::voucherToXml(const domain::Voucher& voucher, const std::string& voucherTypeName, const std::map<std::string, std::string>& ledgerIdToName) {
    std::ostringstream oss;
    std::string safeType = TallyXmlUtils::escapeXml(voucherTypeName.empty() ? "Journal" : voucherTypeName);
    std::string safeNum = TallyXmlUtils::escapeXml(voucher.voucherNumber);
    std::string tallyDate = TallyXmlUtils::formatTallyDate(voucher.date);

    oss << "      <VOUCHER VCHTYPE=\"" << safeType << "\" ACTION=\"Create\">\n";
    oss << "        <DATE>" << tallyDate << "</DATE>\n";
    oss << "        <VOUCHERTYPENAME>" << safeType << "</VOUCHERTYPENAME>\n";
    oss << "        <VOUCHERNUMBER>" << safeNum << "</VOUCHERNUMBER>\n";
    if (voucher.referenceNumber.has_value() && !voucher.referenceNumber->empty()) {
        oss << "        <REFERENCE>" << TallyXmlUtils::escapeXml(*voucher.referenceNumber) << "</REFERENCE>\n";
    }
    if (voucher.narration.has_value() && !voucher.narration->empty()) {
        oss << "        <NARRATION>" << TallyXmlUtils::escapeXml(*voucher.narration) << "</NARRATION>\n";
    }

    for (const auto& entry : voucher.entries) {
        std::string lName = "Unknown";
        auto it = ledgerIdToName.find(entry.ledgerId);
        if (it != ledgerIdToName.end()) {
            lName = it->second;
        }

        bool isDr = (entry.entryType == domain::BalanceType::Dr);
        std::string deemed = isDr ? "Yes" : "No";
        std::string amtStr = isDr ? ("-" + entry.amount.toDbString()) : entry.amount.toDbString();

        oss << "        <ALLLEDGERENTRIES.LIST>\n";
        oss << "          <LEDGERNAME>" << TallyXmlUtils::escapeXml(lName) << "</LEDGERNAME>\n";
        oss << "          <ISDEEMEDPOSITIVE>" << deemed << "</ISDEEMEDPOSITIVE>\n";
        oss << "          <AMOUNT>" << amtStr << "</AMOUNT>\n";
        if (entry.narration.has_value() && !entry.narration->empty()) {
            oss << "          <NARRATION>" << TallyXmlUtils::escapeXml(*entry.narration) << "</NARRATION>\n";
        }
        oss << "        </ALLLEDGERENTRIES.LIST>\n";
    }

    oss << "      </VOUCHER>";
    return oss.str();
}

std::string TallyXmlWriter::wrapEnvelope(const std::string& requestType,
                                        const std::string& reportId,
                                        const std::string& companyName,
                                        const std::string& innerDataXml) {
    return TallyXmlUtils::buildTallyEnvelope(requestType, reportId, companyName, innerDataXml);
}

} // namespace tally::integrations::xml
