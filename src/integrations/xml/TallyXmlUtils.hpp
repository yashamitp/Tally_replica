#pragma once

#include <string>
#include <regex>
#include <QString>

namespace tally::integrations::xml {

class TallyXmlUtils {
public:
    static std::string escapeXml(const std::string& input) {
        std::string buffer;
        buffer.reserve(input.size());
        for (char c : input) {
            switch (c) {
                case '&':  buffer.append("&amp;"); break;
                case '<':  buffer.append("&lt;"); break;
                case '>':  buffer.append("&gt;"); break;
                case '"':  buffer.append("&quot;"); break;
                case '\'': buffer.append("&apos;"); break;
                default:   buffer.push_back(c); break;
            }
        }
        return buffer;
    }

    static std::string formatTallyDate(const std::string& isoDate) {
        // Expected format: YYYY-MM-DD -> YYYYMMDD
        if (isoDate.size() == 8 && std::all_of(isoDate.begin(), isoDate.end(), ::isdigit)) {
            return isoDate;
        }
        if (isoDate.size() >= 10 && isoDate[4] == '-' && isoDate[7] == '-') {
            return isoDate.substr(0, 4) + isoDate.substr(5, 2) + isoDate.substr(8, 2);
        }
        return isoDate;
    }

    static std::string parseTallyDate(const std::string& tallyDate) {
        // Expected format: YYYYMMDD -> YYYY-MM-DD
        if (tallyDate.size() == 8 && std::all_of(tallyDate.begin(), tallyDate.end(), ::isdigit)) {
            return tallyDate.substr(0, 4) + "-" + tallyDate.substr(4, 2) + "-" + tallyDate.substr(6, 2);
        }
        return tallyDate;
    }

    static std::string buildTallyEnvelope(const std::string& requestType,
                                          const std::string& reportId,
                                          const std::string& companyName,
                                          const std::string& innerDataXml) {
        std::string staticVars;
        if (!companyName.empty()) {
            staticVars = "<SVCURRENTCOMPANY>" + escapeXml(companyName) + "</SVCURRENTCOMPANY>";
        }

        std::string descBlock = staticVars.empty()
            ? "<DESC></DESC>"
            : "<DESC><STATICVARIABLES>" + staticVars + "</STATICVARIABLES></DESC>";

        std::string envelope = "<ENVELOPE>\n"
                               "  <HEADER>\n"
                               "    <VERSION>1</VERSION>\n"
                               "    <TALLYREQUEST>" + escapeXml(requestType) + "</TALLYREQUEST>\n"
                               "    <TYPE>Data</TYPE>\n"
                               "    <ID>" + escapeXml(reportId) + "</ID>\n"
                               "  </HEADER>\n"
                               "  <BODY>\n"
                               "    " + descBlock + "\n"
                               "    <DATA>\n"
                               "      <TALLYMESSAGE xmlns:UDF=\"TallyUDF\">\n" +
                               innerDataXml +
                               "\n      </TALLYMESSAGE>\n"
                               "    </DATA>\n"
                               "  </BODY>\n"
                               "</ENVELOPE>\n";
        return envelope;
    }
};

} // namespace tally::integrations::xml
