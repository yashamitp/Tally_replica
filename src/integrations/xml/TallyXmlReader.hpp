#pragma once

#include <string>
#include <vector>
#include <QXmlStreamReader>
#include "TallyXmlDataTypes.hpp"

namespace tally::integrations::xml {

struct ParsedTallyPayload {
    std::vector<std::string> companyNames;
    std::vector<RawTallyGroup> groups;
    std::vector<RawTallyLedger> ledgers;
    std::vector<RawTallyVoucher> vouchers;
    std::vector<std::string> parseErrors;
};

class TallyXmlReader {
public:
    static ParsedTallyPayload parse(const std::string& xmlContent);

private:
    static void parseEnvelope(QXmlStreamReader& reader, ParsedTallyPayload& payload);
    static void parseBody(QXmlStreamReader& reader, ParsedTallyPayload& payload);
    static void parseData(QXmlStreamReader& reader, ParsedTallyPayload& payload);
    static void parseTallyMessage(QXmlStreamReader& reader, ParsedTallyPayload& payload);
    static RawTallyGroup parseGroup(QXmlStreamReader& reader);
    static RawTallyLedger parseLedger(QXmlStreamReader& reader);
    static RawTallyVoucher parseVoucher(QXmlStreamReader& reader);
    static RawTallyVoucherLine parseLedgerEntry(QXmlStreamReader& reader);
};

} // namespace tally::integrations::xml
