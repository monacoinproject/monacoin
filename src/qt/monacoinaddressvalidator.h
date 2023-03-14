// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MONACOIN_QT_MONACOINADDRESSVALIDATOR_H
#define MONACOIN_QT_MONACOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class MonacoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MonacoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Monacoin address widget validator, checks for a valid monacoin address.
 */
class MonacoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MonacoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // MONACOIN_QT_MONACOINADDRESSVALIDATOR_H
