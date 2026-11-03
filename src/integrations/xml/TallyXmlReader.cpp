#include "TallyXmlReader.hpp"
#include "TallyXmlUtils.hpp"
#include "../../utilities/Logger.hpp"
#include <QString>

namespace tally::integrations::xml {

ParsedTallyPayload TallyXmlReader::parse(const std::string& xmlContent) {
    ParsedTallyPayload payload;
    QXmlStreamReader reader(QString::fromStdString(xmlContent));

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString name = reader.name().toString();
            if (name.compare("ENVELOPE", Qt::CaseInsensitive) == 0) {
                parseEnvelope(reader, payload);
            } else if (name.compare("SVCURRENTCOMPANY", Qt::CaseInsensitive) == 0) {
                payload.companyNames.push_back(reader.readElementText().trimmed().toStdString());
            } else if (name.compare("GROUP", Qt::CaseInsensitive) == 0) {
                payload.groups.push_back(parseGroup(reader));
            } else if (name.compare("LEDGER", Qt::CaseInsensitive) == 0) {
                payload.ledgers.push_back(parseLedger(reader));
            } else if (name.compare("VOUCHER", Qt::CaseInsensitive) == 0) {
                payload.vouchers.push_back(parseVoucher(reader));
            }
        }
    }

    if (reader.hasError()) {
        payload.parseErrors.push_back(reader.errorString().toStdString());
    }

    return payload;
}

void TallyXmlReader::parseEnvelope(QXmlStreamReader& reader, ParsedTallyPayload& payload) {
    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString name = reader.name().toString();
            if (name.compare("BODY", Qt::CaseInsensitive) == 0) {
                parseBody(reader, payload);
            } else if (name.compare("SVCURRENTCOMPANY", Qt::CaseInsensitive) == 0) {
                payload.companyNames.push_back(reader.readElementText().trimmed().toStdString());
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name().toString().compare("ENVELOPE", Qt::CaseInsensitive) == 0) {
            break;
        }
    }
}

void TallyXmlReader::parseBody(QXmlStreamReader& reader, ParsedTallyPayload& payload) {
    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString name = reader.name().toString();
            if (name.compare("DATA", Qt::CaseInsensitive) == 0) {
                parseData(reader, payload);
            } else if (name.compare("SVCURRENTCOMPANY", Qt::CaseInsensitive) == 0) {
                payload.companyNames.push_back(reader.readElementText().trimmed().toStdString());
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name().toString().compare("BODY", Qt::CaseInsensitive) == 0) {
            break;
        }
    }
}

void TallyXmlReader::parseData(QXmlStreamReader& reader, ParsedTallyPayload& payload) {
    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString name = reader.name().toString();
            if (name.compare("TALLYMESSAGE", Qt::CaseInsensitive) == 0) {
                parseTallyMessage(reader, payload);
            } else if (name.compare("GROUP", Qt::CaseInsensitive) == 0) {
                payload.groups.push_back(parseGroup(reader));
            } else if (name.compare("LEDGER", Qt::CaseInsensitive) == 0) {
                payload.ledgers.push_back(parseLedger(reader));
            } else if (name.compare("VOUCHER", Qt::CaseInsensitive) == 0) {
                payload.vouchers.push_back(parseVoucher(reader));
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name().toString().compare("DATA", Qt::CaseInsensitive) == 0) {
            break;
        }
    }
}

void TallyXmlReader::parseTallyMessage(QXmlStreamReader& reader, ParsedTallyPayload& payload) {
    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString name = reader.name().toString();
            if (name.compare("GROUP", Qt::CaseInsensitive) == 0) {
                payload.groups.push_back(parseGroup(reader));
            } else if (name.compare("LEDGER", Qt::CaseInsensitive) == 0) {
                payload.ledgers.push_back(parseLedger(reader));
            } else if (name.compare("VOUCHER", Qt::CaseInsensitive) == 0) {
                payload.vouchers.push_back(parseVoucher(reader));
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name().toString().compare("TALLYMESSAGE", Qt::CaseInsensitive) == 0) {
            break;
        }
    }
}

RawTallyGroup TallyXmlReader::parseGroup(QXmlStreamReader& reader) {
    RawTallyGroup grp;
    if (reader.attributes().hasAttribute("NAME")) {
        grp.name = reader.attributes().value("NAME").toString().trimmed().toStdString();
    }

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString tag = reader.name().toString();
            if (tag.compare("NAME", Qt::CaseInsensitive) == 0) {
                grp.name = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("PARENT", Qt::CaseInsensitive) == 0) {
                grp.parentName = reader.readElementText().trimmed().toStdString();
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name().toString().compare("GROUP", Qt::CaseInsensitive) == 0) {
            break;
        }
    }
    return grp;
}

RawTallyLedger TallyXmlReader::parseLedger(QXmlStreamReader& reader) {
    RawTallyLedger l;
    if (reader.attributes().hasAttribute("NAME")) {
        l.name = reader.attributes().value("NAME").toString().trimmed().toStdString();
    }

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString tag = reader.name().toString();
            if (tag.compare("NAME", Qt::CaseInsensitive) == 0) {
                l.name = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("PARENT", Qt::CaseInsensitive) == 0) {
                l.parentGroupName = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("OPENINGBALANCE", Qt::CaseInsensitive) == 0) {
                l.openingBalance = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("ISDEEMEDPOSITIVE", Qt::CaseInsensitive) == 0) {
                l.isDeemedPositive = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("PARTYGSTIN", Qt::CaseInsensitive) == 0) {
                l.gstin = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("PAN", Qt::CaseInsensitive) == 0) {
                l.pan = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("ADDRESS", Qt::CaseInsensitive) == 0) {
                l.address = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("STATENAME", Qt::CaseInsensitive) == 0) {
                l.state = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("PINCODE", Qt::CaseInsensitive) == 0) {
                l.pincode = reader.readElementText().trimmed().toStdString();
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name().toString().compare("LEDGER", Qt::CaseInsensitive) == 0) {
            break;
        }
    }
    return l;
}

RawTallyVoucher TallyXmlReader::parseVoucher(QXmlStreamReader& reader) {
    RawTallyVoucher v;
    if (reader.attributes().hasAttribute("VCHTYPE")) {
        v.voucherTypeName = reader.attributes().value("VCHTYPE").toString().trimmed().toStdString();
    }

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString tag = reader.name().toString();
            if (tag.compare("VOUCHERTYPENAME", Qt::CaseInsensitive) == 0) {
                v.voucherTypeName = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("VOUCHERNUMBER", Qt::CaseInsensitive) == 0) {
                v.voucherNumber = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("DATE", Qt::CaseInsensitive) == 0) {
                v.date = TallyXmlUtils::parseTallyDate(reader.readElementText().trimmed().toStdString());
            } else if (tag.compare("REFERENCE", Qt::CaseInsensitive) == 0) {
                v.referenceNumber = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("NARRATION", Qt::CaseInsensitive) == 0) {
                v.narration = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("GUID", Qt::CaseInsensitive) == 0) {
                v.guid = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("ALLLEDGERENTRIES.LIST", Qt::CaseInsensitive) == 0) {
                v.entries.push_back(parseLedgerEntry(reader));
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name().toString().compare("VOUCHER", Qt::CaseInsensitive) == 0) {
            break;
        }
    }

    // Double-entry validation on parsed entries
    domain::ExactDecimal drSum = domain::ExactDecimal::zero();
    domain::ExactDecimal crSum = domain::ExactDecimal::zero();
    for (const auto& entry : v.entries) {
        if (entry.entryType == domain::BalanceType::Dr) {
            drSum = drSum.add(entry.amount);
        } else {
            crSum = crSum.add(entry.amount);
        }
    }
    v.totalDebit = drSum;
    v.totalCredit = crSum;
    v.isBalanced = (!v.entries.empty() && drSum.equals(crSum) && !drSum.isZero());

    return v;
}

RawTallyVoucherLine TallyXmlReader::parseLedgerEntry(QXmlStreamReader& reader) {
    RawTallyVoucherLine line;
    std::string rawAmount;
    std::string deemed;

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString tag = reader.name().toString();
            if (tag.compare("LEDGERNAME", Qt::CaseInsensitive) == 0) {
                line.ledgerName = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("ISDEEMEDPOSITIVE", Qt::CaseInsensitive) == 0) {
                deemed = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("AMOUNT", Qt::CaseInsensitive) == 0) {
                rawAmount = reader.readElementText().trimmed().toStdString();
            } else if (tag.compare("NARRATION", Qt::CaseInsensitive) == 0) {
                line.narration = reader.readElementText().trimmed().toStdString();
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name().toString().compare("ALLLEDGERENTRIES.LIST", Qt::CaseInsensitive) == 0) {
            break;
        }
    }

    // Tally conventions:
    // ISDEEMEDPOSITIVE = "Yes" -> DR, "No" -> CR
    // Negative AMOUNT -> DR, Positive AMOUNT -> CR
    bool isDr = false;
    if (!deemed.empty()) {
        isDr = (QString::fromStdString(deemed).compare("Yes", Qt::CaseInsensitive) == 0);
    } else if (!rawAmount.empty() && rawAmount[0] == '-') {
        isDr = true;
    }

    line.entryType = isDr ? domain::BalanceType::Dr : domain::BalanceType::Cr;

    // Clean absolute amount string
    std::string cleanAmt = rawAmount;
    if (!cleanAmt.empty() && cleanAmt[0] == '-') {
        cleanAmt = cleanAmt.substr(1);
    }
    if (cleanAmt.empty()) {
        cleanAmt = "0.00";
    }

    try {
        line.amount = domain::ExactDecimal::fromString(cleanAmt);
    } catch (...) {
        line.amount = domain::ExactDecimal::zero();
    }

    return line;
}

} // namespace tally::integrations::xml
